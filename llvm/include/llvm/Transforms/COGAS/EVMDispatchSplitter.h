#ifndef LLVM_TRANSFORMS_COGAS_EVMDISPATCHSPLITTER_H
#define LLVM_TRANSFORMS_COGAS_EVMDISPATCHSPLITTER_H

#include "llvm/Analysis/EVMDispatchFinder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"

namespace llvm {

class DispatchSplitter : public PassInfoMixin<DispatchSplitter> {
public:
  explicit DispatchSplitter(DispatchFinderOpts O = {}) : Opts(O) {}

  PreservedAnalyses run(Module &M, ModuleAnalysisManager &MAM);

  static bool isRequired() { return false; }

private:
  DispatchFinderOpts Opts;
};

} // namespace llvm

#endif // LLVM_TRANSFORMS_COGAS_EVMDISPATCHSPLITTER_H