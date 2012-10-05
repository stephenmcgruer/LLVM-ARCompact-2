//===-- ARCompactInstrInfo.h - ARCompact Instruction Information ------*- C++ -*-===//
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

#ifndef LLVM_TARGET_ARCompactINSTRINFO_H
#define LLVM_TARGET_ARCompactINSTRINFO_H

#include "ARCompactRegisterInfo.h"
#include "llvm/Target/TargetInstrInfo.h"

#define GET_INSTRINFO_HEADER
#include "ARCompactGenInstrInfo.inc"

namespace llvm {

class ARCompactTargetMachine;

class ARCompactInstrInfo : public ARCompactGenInstrInfo {
  const ARCompactRegisterInfo RI;
public:
  explicit ARCompactInstrInfo(ARCompactTargetMachine &TM);

  /// Inserts instructions to copy a pair of physical registers, moving the
  /// contents of SrcReg to DestReg. If KillSrc is set, the source register is
  /// no longer needed.
  virtual void copyPhysReg(MachineBasicBlock &MBB,
                           MachineBasicBlock::iterator I, DebugLoc DL,
                           unsigned DestReg, unsigned SrcReg,
                           bool KillSrc) const;

  /// Store the specified register of the given register class to the specified
  /// stack frame index. The store instruction is to be added to the given
  /// machine basic block before the specified machine instruction. If isKill
  /// is true, the register operand is the last use and must be marked kill.
  virtual void storeRegToStackSlot(MachineBasicBlock &MBB,
      MachineBasicBlock::iterator MI,
      unsigned SrcReg, bool isKill, int FrameIndex,
      const TargetRegisterClass *RC,
      const TargetRegisterInfo *TRI) const;

  /// Load the specified register of the given register class from the
  /// specified stack frame index. The load instruction is to be added to the
  /// given machine basic block before the specified machine instruction.
  virtual void loadRegFromStackSlot(MachineBasicBlock &MBB,
      MachineBasicBlock::iterator MI,
      unsigned DestReg, int FrameIdx,
      const TargetRegisterClass *RC,
      const TargetRegisterInfo *TRI) const;

  virtual const ARCompactRegisterInfo &getRegisterInfo() const { return RI; }
};

}

#endif
