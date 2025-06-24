set(LLVM_LINK_COMPONENTS
  Core
  Support)

add_llvm_tool(contract-opt
  contract-opt.cpp
  
  DEPENDS
  intrinsics_gen
  SUPPORT_PLUGINS)
