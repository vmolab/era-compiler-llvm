#ifndef FINDER_DISPATCHFINDER_H
#define FINDER_DISPATCHFINDER_H

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"

namespace llvm {

struct DispatchFinderOpts {
  SmallVector<unsigned int, 8> Hashes;
};

class DispatchFinder : public llvm::AnalysisInfoMixin<DispatchFinder> {
private:
public:
  using Result = llvm::DenseMap<unsigned int, llvm::BasicBlock *>;

  explicit DispatchFinder(DispatchFinderOpts O = {}) : Opts(std::move(O)) {}

  Result run(llvm::Module &M, llvm::ModuleAnalysisManager &MAM);

  static bool isRequired() { return false; }

private:
  friend llvm::AnalysisInfoMixin<DispatchFinder>;
  static llvm::AnalysisKey Key;

  DispatchFinderOpts Opts;

  llvm::DenseMap<unsigned int, llvm::Instruction *> Found;
};

class DispatchFinderPrinter
    : public llvm::PassInfoMixin<DispatchFinderPrinter> {
public:
  explicit DispatchFinderPrinter(DispatchFinderOpts O = {},
                                 llvm::raw_ostream &ROS = llvm::errs())
      : Opts(std::move(O)), ROS(ROS) {}

  llvm::PreservedAnalyses run(llvm::Module &M,
                              llvm::ModuleAnalysisManager &MAM);

  static bool isRequired() { return false; }

private:
  friend llvm::PassInfoMixin<DispatchFinderPrinter>;

  DispatchFinderOpts Opts;
  llvm::raw_ostream &ROS;
};

} // namespace llvm

#endif // FINDER_DISPATCHFINDER_H