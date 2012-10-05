set(LLVM_TARGET_DEFINITIONS ARCompact.td)

tablegen(LLVM ARCompactGenRegisterInfo.inc -gen-register-info)
tablegen(LLVM ARCompactGenInstrInfo.inc -gen-instr-info)
tablegen(LLVM ARCompactGenAsmWriter.inc -gen-asm-writer)
tablegen(LLVM ARCompactGenDAGISel.inc -gen-dag-isel)
tablegen(LLVM ARCompactGenCallingConv.inc -gen-callingconv)
tablegen(LLVM ARCompactGenSubtargetInfo.inc -gen-subtarget)
add_public_tablegen_target(ARCompactCommonTableGen)

add_llvm_target(ARCompactCodeGen
  ARCompactISelDAGToDAG.cpp
  ARCompactISelLowering.cpp
  ARCompactInstrInfo.cpp
  ARCompactFrameLowering.cpp
  ARCompactMachineFunctionInfo.cpp
  ARCompactRegisterInfo.cpp
  ARCompactSubtarget.cpp
  ARCompactTargetMachine.cpp
  ARCompactSelectionDAGInfo.cpp
  ARCompactAsmPrinter.cpp
  ARCompactMCInstLower.cpp
  )

add_dependencies(LLVMARCompactCodeGen intrinsics_gen)

add_subdirectory(InstPrinter)
add_subdirectory(TargetInfo)
add_subdirectory(MCTargetDesc)
