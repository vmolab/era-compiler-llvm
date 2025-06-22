#include "llvm/Analysis/EVMDispatchFinder.h"

#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/IR/Constants.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

AnalysisKey DispatchFinder::Key;

DispatchFinder::Result DispatchFinder::run(Module &M,
                                           ModuleAnalysisManager &MAM) {
  DenseMap<unsigned int, BasicBlock *> Result;

  // Find the first hash
  for (auto &F : M) {
    for (auto &BB : F) {
      for (auto &I : BB) {
        for (auto &Operand : I.operands()) {
          if (const auto *CI = dyn_cast<ConstantInt>(Operand)) {
            const auto &ConstantValue = CI->getValue();
            errs() << "CONSTANT AT " << BB.getName() << " : "
                   << ConstantValue << "\n";
          }
        }
      }
    }
  }

  return Result;
}

PreservedAnalyses DispatchFinderPrinter::run(Module &M,
                                             ModuleAnalysisManager &MAM) {
  auto HashesFound = DispatchFinder(Opts).run(M, MAM);

  for (const auto &HashBBPair : HashesFound) {
    unsigned int Hash = HashBBPair.first;
    BasicBlock *BB = HashBBPair.second;

    ROS << "Hash " << Hash << " at " << BB->getName() << "\n";
  }

  return PreservedAnalyses::all();
}
