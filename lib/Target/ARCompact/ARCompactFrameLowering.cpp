//===-- ARCompactFrameLowering.cpp - ARCompact Frame Information ----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the ARCompact implementation of TargetFrameLowering class.
//
//===----------------------------------------------------------------------===//

#include "ARCompactFrameLowering.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"

using namespace llvm;

void ARCompactFrameLowering::emitPrologue(MachineFunction &MF) const {
  // Do nothing.
}

void ARCompactFrameLowering::emitEpilogue(MachineFunction &MF,
                                       MachineBasicBlock &MBB) const {
  // Do nothing.
}

bool ARCompactFrameLowering::hasFP(const MachineFunction &MF) const {
  return (MF.getTarget().Options.DisableFramePointerElim(MF) ||
      MF.getFrameInfo()->hasVarSizedObjects());
}