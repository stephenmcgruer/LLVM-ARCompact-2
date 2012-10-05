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
#include "ARCompactMachineFunctionInfo.h"
#include "ARCompactTargetMachine.h"

#include "llvm/CodeGen/CallingConvLower.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"

using namespace llvm;

#include "ARCompactGenCallingConv.inc"

// TODO: Can we pull these from somewhere central?
static const uint16_t ArgumentRegisters[] = {
  ARC::R0, ARC::R1, ARC::R2, ARC::R3,
  ARC::R4, ARC::R5, ARC::R6, ARC::R7
};

ARCompactTargetLowering::ARCompactTargetLowering(ARCompactTargetMachine &tm)
    : TargetLowering(tm, new TargetLoweringObjectFileELF()) {
  // Set up the register classes.
  addRegisterClass(MVT::i32, &ARC::CPURegsRegClass);

  // Compute derived properties from the register classes
  computeRegisterProperties();

  // We do not have division, so mark it as expensive.
  setIntDivIsCheap(false);
}

SDValue ARCompactTargetLowering::LowerOperation(SDValue Op, SelectionDAG &DAG)
    const {
  switch (Op.getOpcode()) {
    default:
      llvm_unreachable("unimplemented operand");
  }
}

SDValue ARCompactTargetLowering::LowerFormalArguments(SDValue Chain,
    CallingConv::ID CallConv, bool isVarArg,
    const SmallVectorImpl<ISD::InputArg> &Ins, DebugLoc dl, SelectionDAG &DAG,
    SmallVectorImpl<SDValue> &InVals) const {
  MachineFunction &MF = DAG.getMachineFunction();
  MachineFrameInfo *MFI = MF.getFrameInfo();
  ARCompactMachineFunctionInfo *AFI =
      MF.getInfo<ARCompactMachineFunctionInfo>();

  // Assign locations to all of the incoming arguments.
  SmallVector<CCValAssign, 16> ArgLocs;
  CCState CCInfo(CallConv, isVarArg, DAG.getMachineFunction(),
      getTargetMachine(), ArgLocs, *DAG.getContext());
  CCInfo.AnalyzeFormalArguments(Ins, CC_ARCompact);

  // Push the arguments onto the InVals vector.
  SDValue ArgValue;
  for (unsigned int i = 0, e = ArgLocs.size(); i != e; ++i) {
    CCValAssign &VA = ArgLocs[i];
    if (VA.isRegLoc()) {
      // Arguments passed in registers.

      TargetRegisterClass RC = ARC::CPURegsRegClass;
      unsigned int Register = MF.addLiveIn(VA.getLocReg(), &RC);
      EVT RegisterValueType = VA.getLocVT();
      ArgValue = DAG.getCopyFromReg(Chain, dl, Register, RegisterValueType);

      InVals.push_back(ArgValue);
    } else {
      // Sanity check
      assert(VA.isMemLoc());

      // Load the argument to a virtual register
      unsigned ObjSize = VA.getLocVT().getSizeInBits()/8;

      if (ObjSize != 4) {
        llvm_unreachable("Memory argument is wrong size - not 32 bit!");
      }

      // Create the frame index object for this incoming parameter...
      int FI = MFI->CreateFixedObject(ObjSize, VA.getLocMemOffset(), true);

      // Create the SelectionDAG nodes corresponding to a load from this
      // parameter.
      SDValue FIN = DAG.getFrameIndex(FI, MVT::i32);
      InVals.push_back(DAG.getLoad(VA.getLocVT(), dl, Chain, FIN,
            MachinePointerInfo::getFixedStack(FI),
            false, false, false, 0));
    }
  }

  if (isVarArg) {
    // For this, we need to declare any varargs that can fit into the unused
    // parameter registers (register r0-r7.) This involves updating the
    // ARCompactMachineFunctionInfo with the size of the space needed
    // (num_regs * 4), and sticking the values into the registers. The
    // remaining varargs are the problem of the caller to deal with.

    // Work out how many varargs we can fit into registers.
    unsigned FirstFreeIndex = CCInfo.getFirstUnallocated(ArgumentRegisters,
        sizeof(ArgumentRegisters) / sizeof(ArgumentRegisters[0]));
    unsigned NumFreeRegisters = (FirstFreeIndex <= 7) ?
        (8 - FirstFreeIndex) : 0;

    // We must make sure to preserve the alignment.
    unsigned Align = MF.getTarget().getFrameLowering()->getStackAlignment();
    unsigned VARegSize = NumFreeRegisters * 4;
    unsigned VARegSaveSize = (VARegSize + Align - 1) & ~(Align - 1);

    // Update the ARCompactMachineFunctionInfo with the details about the
    // varargs.
    AFI->setVarArgsRegSaveSize(VARegSaveSize);
    int64_t offset = CCInfo.getNextStackOffset() + VARegSaveSize - VARegSize;
    // Adjust the offset for the fact that the save locations starts at least 8
    // above the FP, not at the FP.
    offset += 8;
    AFI->setVarArgsFrameIndex(MFI->CreateFixedObject(VARegSaveSize,
          offset, false));

    // Create a frame index for the varargs area.
    SDValue FIN = DAG.getFrameIndex(AFI->getVarArgsFrameIndex(),
        getPointerTy());

    // Now place as many varargs into the registers as we can.
    SmallVector<SDValue, 8> MemOps;
    for (; FirstFreeIndex < 8; ++FirstFreeIndex) {
      TargetRegisterClass RC = ARC::CPURegsRegClass;
      unsigned VReg = MF.addLiveIn(ArgumentRegisters[FirstFreeIndex], &RC);
      SDValue Val = DAG.getCopyFromReg(Chain, dl, VReg, MVT::i32);
      SDValue Store = DAG.getStore(Val.getValue(1), dl, Val, FIN,
          MachinePointerInfo::getFixedStack(AFI->getVarArgsFrameIndex()),
          false, false, 0);
      MemOps.push_back(Store);
      // Increment the frame pointer location.
      FIN = DAG.getNode(ISD::ADD, dl, getPointerTy(), FIN,
          DAG.getConstant(4, getPointerTy()));
    }

    // If we added any varargs, update the chain.
    if (!MemOps.empty()) {
      Chain = DAG.getNode(ISD::TokenFactor, dl, MVT::Other,
          &MemOps[0], MemOps.size());
    }
  }

  return Chain;
}

SDValue ARCompactTargetLowering::LowerReturn(SDValue Chain,
    CallingConv::ID CallConv, bool isVarArg,
    const SmallVectorImpl<ISD::OutputArg> &Outs,
    const SmallVectorImpl<SDValue> &OutVals, DebugLoc dl, SelectionDAG &DAG)
    const {
  MachineFunction &MF = DAG.getMachineFunction();

  // CCValAssign - represent the assignment of the return value to locations.
  SmallVector<CCValAssign, 16> RVLocs;

  // CCState - Info about the registers and stack slot.
  CCState CCInfo(CallConv, isVarArg, DAG.getMachineFunction(),
      DAG.getTarget(), RVLocs, *DAG.getContext());

  // Analize return values.
  CCInfo.AnalyzeReturn(Outs, RetCC_ARCompact);

  // If this is the first return lowered for this function, add the regs to the
  // liveout set for the function.
  if (MF.getRegInfo().liveout_empty()) {
    for (unsigned i = 0; i != RVLocs.size(); ++i)
      if (RVLocs[i].isRegLoc()) {
        MF.getRegInfo().addLiveOut(RVLocs[i].getLocReg());
      }
  }
  SDValue Flag;

  // Copy the result values into the output registers.
  for (unsigned i = 0; i != RVLocs.size(); ++i) {
    CCValAssign &VA = RVLocs[i];
    assert(VA.isRegLoc() && "Can only return in registers!");

    Chain = DAG.getCopyToReg(Chain, dl, VA.getLocReg(), OutVals[i], Flag);

    // Guarantee that all emitted copies are stuck together with flags.
    Flag = Chain.getValue(1);
  }

  // TODO: Check this value.
  unsigned int RetAddrOffset = 8; // Call Instruction + Delay Slot
  SDValue RetAddrOffsetNode = DAG.getConstant(RetAddrOffset, MVT::i32);

  if (Flag.getNode()) {
    return DAG.getNode(ARCISD::RET_FLAG, dl, MVT::Other, Chain,
        RetAddrOffsetNode, Flag);
  }

  return DAG.getNode(ARCISD::RET_FLAG, dl, MVT::Other, Chain,
      RetAddrOffsetNode);
}
