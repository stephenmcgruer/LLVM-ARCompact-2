//===---- ARCompactTargetMachine.h - Define TargetMachine for ARCompact ---===//
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

#ifndef ARCOMPACTTARGETMACHINE_H
#define ARCOMPACTTARGETMACHINE_H

#include "ARCompactInstrInfo.h"
#include "ARCompactISelLowering.h"
#include "ARCompactFrameLowering.h"
#include "ARCompactSelectionDAGInfo.h"
#include "ARCompactSubtarget.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetFrameLowering.h"
#include "llvm/DataLayout.h"

namespace llvm {

class ARCompactTargetMachine : public LLVMTargetMachine {
  ARCompactSubtarget Subtarget;
  const DataLayout DL;
  ARCompactTargetLowering TLInfo;
  ARCompactSelectionDAGInfo TSInfo;
  ARCompactInstrInfo InstrInfo;
  ARCompactFrameLowering FrameLowering;
public:
  ARCompactTargetMachine(const Target &T, StringRef TT,
                      StringRef CPU, StringRef FS, const TargetOptions &Options,
                      Reloc::Model RM, CodeModel::Model CM,
                      CodeGenOpt::Level OL);

  virtual const ARCompactInstrInfo *getInstrInfo() const {
    return &InstrInfo;
  }
  virtual const TargetFrameLowering *getFrameLowering() const {
    return &FrameLowering;
  }
  virtual const ARCompactSubtarget *getSubtargetImpl() const {
    return &Subtarget;
  }
  virtual const ARCompactRegisterInfo *getRegisterInfo() const {
    return &InstrInfo.getRegisterInfo();
  }
  virtual const ARCompactTargetLowering* getTargetLowering() const {
    return &TLInfo;
  }
  virtual const ARCompactSelectionDAGInfo* getSelectionDAGInfo() const {
    return &TSInfo;
  }
  virtual const DataLayout *getDataLayout() const {
    return &DL;
  }

  // Pass Pipeline Configuration
  virtual TargetPassConfig *createPassConfig(PassManagerBase &PM);
};

} // end namespace llvm

#endif

