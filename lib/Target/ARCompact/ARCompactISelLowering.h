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

  namespace ARCISD {
    enum {
      // Start at the end of the built-in ops.
      FIRST_NUMBER = ISD::BUILTIN_OP_END,

      // Return with a flag.
      RET_FLAG
    };
  } // end namespace ARCISD

  class ARCompactTargetLowering : public TargetLowering {
  public:
    explicit ARCompactTargetLowering(ARCompactTargetMachine &TM);

    /// LowerOperation - Provide custom lowering hooks for some operations.
    virtual SDValue LowerOperation(SDValue Op, SelectionDAG &DAG) const;

    /// This hook must be implemented to lower the incoming (formal) arguments,
    /// described by the Ins array, into the specified DAG. The implementation
    /// should fill in the InVals array with legal-type argument values, and
    /// return the resulting token chain value.
    virtual SDValue LowerFormalArguments(SDValue Chain,
        CallingConv::ID CallConv, bool isVarArg,
        const SmallVectorImpl<ISD::InputArg> &Ins, DebugLoc dl,
        SelectionDAG &DAG, SmallVectorImpl<SDValue> &InVals) const;

    /// This hook must be implemented to lower outgoing return values,
    /// described by the Outs array, into the specified DAG. The implementation
    /// should return the resulting token chain value.
    virtual SDValue LowerReturn(SDValue Chain, CallingConv::ID CallConv,
        bool isVarArg, const SmallVectorImpl<ISD::OutputArg> &Outs,
        const SmallVectorImpl<SDValue> &OutVals, DebugLoc dl, SelectionDAG &DAG)
        const;
  };
} // namespace llvm

#endif // LLVM_TARGET_ARCompact_ISELLOWERING_H
