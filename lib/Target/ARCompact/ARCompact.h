//==-- ARCompact.h - Top-level interface for ARCompact representation --*- C++ -*-==//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the entry points for global functions defined in
// the LLVM ARCompact backend.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_TARGET_ARCompact_H
#define LLVM_TARGET_ARCompact_H

#include "MCTargetDesc/ARCompactMCTargetDesc.h"
#include "llvm/Target/TargetMachine.h"

namespace ARCCC {
  // ARC specific condition codes.
  enum CondCodes {
    COND_AL,  // Always (i.e. 'True').
    COND_EQ,  // Equal.
    COND_NE,  // Not Equal.
    COND_P,   // Positive.
    COND_N,   // Negative.
    COND_LO,  // Lower Than.
    COND_HS,  // Higher Than or Same.
    COND_V,   // Overflow.
    COND_NV,  // No Overflow.
    COND_GT,  // Greater Than.
    COND_GE,  // Greater Than or Equal To.
    COND_LT,  // Less Than.
    COND_LE,  // Less Than or Equal To.
    COND_HI,  // Higher Than.
    COND_LS,  // Lower Than or Same.
    COND_PNZ, // Positive, non-zero.

    COND_INVALID
  };
}

namespace llvm {
  // Forward declarations.
  class ARCompactTargetMachine;
  class FunctionPass;
  class formatted_raw_ostream;

  FunctionPass *createARCompactISelDag(ARCompactTargetMachine &TM,
      CodeGenOpt::Level OptLevel);
} // end namespace llvm;

#endif
