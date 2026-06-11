add_llvm_library( LLVMDeadBBElimPass MODULE
  DeadBBElimPass.cpp

  PLUGIN_TOOL
  opt
)
