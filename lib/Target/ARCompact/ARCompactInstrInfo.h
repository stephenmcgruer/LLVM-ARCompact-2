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

  virtual const ARCompactRegisterInfo &getRegisterInfo() const { return RI; }
};

}

#endif
