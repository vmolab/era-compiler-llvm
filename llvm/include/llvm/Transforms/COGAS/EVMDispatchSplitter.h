#ifndef LLVM_TRANSFORMS_COGAS_EVMDISPATCHSPLITTER_H
#define LLVM_TRANSFORMS_COGAS_EVMDISPATCHSPLITTER_H

#include "llvm/ADT/StringRef.h"
#include "llvm/Analysis/EVMDispatchFinder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include <utility>

namespace llvm {

struct DispatchSplitterOpts {
  SmallVector<std::pair<unsigned int, StringRef>, 8> HashesAndOptLevel;
};

class DispatchSplitter : public PassInfoMixin<DispatchSplitter> {
public:
  explicit DispatchSplitter(DispatchSplitterOpts O = {}) : Opts(O) {}

  PreservedAnalyses run(Module &M, ModuleAnalysisManager &MAM);

  static bool isRequired() { return false; }

private:
  DispatchSplitterOpts Opts;
};

} // namespace llvm

#endif // LLVM_TRANSFORMS_COGAS_EVMDISPATCHSPLITTER_H