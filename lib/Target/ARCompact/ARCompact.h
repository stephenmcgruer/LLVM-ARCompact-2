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

namespace llvm {
  // Forward declarations.
  class ARCompactTargetMachine;
  class FunctionPass;
  class formatted_raw_ostream;

  FunctionPass *createARCompactISelDag(ARCompactTargetMachine &TM,
      CodeGenOpt::Level OptLevel);
} // end namespace llvm;

#endif
