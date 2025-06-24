#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

int main(int argc, char *argv[]) {
  InitLLVM X(argc, argv);

  errs() << "TEST\n";

  return 0;
}