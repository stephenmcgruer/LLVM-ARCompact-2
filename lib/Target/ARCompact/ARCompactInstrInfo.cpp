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

void ARCompactInstrInfo::storeRegToStackSlot(MachineBasicBlock &MBB,
    MachineBasicBlock::iterator MI, unsigned SrcReg, bool isKill, int FrameIdx,
    const TargetRegisterClass *RC, const TargetRegisterInfo *TRI) const {
  DebugLoc DL;
  if (MI != MBB.end()) DL = MI->getDebugLoc();

  BuildMI(MBB, MI, DL, get(ARC::STrri))
      .addFrameIndex(FrameIdx).addImm(0)
      .addReg(SrcReg, getKillRegState(isKill));
}

void ARCompactInstrInfo::loadRegFromStackSlot(MachineBasicBlock &MBB,
    MachineBasicBlock::iterator MI, unsigned DestReg, int FrameIdx,
    const TargetRegisterClass *RC, const TargetRegisterInfo *TRI) const {
  DebugLoc DL;
  if (MI != MBB.end()) DL = MI->getDebugLoc();

  BuildMI(MBB, MI, DL, get(ARC::LDrli))
      .addReg(DestReg)
      .addFrameIndex(FrameIdx).addImm(0);
}

bool ARCompactInstrInfo::AnalyzeBranch(MachineBasicBlock &MBB,
    MachineBasicBlock *&TBB, MachineBasicBlock *&FBB,
    SmallVectorImpl<MachineOperand> &Cond, bool AllowModify) const {
  // Start from the bottom of the block and work up, examining the
  // terminator instructions.
  MachineBasicBlock::iterator I = MBB.end();
  while (I != MBB.begin()) {
    --I;

    if (I->isDebugValue()) {
      continue;
    }

    // Working from the bottom, when we see a non-terminator
    // instruction, we're done.
    // TODO: Delay slots?
    if (!isUnpredicatedTerminator(I)) {
      break;
    }

    // A terminator that isn't a branch can't easily be handled
    // by this analysis.
    if (!I->getDesc().isBranch()) {
      return true;
    }

    // Handle unconditional branches.
    if (I->getOpcode() == ARC::B) {
      // If we can modify the branch, there are multiple optimisations
      // we can apply.
      if (AllowModify) {
        // If the block has any instructions after the unconditional branch,
        // delete them.
        // TODO: Check for delay slot!
        while (llvm::next(I) != MBB.end()) {
          llvm::next(I)->eraseFromParent();
        }

        // There should be no condition nor false block for an unconditional
        // branch.
        Cond.clear();
        FBB = 0;

        // Delete the branch if it's equivalent to a fall-through.
        if (MBB.isLayoutSuccessor(I->getOperand(0).getMBB())) {
          TBB = 0;
          I->eraseFromParent();
          I = MBB.end();
          continue;
        }
      }

      // TBB is used to indicate the unconditional destination. If there is
      // a preceding conditional branch, this will be moved to FBB then.
      TBB = I->getOperand(0).getMBB();
      continue;
    }

    // Handle conditional branches.
    assert(I->getOpcode() == ARC::BCC && "Invalid branch");
    ARCCC::CondCodes BranchCode =
        static_cast<ARCCC::CondCodes>(I->getOperand(1).getImm());

    if (BranchCode == ARCCC::COND_INVALID) {
      return true;  // Can't handle weird stuff, so just fail.
    }

    // Cond is empty only for the first conditional branch.
    if (Cond.empty()) {
      // A successor unconditional branch may have set TBB, so move it to FBB.
      FBB = TBB;
      TBB = I->getOperand(0).getMBB();
      Cond.push_back(MachineOperand::CreateImm(BranchCode));
      continue;
    }

    // Handle subsequent conditional branches.
    assert(Cond.size() == 1);
    assert(TBB);

    // Only handle the case where all conditional branches branch to
    // the same destination.
    // TODO: Why?
    if (TBB != I->getOperand(0).getMBB()) {
      return true;
    }

    ARCCC::CondCodes OldBranchCode = (ARCCC::CondCodes) Cond[0].getImm();
    // If both branches have the same conditions, we can leave them alone.
    // TODO: Why? When does this happen?
    if (OldBranchCode == BranchCode) {
      continue;
    }

    // If we reach the end of this loop, we have failed to analyze a branching
    // instruction, so return true.
    return true;
  }

  return false; // Success!
}