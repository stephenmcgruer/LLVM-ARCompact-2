//===-- ARCompactInstrInfo.cpp - ARCompact Instruction Information --------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the ARCompact implementation of the TargetInstrInfo class.
//
//===----------------------------------------------------------------------===//

#include "ARCompactInstrInfo.h"
#include "ARCompactTargetMachine.h"

#include "llvm/CodeGen/MachineInstrBuilder.h"

#define GET_INSTRINFO_CTOR
#include "ARCompactGenInstrInfo.inc"

using namespace llvm;

ARCompactInstrInfo::ARCompactInstrInfo(ARCompactTargetMachine &TM)
  : ARCompactGenInstrInfo(ARC::ADJCALLSTACKDOWN, ARC::ADJCALLSTACKUP),
    RI(TM, *this) {
}

void ARCompactInstrInfo::copyPhysReg(MachineBasicBlock &MBB,
    MachineBasicBlock::iterator I, DebugLoc DL, unsigned int DestReg,
    unsigned int SrcReg, bool KillSrc) const {
  BuildMI(MBB, I, DL, get(ARC::MOVrr), DestReg)
      .addReg(SrcReg, getKillRegState(KillSrc));
}
