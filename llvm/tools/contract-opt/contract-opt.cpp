#include "llvm/Transforms/COGAS/EVMDispatchSplitter.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/WithColor.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/AlwaysInliner.h"
#include <memory>
#include <string>
#include <system_error>
#include <utility>

using namespace llvm;

static cl::opt<std::string> InputFilename(cl::Positional,
                                          cl::desc("input *.ll file"),
                                          cl::init("-"), cl::Required);

static cl::opt<std::string> OutputFilename("o", cl::desc("Output filename"),
                                           cl::value_desc("filename"),
                                           cl::init("-"), cl::Optional);

static cl::list<std::string> HashAndOptLevel(
    "hash-opt", cl::desc("Comma-separated list of <hash>:<opt-level> entries"),
    cl::value_desc("<hash>:<opt-level>"), cl::CommaSeparated, cl::Required);

int main(int argc, char *argv[]) {
  InitLLVM X(argc, argv);
  cl::ParseCommandLineOptions(argc, argv, "Method-wise EVM contract optimizer");

  ErrorOr<std::unique_ptr<MemoryBuffer>> ErrorOrMB =
      MemoryBuffer::getFileOrSTDIN(InputFilename);
  if (!ErrorOrMB) {
    WithColor::error(errs(), argv[0])
        << "Could not open input file '" << InputFilename
        << "': " << ErrorOrMB.getError().message() << "\n";
    exit(EXIT_FAILURE);
  }
  std::unique_ptr<MemoryBuffer> &InputMB = *ErrorOrMB;

  std::error_code EC;
  raw_fd_ostream OutStream(OutputFilename, EC, sys::fs::OF_Text);
  if (EC) {
    WithColor::error(errs(), argv[0])
        << "Could not open output file '" << OutputFilename
        << "': " << EC.message() << "\n";
    exit(EXIT_FAILURE);
  }

  LLVMContext Ctx;
  SMDiagnostic SMDiag;

  std::unique_ptr<Module> InputModule = parseIR(*InputMB, SMDiag, Ctx);

  auto LAM = LoopAnalysisManager();
  auto FAM = FunctionAnalysisManager();
  auto CGAM = CGSCCAnalysisManager();
  auto MAM = ModuleAnalysisManager();

  PassBuilder PB;

  PB.registerModuleAnalyses(MAM);
  PB.registerCGSCCAnalyses(CGAM);
  PB.registerFunctionAnalyses(FAM);
  PB.registerLoopAnalyses(LAM);
  PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);

  DispatchSplitterOpts Options;

  for (StringRef Entry : HashAndOptLevel) {
    auto HLPair = Entry.split(':');

    unsigned Hash;
    if (HLPair.first.getAsInteger(0, Hash)) {
      WithColor::error(errs(), argv[0])
          << "Invalid hash value in --hash-opt entry '" << Entry << "'\n";
      exit(EXIT_FAILURE);
    }

    if (Hash > 0xFFFFFFFF) {
      WithColor::error(errs(), argv[0]) << "Hash value too larg\n";
      exit(EXIT_FAILURE);
    }

    StringRef OptLevel = HLPair.second;
    if (OptLevel.empty())
      OptLevel = "0";

    Options.HashesAndOptLevel.emplace_back(Hash, OptLevel);
  }

  ModulePassManager MPM;

  MPM.addPass(DispatchSplitter(Options));
  MPM.addPass(AlwaysInlinerPass());

  MPM.run(*InputModule, MAM);

  MAM.clear();

  InputModule->print(OutStream, nullptr);

  return 0;
}