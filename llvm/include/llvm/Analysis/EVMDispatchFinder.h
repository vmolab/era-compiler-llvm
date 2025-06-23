#ifndef FINDER_DISPATCHFINDER_H
#define FINDER_DISPATCHFINDER_H

#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"

namespace llvm {

struct DispatchFinderOpts {
  SmallVector<unsigned int, 8> Hashes;
};

class DispatchFinder : public AnalysisInfoMixin<DispatchFinder> {
public:
  using Result = SmallVector<std::pair<unsigned int, BasicBlock *>, 8>;

  explicit DispatchFinder(DispatchFinderOpts O = {}) : Opts(std::move(O)) {}

  Result run(Module &M, ModuleAnalysisManager &MAM);

  static bool isRequired() { return false; }

private:
  friend AnalysisInfoMixin<DispatchFinder>;
  static AnalysisKey Key;

  DispatchFinderOpts Opts;

  bool hasHash(unsigned int Hash, BasicBlock &BB);
};

class DispatchFinderPrinter : public PassInfoMixin<DispatchFinderPrinter> {
public:
  explicit DispatchFinderPrinter(DispatchFinderOpts O = {},
                                 raw_ostream &ROS = errs())
      : Opts(std::move(O)), ROS(ROS) {}

  PreservedAnalyses run(Module &M, ModuleAnalysisManager &MAM);

  static bool isRequired() { return false; }

private:
  DispatchFinderOpts Opts;
  raw_ostream &ROS;
};

} // namespace llvm

#endif // FINDER_DISPATCHFINDER_H