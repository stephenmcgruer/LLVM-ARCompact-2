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

#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/ADT/BitVector.h"

#define GET_REGINFO_TARGET_DESC
#include "ARCompactGenRegisterInfo.inc"

using namespace llvm;

static bool IsStore(MachineInstr &MI) {
  // TODO: Set "isStore" variable on such instructions, check for that.
  switch (MI.getOpcode()) {
    case ARC::STrri:
    case ARC::STrli:
    case ARC::STliri:
    case ARC::STrri_i8:
    case ARC::STrri_i16:
      return true;
    default:
      return false;
  }
}

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
    int SPAdj, RegScavenger *RS) const {
  assert(SPAdj == 0 && "Unexpected non-zero adjustment!");

  // Find the frame index operand.
  unsigned i = 0;
  MachineInstr &MI = *II;
  while (!MI.getOperand(i).isFI()) {
    ++i;
    assert(i < MI.getNumOperands() && "Instr doesn't have FrameIndex operand!");
  }
  int FrameIndex = MI.getOperand(i).getIndex();

  // Addressable stack objects are accessed using negative offsets from the
  // frame pointer.
  MachineFunction &MF = *MI.getParent()->getParent();
  int Offset = MF.getFrameInfo()->getObjectOffset(FrameIndex) +
      MI.getOperand(i + 1).getImm();

  // Replace the frame index with a frame pointer reference.
  if (IsStore(MI)) {
    // Store instructions have at most a 9-bit signed immediate offset. Therefore,
    // if the offset is outside this range we need to do something.
    // TODO: What should we do?
    if (Offset >= -256 && Offset <= 255) {
      MI.getOperand(i).ChangeToRegister(ARC::FP, false);
      MI.getOperand(i+1).ChangeToImmediate(Offset);
    } else {
      MI.getOperand(i).ChangeToRegister(ARC::FP, false);
      MI.getOperand(i+1).ChangeToImmediate(255);
      //llvm_unreachable("Offset immediate too small or too big!");
    }
  } else {
    // For now, just default to converting the frame index.
    // TODO: What other instructions hit this function - load?
    MI.getOperand(i).ChangeToRegister(ARC::FP, false);
    MI.getOperand(i+1).ChangeToImmediate(Offset);
  }
}

unsigned ARCompactRegisterInfo::getFrameRegister(const MachineFunction &MF)
    const {
  return ARC::FP;
}