//==------ ARCompactCallingConv.td - Calling Conventions for ARCompact -----==//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This describes the calling conventions for ARCompact architecture.
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// ARCompact Return Value Calling Convention
//===----------------------------------------------------------------------===//
def RetCC_ARCompact : CallingConv<[
  CCIfType<[i32], CCAssignToReg<[R0]>>
]>;

//===----------------------------------------------------------------------===//
// ARCompact Argument Calling Conventions
//===----------------------------------------------------------------------===//
def CC_ARCompact : CallingConv<[
  // The first 8 int-32 arguments get passed in registers R0-R7.
  CCIfType<[i32], CCAssignToReg<[R0, R1, R2, R3, R4, R5, R6, R7]>>,

  // Everything else is assigned to the stack in 4-byte aligned units.
  CCIfType<[i32], CCAssignToStack<4, 4>> 
]>;
