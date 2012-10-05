//===-- ARCompactISelLowering.h - ARCompact DAG Lowering Interface ----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the interfaces that ARCompact uses to lower LLVM code into a
// selection DAG.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_TARGET_ARCompact_ISELLOWERING_H
#define LLVM_TARGET_ARCompact_ISELLOWERING_H

#include "ARCompact.h"
#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/Target/TargetLowering.h"

namespace llvm {
  class ARCompactTargetLowering : public TargetLowering {
  public:
    explicit ARCompactTargetLowering(ARCompactTargetMachine &TM);

    /// LowerOperation - Provide custom lowering hooks for some operations.
    virtual SDValue LowerOperation(SDValue Op, SelectionDAG &DAG) const;
  };
} // namespace llvm

#endif // LLVM_TARGET_ARCompact_ISELLOWERING_H
