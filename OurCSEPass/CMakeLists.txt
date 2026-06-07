add_llvm_library(LLVMOurCSEPass MODULE
  OurCSEPass.cpp
  PLUGIN_TOOL
  opt
)