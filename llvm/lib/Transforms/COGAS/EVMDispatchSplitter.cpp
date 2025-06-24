#include "llvm/Transforms/COGAS/EVMDispatchSplitter.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/ADT/Twine.h"
#include "llvm/IR/Attributes.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Transforms/Utils/CodeExtractor.h"
#include <string>

#include "llvm/Support/raw_ostream.h"

#define DEBUG_TYPE "dispatch-splitter"

using namespace llvm;

static OptimizationLevel parseOptLevel(StringRef &S) {
  if (S == "s" || S == "S")
    return OptimizationLevel::Os;

  if (S == "z" || S == "Z")
    return OptimizationLevel::Oz;

  if (S == "0")
    return OptimizationLevel::O0;

  if (S == "1")
    return OptimizationLevel::O1;

  if (S == "2")
    return OptimizationLevel::O2;

  if (S == "3")
    return OptimizationLevel::O3;

  return OptimizationLevel::O0;
}

PreservedAnalyses DispatchSplitter::run(Module &M, ModuleAnalysisManager &MAM) {
  errs() << "START\n";

  DispatchFinderOpts FinderOpts;
  FinderOpts.Hashes = to_vector<8>(
      map_range(Opts.HashesAndOptLevel, [](const auto &P) { return P.first; }));

  auto DispatchFound = DispatchFinder(FinderOpts).run(M, MAM);

  auto &CGAM = MAM.getResult<CGSCCAnalysisManagerModuleProxy>(M).getManager();
  auto &FAM = MAM.getResult<FunctionAnalysisManagerModuleProxy>(M).getManager();

  Function *MainFunction = M.getFunction("main");
  if (!MainFunction)
    return PreservedAnalyses::all();

  auto &DT = FAM.getResult<DominatorTreeAnalysis>(*MainFunction);

  PassBuilder PB;
  PB.registerFunctionAnalyses(FAM);

  for (size_t i = 0; i < Opts.HashesAndOptLevel.size(); ++i) {
    const auto &Hash = DispatchFound[i].first;
    auto *const BB = DispatchFound[i].second;
    StringRef OptLevelString = Opts.HashesAndOptLevel[i].second;

    errs() << "METHOD " << Hash << "\n";

    BranchInst *MethodBranch = dyn_cast<BranchInst>(BB->getTerminator());
    BasicBlock *MethodStart = MethodBranch->getSuccessor(0);
    SmallVector<BasicBlock *> MethodBody;

    errs() << "BB " << MethodStart->getName() << "\n";

    DT.getDescendants(MethodStart, MethodBody);
    MethodBody.push_back(MethodStart);

    for (auto *BB : MethodBody) {
      errs() << "BB : " << BB->getName() << "\n";
    }

    CodeExtractor CE(
        MethodBody, &DT, /*AggregateArgs=*/false, /*BlockFrequencyI=*/nullptr,
        /*BlockProbabililtyI=*/nullptr, /*AssumptionCache=*/nullptr,
        /*AllowVarArgs=*/false, /*AllowAlloca=*/true);

    if (!CE.isEligible()) {
      errs() << "Error: CodeExtractor is not eligible\n";
      return PreservedAnalyses::none();
    }

    CodeExtractorAnalysisCache CEAC(*MainFunction);
    Function *Method = CE.extractCodeRegion(CEAC);

    Method->addFnAttr(Attribute::AlwaysInline);

    Method->setName("_method_" + Twine(Hash));
    Method->setLinkage(GlobalValue::InternalLinkage);

    errs() << "METHOD " << Hash << " OPT " << OptLevelString << "\n";

    auto &LAM =
        FAM.getResult<LoopAnalysisManagerFunctionProxy>(*Method).getManager();

    PB.registerLoopAnalyses(LAM);
    PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);

    OptimizationLevel OptLevel = parseOptLevel(OptLevelString);

    FunctionPassManager FPM = PB.buildFunctionSimplificationPipeline(
        OptLevel, ThinOrFullLTOPhase::None);

    FPM.run(*Method, FAM);

    errs() << "METHOD " << Hash << " END\n";
  }

  return PreservedAnalyses::none();
}
