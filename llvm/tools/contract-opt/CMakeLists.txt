set(LLVM_LINK_COMPONENTS
  Core
  IRReader
  Passes
  Support)

add_llvm_tool(contract-opt
  contract-opt.cpp
  
  DEPENDS
  intrinsics_gen
  SUPPORT_PLUGINS)
