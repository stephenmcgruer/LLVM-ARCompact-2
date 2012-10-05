//===- ARCompactISelLowering.cpp - ARCompact DAG Lowering Implementation  -===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the ARCompactTargetLowering class.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "arcompact-lower"

#include "ARCompactISelLowering.h"
#include "ARCompactTargetMachine.h"
#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"
using namespace llvm;

ARCompactTargetLowering::ARCompactTargetLowering(ARCompactTargetMachine &tm)
    : TargetLowering(tm, new TargetLoweringObjectFileELF()) {
  // Set up the register classes.
  addRegisterClass(MVT::i32, &ARC::CPURegsRegClass);

  // Compute derived properties from the register classes
  computeRegisterProperties();
}

SDValue ARCompactTargetLowering::LowerOperation(SDValue Op, SelectionDAG &DAG)
    const {
  switch (Op.getOpcode()) {
    default:
      llvm_unreachable("unimplemented operand");
  }
}
