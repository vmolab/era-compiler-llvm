#include "llvm/Analysis/EVMDispatchFinder.h"

#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/IR/Constants.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"

#define DEBUG_TYPE "dispatch-finder"

using namespace llvm;

AnalysisKey DispatchFinder::Key;

DispatchFinder::Result DispatchFinder::run(Module &M,
                                           ModuleAnalysisManager &MAM) {
  DenseMap<unsigned int, BasicBlock *> Result;
  BasicBlock *HashBB;

  // Find the first hash
  for (auto &F : M) {
    for (auto &BB : F) {
      if (hasHash(Opts.Hashes[0], BB))
        HashBB = &BB;
    }
  }

  for (size_t i = 0; i < Opts.Hashes.size(); ++i) {
    BranchInst *Branch = dyn_cast<BranchInst>(HashBB->getTerminator());
    if (Branch->isUnconditional()) {
      errs() << "Error: Branch was unconditional\n";
      return Result;
    }

    if (!hasHash(Opts.Hashes[i], *HashBB)) {
      errs() << "Error: Cannot find hash: " << Opts.Hashes[i] << " at "
             << HashBB->getName() << "\n";
      return Result;
    }

    Result[Opts.Hashes[i]] = HashBB;

    // False block
    HashBB = Branch->getSuccessor(1);
  }

  return Result;
}

bool DispatchFinder::hasHash(unsigned int Hash, BasicBlock &BB) {
  for (auto &I : BB) {
    for (auto &Operand : I.operands()) {
      if (const auto *CI = dyn_cast<ConstantInt>(Operand)) {
        const auto &ConstantValue = CI->getValue();
        if (Hash == ConstantValue)
          return true;
      }
    }
  }
  return false;
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
