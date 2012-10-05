//===-- ARCompactMCTargetDesc.h - ARCompact Target Descriptions -------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file provides ARCompact specific target descriptions.
//
//===----------------------------------------------------------------------===//

#ifndef ARCompactMCTARGETDESC_H
#define ARCompactMCTARGETDESC_H

namespace llvm {
class Target;

extern Target TheARCompactTarget;

} // End llvm namespace

// Defines symbolic names for ARCompact registers.
// This defines a mapping from register name to register number.
#define GET_REGINFO_ENUM
#include "ARCompactGenRegisterInfo.inc"

// Defines symbolic names for the ARCompact instructions.
#define GET_INSTRINFO_ENUM
#include "ARCompactGenInstrInfo.inc"

#define GET_SUBTARGETINFO_ENUM
#include "ARCompactGenSubtargetInfo.inc"

#endif
