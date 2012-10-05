//===-- ARCompactRegisterInfo.cpp - ARCompact Register Information --------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the ARCompact implementation of the TargetRegisterInfo class.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "arcompact-reg-info"

#include "ARCompactRegisterInfo.h"
#include "ARCompact.h"
#include "ARCompactMachineFunctionInfo.h"
#include "ARCompactTargetMachine.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/ADT/BitVector.h"

#define GET_REGINFO_TARGET_DESC
#include "ARCompactGenRegisterInfo.inc"

using namespace llvm;

ARCompactRegisterInfo::ARCompactRegisterInfo(ARCompactTargetMachine &tm,
                                       const TargetInstrInfo &tii)
    : ARCompactGenRegisterInfo(ARC::BLINK), TM(tm), TII(tii) {
}

const uint16_t* ARCompactRegisterInfo::getCalleeSavedRegs(
    const MachineFunction *MF) const {
  // Taken from page 12 of the ARC GCC calling convention. Not sure
  // if it extends to ARCompact.
  static const uint16_t CalleeSavedRegs[] = { 
    ARC::T5, ARC::T6, ARC::T7, ARC::S0, ARC::S1, ARC::S2, ARC::S3,
    ARC::S4, ARC::S5, ARC::S6, ARC::S7, ARC::S8, ARC::S9, 0
  };  
  return CalleeSavedRegs;
}

BitVector ARCompactRegisterInfo::getReservedRegs(const MachineFunction &MF) 
    const {
  BitVector Reserved(getNumRegs());

  // The global pointer, stack pointer, frame pointer, blink and interrupt
  // link registers are reserved.
  Reserved.set(ARC::GP);
  Reserved.set(ARC::SP);
  Reserved.set(ARC::FP);
  Reserved.set(ARC::BLINK);
  Reserved.set(ARC::ILINK1);
  Reserved.set(ARC::ILINK2);

  return Reserved;
}

void ARCompactRegisterInfo::eliminateFrameIndex(MachineBasicBlock::iterator II,
    int ARCAdj, RegScavenger *RS) const {
  llvm_unreachable("Not implemented yet!");
}

unsigned ARCompactRegisterInfo::getFrameRegister(const MachineFunction &MF)
    const {
  return ARC::FP;
}
