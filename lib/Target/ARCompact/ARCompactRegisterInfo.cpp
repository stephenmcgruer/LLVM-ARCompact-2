//===----- ARCompactRegisterInfo.cpp - ARCOMPACT Register Information -----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the ARCOMPACT implementation of the TargetRegisterInfo
// class.
//
//===----------------------------------------------------------------------===//

#include "ARCompact.h"
#include "ARCompactRegisterInfo.h"
#include "ARCompactSubtarget.h"
#include "ARCompactMachineFunctionInfo.h"
#include "llvm/Function.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

#include "llvm/Target/TargetInstrInfo.h"
#include "llvm/Type.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/ADT/STLExtras.h"

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
  // TODO: If not needed, the frame pointer isnt actually reserved.
  Reserved.set(ARC::GP);
  Reserved.set(ARC::SP);
  Reserved.set(ARC::FP);
  Reserved.set(ARC::BLINK);
  Reserved.set(ARC::ILINK1);
  Reserved.set(ARC::ILINK2);

  Reserved.set(ARC::STATUS32);

  // As with ARC-GCC, I reserve R12 (T4) for temporary calculations.
  Reserved.set(ARC::T4);

  return Reserved;
}

void ARCompactRegisterInfo::eliminateCallFramePseudoInstr(MachineFunction &MF,
    MachineBasicBlock &MBB, MachineBasicBlock::iterator I) const {
  // Simply discard ADJCALLSTACKDOWN and ADJCALLSTACKUP instructions - the
  // callee takes care of moving the stack.
  MBB.erase(I);
}

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

void ARCompactRegisterInfo::eliminateFrameIndex(MachineBasicBlock::iterator II,
    int SPAdj, RegScavenger *RS) const {
  assert(SPAdj == 0 && "Unexpected non-zero adjustment!");

  MachineInstr &MI = *II;
  MachineFunction &MF = *MI.getParent()->getParent();
  MachineFrameInfo *MFI = MF.getFrameInfo();
  ARCompactMachineFunctionInfo *ARCompactFI = 
      MF.getInfo<ARCompactMachineFunctionInfo>();
  MachineBasicBlock &MBB = *MI.getParent();
  DebugLoc dl = II->getDebugLoc();


  // Find the frame index operand.
  unsigned i = 0;
  while (!MI.getOperand(i).isFI()) {
    ++i;
    assert(i < MI.getNumOperands() && "Instr doesn't have FrameIndex operand!");
  }

  DEBUG(errs() << "\nFunction : " << MF.getFunction()->getName() << "\n";
        errs() << "<--------->\n" << MI);

  int FrameIndex = MI.getOperand(i).getIndex();
  uint64_t stackSize = MFI->getStackSize();
  int64_t spOffset = MFI->getObjectOffset(FrameIndex);

  DEBUG(errs() << "FrameIndex : " << FrameIndex << "\n"
               << "spOffset   : " << spOffset << "\n"
               << "stackSize  : " << stackSize << "\n"
               << "VARegsStart: " << ARCompactFI->getVarArgsFrameIndex() << "\n"
               << "VARegsSize : " << ARCompactFI->getVarArgsRegSaveSize() << "\n");

  // TODO: Callee Saved Info?

  // TODO: Figure out when to use SP and when to use FP. For now always use FP.
  unsigned FrameReg = ARC::FP;

  // Figure out the offset. If the frame index is for a memory argument, need to
  // offset by the space used for BLINK (if required) and the old FP (always).
  // TODO: Guess at memloc better.
  int64_t Offset = spOffset;
  if (FrameIndex < 0 && FrameIndex != ARCompactFI->getVarArgsFrameIndex()) {
    // Assuming atm that this means memloc argument.
    Offset += 4; // For the saved FP.
    if (MFI->hasCalls()) {
      Offset += 4; // For the BLINK.
    }
  }
  Offset += MI.getOperand(i+1).getImm();
  DEBUG(errs() << "MI.getOpera: " << MI.getOperand(i+1).getImm() << "\n");
  DEBUG(errs() << "Offset     : " << Offset << "\n");

  if (IsStore(MI) && !isInt<9>(Offset)) {
    // We insert an ADD to get the value within range. Register T4 (R12) is
    // reserved for this purpose.
    int Difference;
    if (Offset > 255) {
      Difference = Offset - 255;
    } else {
      Difference = Offset + 256;
    }
    DEBUG(errs() << "Difference: " << Difference << "\n");

    BuildMI(MBB, II, dl, TII.get(ARC::ADDrli), ARC::T4).addReg(ARC::FP)
        .addImm(Difference);
    Offset -= Difference;

    FrameReg = ARC::T4;
  }

  DEBUG(errs() << "<--------->\n");

  MI.getOperand(i).ChangeToRegister(FrameReg, false);
  MI.getOperand(i+1).ChangeToImmediate(Offset);
}

// Return-Address Register.
unsigned ARCompactRegisterInfo::getRARegister() const {
    return ARC::BLINK;
}

unsigned ARCompactRegisterInfo::getFrameRegister(const MachineFunction &MF)
    const {
  return ARC::FP;
}
