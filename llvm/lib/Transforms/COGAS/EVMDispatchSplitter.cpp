#include "llvm/Transforms/COGAS/EVMDispatchSplitter.h"

#include "llvm/ADT/SetVector.h"
#include "llvm/ADT/Twine.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Transforms/Utils/CodeExtractor.h"
#include <string>

#include "llvm/Support/raw_ostream.h"

#define DEBUG_TYPE "dispatch-splitter"

using namespace llvm;

PreservedAnalyses DispatchSplitter::run(Module &M, ModuleAnalysisManager &MAM) {
  errs() << "START\n";

  auto DispatchFound = DispatchFinder(Opts).run(M, MAM);

  auto &FAMProxy = MAM.getResult<FunctionAnalysisManagerModuleProxy>(M);
  FunctionAnalysisManager &FAM = FAMProxy.getManager();

  Function *MainFunction = M.getFunction("main");
  if (!MainFunction)
    return PreservedAnalyses::all();

  auto &DT = FAM.getResult<DominatorTreeAnalysis>(*MainFunction);

  for (const auto &[Hash, BB] : DispatchFound) {
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

    CodeExtractor CE(MethodBody, &DT, /*AggregateArgs=*/false, /*BFI=*/nullptr,
                     /*BPI=*/nullptr, /*AssumptionCache=*/nullptr,
                     /*AllowVarArgs=*/false, /*AllowAlloca=*/true);

    if (!CE.isEligible()) {
      errs() << "Error: CodeExtractor is not eligible\n";
      return PreservedAnalyses::none();
    }

    CodeExtractorAnalysisCache CEAC(*MainFunction);
    Function *Method = CE.extractCodeRegion(CEAC);

    Method->setName("_method_" + Twine(Hash));
    Method->setLinkage(GlobalValue::InternalLinkage);

    errs() << "METHOD " << Hash << " END\n";
  }

  return PreservedAnalyses::none();
}
