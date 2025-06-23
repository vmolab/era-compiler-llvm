add_llvm_component_library(LLVMCOGASDispatchSplitter
  EVMDispatchSplitter.cpp
    
  LINK_COMPONENTS
  Core
  Analysis
  TransformUtils
  Support)