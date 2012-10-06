//===--- ARCompactTargetMachine.cpp - Define TargetMachine for ARCompact --===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#include "ARCompact.h"
#include "ARCompactTargetMachine.h"
#include "llvm/PassManager.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/Support/TargetRegistry.h"

using namespace llvm;

extern "C" void LLVMInitializeARCompactTarget() {
  // Register the target.
  RegisterTargetMachine<ARCompactTargetMachine> X(TheARCompactTarget);
}

ARCompactTargetMachine::ARCompactTargetMachine(const Target &T,
                                         StringRef TT,
                                         StringRef CPU,
                                         StringRef FS,
                                         const TargetOptions &Options,
                                         Reloc::Model RM, CodeModel::Model CM,
                                         CodeGenOpt::Level OL)
  : LLVMTargetMachine(T, TT, CPU, FS, Options, RM, CM, OL),
    Subtarget(TT, CPU, FS),
    // FIXME: Check TargetData string.
    DataLayout(Subtarget.getDataLayout()),
    InstrInfo(*this), TLInfo(*this), TSInfo(*this),
    FrameLowering(Subtarget) { }

namespace {
/// ARCompact Code Generator Pass Configuration Options.
class ARCompactPassConfig : public TargetPassConfig {
public:
  ARCompactPassConfig(ARCompactTargetMachine *TM, PassManagerBase &PM)
    : TargetPassConfig(TM, PM) {}

  ARCompactTargetMachine &getARCompactTargetMachine() const {
    return getTM<ARCompactTargetMachine>();
  }

  virtual bool addInstSelector();
  virtual bool addPreSched2();
};
} // namespace

TargetPassConfig *ARCompactTargetMachine::createPassConfig(PassManagerBase &PM) {
  return new ARCompactPassConfig(this, PM);
}

bool ARCompactPassConfig::addInstSelector() {
  // Install an instruction selector.
  addPass(createARCompactISelDag(getARCompactTargetMachine(), getOptLevel()));
  return false;
}

bool ARCompactPassConfig::addPreSched2() {
  // Install an if converter.
  if (getOptLevel() != CodeGenOpt::None) {
    addPass(&IfConverterID);
  }
  return false;
}