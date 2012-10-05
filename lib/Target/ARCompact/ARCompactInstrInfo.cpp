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

#define GET_INSTRINFO_CTOR
#include "ARCompactGenInstrInfo.inc"

using namespace llvm;

ARCompactInstrInfo::ARCompactInstrInfo(ARCompactTargetMachine &TM)
  : ARCompactGenInstrInfo(),
    RI(TM, *this) {
}
