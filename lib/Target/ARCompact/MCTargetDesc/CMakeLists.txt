add_llvm_library(LLVMARCompactDesc
  ARCompactMCTargetDesc.cpp
  ARCompactMCAsmInfo.cpp
  )

add_dependencies(LLVMARCompactDesc ARCompactCommonTableGen)
