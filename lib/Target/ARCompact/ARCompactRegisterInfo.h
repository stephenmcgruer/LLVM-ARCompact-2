//===-- ARCompactRegisterInfo.h - ARCompact Register Information Impl -*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the ARCompact implementation of the MRegisterInfo class.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_TARGET_ARCompactREGISTERINFO_H
#define LLVM_TARGET_ARCompactREGISTERINFO_H

#include "llvm/Target/TargetRegisterInfo.h"

#define GET_REGINFO_HEADER
#include "ARCompactGenRegisterInfo.inc"

namespace llvm {

class ARCompactTargetMachine;
class TargetInstrInfo;

struct ARCompactRegisterInfo : public ARCompactGenRegisterInfo {
  private:
  ARCompactTargetMachine &TM;
  const TargetInstrInfo &TII;

  public:
  ARCompactRegisterInfo(ARCompactTargetMachine &tm, const TargetInstrInfo &tii);

  /// Returns a null-terminated list of all of the callee saved registers.
  const uint16_t *getCalleeSavedRegs(const MachineFunction *MF = 0) const;

  /// Returns a bitset indexed by physical register number indicating if a
  /// register is a special register that has particular uses and should be
  /// considered unavailable at all times, e.g. SP, BLINK.
  BitVector getReservedRegs(const MachineFunction &MF) const;

  ///  This method is called during prolog/epilog code insertion to eliminate
  /// call frame setup and destroy pseudo instructions, such as ADJCALLSTACKUP
  /// and ADJCALLSTACKDOWN.
  void eliminateCallFramePseudoInstr(MachineFunction &MF,
      MachineBasicBlock &MBB, MachineBasicBlock::iterator I) const;

  /// This method is called during prolog/epilog code insertion to eliminate
  /// call frame setup and destroy pseudo instructions, such as ADJCALLSTACKUP
  /// and ADJCALLSTACKDOWN.
  void eliminateFrameIndex(MachineBasicBlock::iterator II, 
                           int SPAdj, RegScavenger *RS = NULL) const;

  // Debug information queries.
  unsigned getFrameRegister(const MachineFunction &MF) const;
  unsigned getRARegister() const;
};

} // end namespace llvm

#endif // LLVM_TARGET_ARCompactREGISTERINFO_H
