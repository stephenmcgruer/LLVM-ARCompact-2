//===-- ARCompactTargetMachine.h - Define TargetMachine for ARCompact -*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file declares the ARCompact specific subclass of TargetMachine.
//
//===----------------------------------------------------------------------===//


#ifndef LLVM_TARGET_ARCompact_TARGETMACHINE_H
#define LLVM_TARGET_ARCompact_TARGETMACHINE_H

#include "ARCompactInstrInfo.h"
#include "ARCompactISelLowering.h"
#include "ARCompactFrameLowering.h"
#include "ARCompactSelectionDAGInfo.h"
#include "ARCompactRegisterInfo.h"
#include "ARCompactSubtarget.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Target/TargetFrameLowering.h"
#include "llvm/Target/TargetMachine.h"

namespace llvm {

/// ARCompactTargetMachine
///
class ARCompactTargetMachine : public LLVMTargetMachine {
  ARCompactSubtarget        Subtarget;
  const TargetData       DataLayout;       // Calculates type size & alignment
  ARCompactInstrInfo        InstrInfo;
  ARCompactTargetLowering   TLInfo;
  ARCompactSelectionDAGInfo TSInfo;
  ARCompactFrameLowering    FrameLowering;

public:
  ARCompactTargetMachine(const Target &T, StringRef TT,
                      StringRef CPU, StringRef FS, const TargetOptions &Options,
                      Reloc::Model RM, CodeModel::Model CM,
                      CodeGenOpt::Level OL);

  virtual const TargetFrameLowering *getFrameLowering() const {
    return &FrameLowering;
  }
  virtual const ARCompactInstrInfo *getInstrInfo() const  { return &InstrInfo; }
  virtual const TargetData *getTargetData() const     { return &DataLayout;}
  virtual const ARCompactSubtarget *getSubtargetImpl() const { return &Subtarget; }

  virtual const TargetRegisterInfo *getRegisterInfo() const {
    return &InstrInfo.getRegisterInfo();
  }

  virtual const ARCompactTargetLowering *getTargetLowering() const {
    return &TLInfo;
  }

  virtual const ARCompactSelectionDAGInfo* getSelectionDAGInfo() const {
    return &TSInfo;
  }

  virtual TargetPassConfig *createPassConfig(PassManagerBase &PM);
}; // ARCompactTargetMachine.

} // end namespace llvm

#endif // LLVM_TARGET_ARCompact_TARGETMACHINE_H
