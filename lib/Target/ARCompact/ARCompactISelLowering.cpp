//===-- ARCompactISelLowering.cpp - ARCompact DAG Lowering Implementation -===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the interfaces that ARCompact uses to lower LLVM code
// into a selection DAG.
//
//===----------------------------------------------------------------------===//

#include "ARCompactISelLowering.h"
#include "ARCompactMachineFunctionInfo.h"
#include "ARCompactTargetMachine.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Function.h"
#include "llvm/Module.h"
#include "llvm/CodeGen/CallingConvLower.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
using namespace llvm;

#include "ARCompactGenCallingConv.inc"

ARCompactTargetLowering::ARCompactTargetLowering(ARCompactTargetMachine &tm)
    : TargetLowering(tm, new TargetLoweringObjectFileELF()) {

  TD = getTargetData();

  //DEBUG(dbgs() << "ARCompactTargetLowering::ARCompactTargetLowering()\n");
  // Set up the register classes.
  addRegisterClass(MVT::i32, &ARC::CPURegsRegClass);

  // Compute the derived properties from the register classes.
  computeRegisterProperties();

  // Do not have division, so int-division is expensive.
  setIntDivIsCheap(false);

  setStackPointerRegisterToSaveRestore(ARC::SP);

  // We don't have 1-bit extension (i.e. for bools).
  setLoadExtAction(ISD::EXTLOAD,  MVT::i1,  Promote);
  setLoadExtAction(ISD::SEXTLOAD, MVT::i1,  Promote);
  setLoadExtAction(ISD::ZEXTLOAD, MVT::i1,  Promote);

  // Global addresses are custom lowered to ARCISD:Wrappers.
  setOperationAction(ISD::GlobalAddress,  MVT::i32,   Custom);

  // Expand non-supported branches.
  // TODO: Check if these can be supported.
  setOperationAction(ISD::BR_JT,          MVT::Other, Expand);
  setOperationAction(ISD::BRIND,          MVT::Other, Expand);

  // BRCOND is expanded to ???, BR_CC is lowered to a CMP and Bcc.
  setOperationAction(ISD::BR_CC,          MVT::i32,   Custom);
  setOperationAction(ISD::BRCOND,         MVT::Other, Expand);

  // SELECT is expanded to SELECT_CC, and SELECT_CC is custom lowered to
  // a CMP and Bcc.
  setOperationAction(ISD::SELECT_CC,      MVT::i32,   Custom);
  setOperationAction(ISD::SELECT,         MVT::i32,   Expand);

  // SETCC is expanded to ???
  setOperationAction(ISD::SETCC,          MVT::i32,   Expand);

  // SREM is expanded to ???
  setOperationAction(ISD::SREM,           MVT::i32,   Expand);

  // SDIV is expanded to ???
  setOperationAction(ISD::SDIV,           MVT::i32,   Expand);

  // SDIVREM is expanded to ???
  setOperationAction(ISD::SDIVREM,        MVT::i32,   Expand);

  // Expand UDIV too.
  setOperationAction(ISD::UDIV,           MVT::i32,   Expand);

  // UDIVREM
  setOperationAction(ISD::UDIVREM,        MVT::i32,   Expand);

  // Variadic function-related stuff.
  setOperationAction(ISD::VASTART,        MVT::Other, Custom);
  setOperationAction(ISD::VAARG,          MVT::Other, Expand);
  setOperationAction(ISD::VACOPY,         MVT::Other, Expand);
  setOperationAction(ISD::VAEND,          MVT::Other, Expand);

  setOperationAction(ISD::UREM,           MVT::i32,   Expand);

  setOperationAction(ISD::ROTL,           MVT::i32,   Expand);
  setOperationAction(ISD::ROTR,           MVT::i32,   Expand);

  setOperationAction(ISD::UMUL_LOHI,      MVT::i32,   Expand);

  setOperationAction(ISD::BSWAP,          MVT::i32,   Expand);

  setOperationAction(ISD::DYNAMIC_STACKALLOC, MVT::i32, Expand);

  setOperationAction(ISD::STACKSAVE,      MVT::Other, Expand);
  setOperationAction(ISD::STACKRESTORE,   MVT::Other, Expand);

  setOperationAction(ISD::SIGN_EXTEND_INREG,  MVT::i32, Expand);
  setOperationAction(ISD::SIGN_EXTEND_INREG,  MVT::i16, Expand);
  setOperationAction(ISD::SIGN_EXTEND_INREG,  MVT::i8, Expand);
  setOperationAction(ISD::SIGN_EXTEND_INREG,  MVT::i1, Expand);
}

const char* ARCompactTargetLowering::getTargetNodeName(unsigned Opcode) const {
  //DEBUG(dbgs() << "ARCompactTargetLowering::getTargetNodeName()\n");
  switch (Opcode) {
    case ARCISD::CALL:        return "ARCISD::CALL";
    case ARCISD::CMP:         return "ARCISD::CMP";
    case ARCISD::BR_CC:       return "ARCISD::BR_CC";
    case ARCISD::SELECT_CC:   return "ARCISD::SELECT_CC";
    case ARCISD::Wrapper:     return "ARCISD::Wrapper";
    case ARCISD::RET_FLAG:    return "ARCISD::RET_FLAG";
    default:                  return 0;
  }
}

SDValue ARCompactTargetLowering::LowerOperation(SDValue Op, SelectionDAG &DAG)
    const {
  //DEBUG(dbgs() << "ARCompactTargetLowering::LowerOperation()\n");
  switch (Op.getOpcode()) {
    case ISD::GlobalAddress:        return LowerGlobalAddress(Op, DAG);
    case ISD::BR_CC:                return LowerBR_CC(Op, DAG);
    case ISD::SELECT_CC:            return LowerSELECT_CC(Op, DAG);
    case ISD::VASTART:              return LowerVASTART(Op, DAG);
    case ISD::FRAMEADDR:            return LowerFRAMEADDR(Op, DAG);
    case ISD::RETURNADDR:           return LowerRETURNADDR(Op, DAG);
    default:
      assert(0 && "Unimplemented operation!");
      return SDValue();
  }
}

SDValue ARCompactTargetLowering::LowerVASTART(SDValue Op, SelectionDAG &DAG) const {
  MachineFunction &MF = DAG.getMachineFunction();
  ARCompactMachineFunctionInfo *FuncInfo = MF.getInfo<ARCompactMachineFunctionInfo>();

  // vastart just stores the address of the VarArgsFrameIndex slot into the
  // memory location argument.
  DebugLoc dl = Op.getDebugLoc();
  EVT PtrVT = DAG.getTargetLoweringInfo().getPointerTy();
  SDValue FR = DAG.getFrameIndex(FuncInfo->getVarArgsFrameIndex(), PtrVT);
  const Value *SV = cast<SrcValueSDNode>(Op.getOperand(2))->getValue();
  return DAG.getStore(Op.getOperand(0), dl, FR, Op.getOperand(1),
                      MachinePointerInfo(SV), false, false, 0);
}


static const uint16_t ArgumentRegisters[] = {
  ARC::R0, ARC::R1, ARC::R2, ARC::R3,
  ARC::R4, ARC::R5, ARC::R6, ARC::R7
};

SDValue ARCompactTargetLowering::LowerFormalArguments(SDValue Chain,
    CallingConv::ID CallConv, bool isVarArg,
    const SmallVectorImpl<ISD::InputArg> &Ins, DebugLoc dl, SelectionDAG &DAG,
    SmallVectorImpl<SDValue> &InVals) const {
  //DEBUG(dbgs() << "ARCompactTargetLowering::LowerFormalArguments()\n");
  //DEBUG(dbgs() << "isVarArg? " << isVarArg << "\n");
  //DEBUG(Chain.getNode()->dump());
  MachineFunction &MF = DAG.getMachineFunction();
  MachineFrameInfo *MFI = MF.getFrameInfo();
  ARCompactMachineFunctionInfo *AFI = MF.getInfo<ARCompactMachineFunctionInfo>();

  // Assign locations to all of the incoming arguments.
  SmallVector<CCValAssign, 16> ArgLocs;
  CCState CCInfo(CallConv, isVarArg, DAG.getMachineFunction(),
     getTargetMachine(), ArgLocs, *DAG.getContext());
  CCInfo.AnalyzeFormalArguments(Ins, CC_ARCompact32);

  // Push the arguments onto the InVals vector.
  SDValue ArgValue;
  for (unsigned int i = 0, e = ArgLocs.size(); i != e; ++i) {
    CCValAssign &VA = ArgLocs[i];
    if (VA.isRegLoc()) {
      // Arguments passed in registers.

      const TargetRegisterClass *RC = ARC::CPURegsRegisterClass;
      unsigned int Register = MF.addLiveIn(VA.getLocReg(), RC);
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

  // varargs
  if (isVarArg) {
    // For this, we need to declare any varargs that can fit into the unused
    // parameter registers (register r0-r7.) This involves updating the 
    // ARCompactMachineFunctionInfo with the size of the space needed (num_regs * 4),
    // and sticking the values into the registers. The remaining varargs are
    // the problem of the caller to deal with.
    
    // Work out how many varargs we can fit into registers.
    unsigned FirstFreeIndex = CCInfo.getFirstUnallocated(ArgumentRegisters,
        sizeof(ArgumentRegisters) / sizeof(ArgumentRegisters[0]));
    unsigned NumFreeRegisters = (FirstFreeIndex <= 7) ? 
        (8 - FirstFreeIndex) : 0;
    //dbgs() << "NumFreeRegisters: " << NumFreeRegisters << "\n";

    // We must make sure to preserve the alignment.
    unsigned Align = MF.getTarget().getFrameLowering()->getStackAlignment();
    unsigned VARegSize = NumFreeRegisters * 4;
    unsigned VARegSaveSize = (VARegSize + Align - 1) & ~(Align - 1);
    //dbgs() << "VARegSize: " << VARegSize << "\n";
    //dbgs() << "VARegSaveSize: " << VARegSaveSize << "\n";

    // Update the ARCompactMachineFunctionInfo with the details about the varargs.
    AFI->setVarArgsRegSaveSize(VARegSaveSize);
    int64_t offset = CCInfo.getNextStackOffset() + VARegSaveSize - VARegSize;
    // Adjust the offset for the fact that the save locations starts at least 8
    // above the FP, not at the FP.
    offset += 8;
    //dbgs() << "offset: " << offset << "\n";
    AFI->setVarArgsFrameIndex(MFI->CreateFixedObject(VARegSaveSize,
        offset, false));

    // Create a frame index for the varargs area.
    SDValue FIN = DAG.getFrameIndex(AFI->getVarArgsFrameIndex(),
        getPointerTy());

    // Now place as many varargs into the registers as we can.
    SmallVector<SDValue, 8> MemOps;
    for (; FirstFreeIndex < 8; ++FirstFreeIndex) {
      const TargetRegisterClass *RC = ARC::CPURegsRegisterClass;
      unsigned VReg = MF.addLiveIn(ArgumentRegisters[FirstFreeIndex], RC);
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
  //DEBUG(dbgs() << "ARCompactTargetLowering::LowerReturn()\n");
  MachineFunction &MF = DAG.getMachineFunction();

  // CCValAssign - represent the assignment of the return value to locations.
  SmallVector<CCValAssign, 16> RVLocs;

  // CCState - Info about the registers and stack slot.
  CCState CCInfo(CallConv, isVarArg, DAG.getMachineFunction(),
     DAG.getTarget(), RVLocs, *DAG.getContext());

  // Analize return values.
  CCInfo.AnalyzeReturn(Outs, RetCC_ARCompact32);

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

SDValue ARCompactTargetLowering::LowerCall(SDValue Chain, SDValue Callee,
    CallingConv::ID CallConv, bool isVarArg, bool doesNotRet, bool &isTailCall,
    const SmallVectorImpl<ISD::OutputArg> &Outs,
    const SmallVectorImpl<SDValue> &OutVals,
    const SmallVectorImpl<ISD::InputArg> &Ins,
    DebugLoc dl, SelectionDAG &DAG, SmallVectorImpl<SDValue> &InVals) const {
  //DEBUG(dbgs() << "ARCompactTargetLowering::LowerCall()\n");
  //DEBUG(Chain.getNode()->dump());
  //DEBUG(dbgs() << "isVarArg? " << isVarArg << "\n");
  // ARCompact does not support tail calls yet.
  isTailCall = false;

  // Analyze operands of the call, assigning locations to each operand.
  SmallVector<CCValAssign, 16> ArgLocs;
  CCState CCInfo(CallConv, isVarArg, DAG.getMachineFunction(),
      getTargetMachine(), ArgLocs, *DAG.getContext());

  CCInfo.AnalyzeCallOperands(Outs, CC_ARCompact32);

  // Get a count of how many bytes are to be pushed on the stack.
  unsigned NumBytes = CCInfo.getNextStackOffset();

  Chain = DAG.getCALLSEQ_START(Chain,
      DAG.getConstant(NumBytes, getPointerTy(), true));

  SmallVector<std::pair<unsigned, SDValue>, 4> RegsToPass;
  SmallVector<SDValue, 12> MemOpChains;
  SDValue StackPtr;

  // Walk the register/memloc assignments, inserting copies/loads.
  for (unsigned i = 0, e = ArgLocs.size(); i != e; ++i) {
    CCValAssign &VA = ArgLocs[i];

    SDValue Arg = OutVals[i];
    //DEBUG(Arg.getNode()->dump());

    // Promote the value if needed.
    switch (VA.getLocInfo()) {
      case CCValAssign::Full:
        //DEBUG(dbgs() << "Full.\n");
        break;
      case CCValAssign::SExt:
        Arg = DAG.getNode(ISD::SIGN_EXTEND, dl, VA.getLocVT(), Arg);
        break;
      case CCValAssign::ZExt:
        Arg = DAG.getNode(ISD::ZERO_EXTEND, dl, VA.getLocVT(), Arg);
        break;
      case CCValAssign::AExt:
        Arg = DAG.getNode(ISD::ANY_EXTEND, dl, VA.getLocVT(), Arg);
        break;
      default:
        llvm_unreachable("Unknown loc info!");
    }

    // Arguments that can be passed in a register must be kept in the
    // RegsToPass vector.
    if (VA.isRegLoc()) {
      //DEBUG(dbgs() << "RegLoc!\n");
      RegsToPass.push_back(std::make_pair(VA.getLocReg(), Arg));
    } else {
      //DEBUG(dbgs() << "Else!\n");
      // Sanity check.
      assert(VA.isMemLoc());
      //llvm_unreachable("Mem");

      // Get the stack pointer if needed.
      if (StackPtr.getNode() == 0) {
        StackPtr = DAG.getCopyFromReg(Chain, dl, ARC::SP, getPointerTy());
      }

      SDValue PtrOff = DAG.getNode(ISD::ADD, dl, getPointerTy(), StackPtr,
          DAG.getIntPtrConstant(VA.getLocMemOffset()));

      MemOpChains.push_back(DAG.getStore(Chain, dl, Arg, PtrOff,
          MachinePointerInfo(),false, false, 0));
    }
  }

  // Transform all store nodes into one single node because all store nodes are
  // independent of each other.
  if (!MemOpChains.empty()) {
    Chain = DAG.getNode(ISD::TokenFactor, dl, MVT::Other, &MemOpChains[0],
        MemOpChains.size());
  }

  // Build a sequence of copy-to-reg nodes chained together with token chain and
  // flag operands which copy the outgoing args into registers.  The InFlag in
  // necessary since all emitted instructions must be stuck together.
  SDValue InFlag;
  for (unsigned i = 0, e = RegsToPass.size(); i != e; ++i) {
    Chain = DAG.getCopyToReg(Chain, dl, RegsToPass[i].first,
        RegsToPass[i].second, InFlag);
    InFlag = Chain.getValue(1);
  }

  // If the callee is a GlobalAddress node (quite common, every direct call is)
  // turn it into a TargetGlobalAddress node so that legalize doesn't hack it.
  // Likewise ExternalSymbol -> TargetExternalSymbol.
  if (GlobalAddressSDNode *G = dyn_cast<GlobalAddressSDNode>(Callee)) {
    Callee = DAG.getTargetGlobalAddress(G->getGlobal(), dl, MVT::i32);
  } else if (ExternalSymbolSDNode *E = dyn_cast<ExternalSymbolSDNode>(Callee)) {
    Callee = DAG.getTargetExternalSymbol(E->getSymbol(), MVT::i32);
  }

  // Returns a chain & a flag for retval copy to use.
  SDVTList NodeTys = DAG.getVTList(MVT::Other, MVT::Glue);
  SmallVector<SDValue, 8> Ops;
  Ops.push_back(Chain);
  Ops.push_back(Callee);

  // Add the argument registers to the end of the list so that they are
  // known live into the call.
  for (unsigned i = 0, e = RegsToPass.size(); i != e; ++i) {
    Ops.push_back(DAG.getRegister(RegsToPass[i].first,
        RegsToPass[i].second.getValueType()));
  }

  if (InFlag.getNode()) {
    Ops.push_back(InFlag);
  }

  Chain = DAG.getNode(ARCISD::CALL, dl, NodeTys, &Ops[0], Ops.size());
  InFlag = Chain.getValue(1);

  // Create the CALLSEQ_END node.
  Chain = DAG.getCALLSEQ_END(Chain,
      DAG.getConstant(NumBytes, getPointerTy(), true),
      DAG.getConstant(0, getPointerTy(), true), InFlag);
  InFlag = Chain.getValue(1);

  // Handle result values, copying them out of physregs into vregs that we
  // return.
  return LowerCallResult(Chain, InFlag, CallConv, isVarArg, Ins, dl, DAG,
      InVals);
}

SDValue ARCompactTargetLowering::LowerCallResult(SDValue Chain, SDValue InFlag,
    CallingConv::ID CallConv, bool isVarArg,
    const SmallVectorImpl<ISD::InputArg> &Ins,
    DebugLoc dl, SelectionDAG &DAG,
    SmallVectorImpl<SDValue> &InVals) const {
  //DEBUG(dbgs() << "ARCompactTargetLowering::LowerCallResult()\n");
  // Assign locations to each value returned by this call.
  SmallVector<CCValAssign, 16> RVLocs;
  CCState CCInfo(CallConv, isVarArg,
      DAG.getMachineFunction(), getTargetMachine(), RVLocs, *DAG.getContext());

  CCInfo.AnalyzeCallResult(Ins, RetCC_ARCompact32);

  // Copy all of the result registers out of their specified physreg.
  for (unsigned i = 0; i != RVLocs.size(); ++i) {
    Chain = DAG.getCopyFromReg(Chain, dl, RVLocs[i].getLocReg(),
                               RVLocs[i].getValVT(), InFlag).getValue(1);
    InFlag = Chain.getValue(2);
    InVals.push_back(Chain.getValue(0));
  }

  return Chain;
}

EVT ARCompactTargetLowering::getSetCCResultType(EVT VT) const {
  //DEBUG(dbgs() << "ARCompactTargetLowering::getSetCCResultType()\n");
  return MVT::i32;
}

// Emits and returns an ARCompact compare instruction for the given
// ISD::CondCode.
static SDValue EmitCMP(SDValue &LHS, SDValue &RHS, SDValue &TargetCC,
    ISD::CondCode CC, DebugLoc dl, SelectionDAG &DAG) {
  //DEBUG(dbgs() << "ARCompactTargetLowering::EmitCMP()\n");
  assert(!LHS.getValueType().isFloatingPoint() && "We don't do FP");

  // From the ISD::CondCode documentation:
  //   For integer, only the SETEQ,SETNE,SETLT,SETLE,SETGT,
  //   SETGE,SETULT,SETULE,SETUGT, and SETUGE opcodes are used.

  // FIXME: Handle jump negative someday
  ARCCC::CondCodes TCC = ARCCC::COND_INVALID;
  switch (CC) {
    case ISD::SETEQ:
      TCC = ARCCC::COND_EQ;
      break;
    case ISD::SETNE:
      TCC = ARCCC::COND_NE;
      break;
    case ISD::SETLT:
      TCC = ARCCC::COND_LT;
      break;
    case ISD::SETLE:
      TCC = ARCCC::COND_LE;
      break;
    case ISD::SETGT:
      TCC = ARCCC::COND_GT;
      break;
    case ISD::SETGE:
      TCC = ARCCC::COND_GE;
      break;
    case ISD::SETULT:
      TCC = ARCCC::COND_LO;
      break;
    case ISD::SETULE:
      TCC = ARCCC::COND_LS;
      break;
    case ISD::SETUGT:
      TCC = ARCCC::COND_HI;
      break;
    case ISD::SETUGE:
      TCC = ARCCC::COND_HS;
      break;
    default:
      llvm_unreachable("Invalid integer condition!");
  }

  TargetCC = DAG.getConstant(TCC, MVT::i32);
  return DAG.getNode(ARCISD::CMP, dl, MVT::Glue, LHS, RHS);
}


SDValue ARCompactTargetLowering::LowerBR_CC(SDValue Op, SelectionDAG &DAG)
    const {
  // Arguments are the chain, the condition code, the lhs, the rhs, and the
  // destination.
  SDValue Chain = Op.getOperand(0);
  ISD::CondCode CC = cast<CondCodeSDNode>(Op.getOperand(1))->get();
  SDValue LHS   = Op.getOperand(2);
  SDValue RHS   = Op.getOperand(3);
  SDValue Dest  = Op.getOperand(4);
  DebugLoc dl   = Op.getDebugLoc();

  SDValue TargetCC;

  // Emit a compare instruction to perform the compare and return it
  // so that the branch can refer to it.
  SDValue Flag = EmitCMP(LHS, RHS, TargetCC, CC, dl, DAG);

  return DAG.getNode(ARCISD::BR_CC, dl, Op.getValueType(),
      Chain, Dest, TargetCC, Flag);
}

SDValue ARCompactTargetLowering::LowerSELECT_CC(SDValue Op, SelectionDAG &DAG)
    const {
  // Arguments are the lhs, rhs, true value, false value, and condition code.
  SDValue LHS     = Op.getOperand(0);
  SDValue RHS     = Op.getOperand(1);
  SDValue TrueV   = Op.getOperand(2);
  SDValue FalseV  = Op.getOperand(3);
  ISD::CondCode CC = cast<CondCodeSDNode>(Op.getOperand(4))->get();
  DebugLoc dl    = Op.getDebugLoc();

  SDValue TargetCC;

  // Emit a compare instruction to perform the compare and return it
  // so that the select can refer to it.
  SDValue Flag = EmitCMP(LHS, RHS, TargetCC, CC, dl, DAG);

  SDVTList VTs = DAG.getVTList(Op.getValueType(), MVT::Glue);
  SmallVector<SDValue, 4> Ops;
  Ops.push_back(TrueV);
  Ops.push_back(FalseV);
  Ops.push_back(TargetCC);
  Ops.push_back(Flag);

  // Lower it to an ARCISD::SELECT_CC, which EmitInstrWithCustomInserter
  // will take care of.
  return DAG.getNode(ARCISD::SELECT_CC, dl, VTs, &Ops[0], Ops.size());
}

SDValue ARCompactTargetLowering::getReturnAddressFrameIndex(SelectionDAG &DAG)
    const {
  MachineFunction &MF = DAG.getMachineFunction();
  ARCompactMachineFunctionInfo *FuncInfo =
      MF.getInfo<ARCompactMachineFunctionInfo>();
  int ReturnAddrIndex = FuncInfo->getRAIndex();

  if (ReturnAddrIndex == 0) {
    // Set up a frame object for the return address.
    uint64_t SlotSize = TD->getPointerSize();
    ReturnAddrIndex = MF.getFrameInfo()->CreateFixedObject(SlotSize, -SlotSize,
        false);
    FuncInfo->setRAIndex(ReturnAddrIndex);
  }

  return DAG.getFrameIndex(ReturnAddrIndex, getPointerTy());
}

SDValue ARCompactTargetLowering::LowerRETURNADDR(SDValue Op, SelectionDAG &DAG)
    const {
  MachineFrameInfo *MFI = DAG.getMachineFunction().getFrameInfo();
  MFI->setReturnAddressIsTaken(true);

  unsigned Depth = cast<ConstantSDNode>(Op.getOperand(0))->getZExtValue();
  DebugLoc dl = Op.getDebugLoc();

  if (Depth > 0) {
    SDValue FrameAddr = LowerFRAMEADDR(Op, DAG);
    SDValue Offset = DAG.getConstant(TD->getPointerSize(), MVT::i32);
    return DAG.getLoad(getPointerTy(), dl, DAG.getEntryNode(),
                       DAG.getNode(ISD::ADD, dl, getPointerTy(),
                                   FrameAddr, Offset),
                       MachinePointerInfo(), false, false, false, 0);
  }

  // Just load the return address.
  SDValue RetAddrFI = getReturnAddressFrameIndex(DAG);
  return DAG.getLoad(getPointerTy(), dl, DAG.getEntryNode(),
                     RetAddrFI, MachinePointerInfo(), false, false, false, 0);
}


SDValue ARCompactTargetLowering::LowerFRAMEADDR(SDValue Op, SelectionDAG &DAG)
    const {
  MachineFrameInfo *MFI = DAG.getMachineFunction().getFrameInfo();
  MFI->setFrameAddressIsTaken(true);

  EVT VT = Op.getValueType();
  DebugLoc dl = Op.getDebugLoc();  // FIXME probably not meaningful
  unsigned Depth = cast<ConstantSDNode>(Op.getOperand(0))->getZExtValue();
  unsigned FrameReg = ARC::FP;
  SDValue FrameAddr = DAG.getCopyFromReg(DAG.getEntryNode(), dl, FrameReg, VT);
  while (Depth--)
    FrameAddr = DAG.getLoad(VT, dl, DAG.getEntryNode(), FrameAddr,
        MachinePointerInfo(),
        false, false, false, 0);
  return FrameAddr;
}


SDValue ARCompactTargetLowering::LowerGlobalAddress(SDValue Op,
    SelectionDAG &DAG) const {
  //DEBUG(dbgs() << "ARCompactTargetLowering::LowerGlobalAddress()\n");
  const GlobalValue *GV = cast<GlobalAddressSDNode>(Op)->getGlobal();
  int64_t Offset = cast<GlobalAddressSDNode>(Op)->getOffset();

  // Create the TargetGlobalAddress node, folding in the constant offset.
  SDValue Result = DAG.getTargetGlobalAddress(GV, Op.getDebugLoc(),
      getPointerTy(), Offset);
  return DAG.getNode(ARCISD::Wrapper, Op.getDebugLoc(),
      getPointerTy(), Result);
}

MachineBasicBlock* ARCompactTargetLowering::EmitInstrWithCustomInserter(
    MachineInstr *MI, MachineBasicBlock *BB) const {
  const TargetInstrInfo &TII = *getTargetMachine().getInstrInfo();
  DebugLoc dl = MI->getDebugLoc();
  assert((MI->getOpcode() == ARC::Select) && "We can only emit SELECT_CC");

  // TODO: We can totally use conditional instructions here, can't we?

  // To "insert" a SELECT instruction, we actually have to insert the diamond
  // control-flow pattern.  The incoming instruction knows the destination vreg
  // to set, the condition code register to branch on, the true/false values to
  // select between, and a branch opcode to use.
  const BasicBlock *LLVM_BB = BB->getBasicBlock();
  MachineFunction::iterator I = BB;
  ++I;

  // copy0MBB is the fallthrough block, copy1MBB is the branch
  // block.
  MachineBasicBlock *thisMBB = BB;
  MachineFunction *F = BB->getParent();
  MachineBasicBlock *copy0MBB = F->CreateMachineBasicBlock(LLVM_BB);
  MachineBasicBlock *copy1MBB = F->CreateMachineBasicBlock(LLVM_BB);
  F->insert(I, copy0MBB);
  F->insert(I, copy1MBB);

  // Update machine-CFG edges by transferring all successors of the current
  // block to the new block which will contain the Phi node for the select.
  copy1MBB->splice(copy1MBB->begin(), BB,
                   llvm::next(MachineBasicBlock::iterator(MI)),
                   BB->end());
  copy1MBB->transferSuccessorsAndUpdatePHIs(BB);

  // Next, add the true and fallthrough blocks as its successors.
  BB->addSuccessor(copy0MBB);
  BB->addSuccessor(copy1MBB);

  BuildMI(BB, dl, TII.get(ARC::BCC)).addMBB(copy1MBB)
      .addImm(MI->getOperand(3).getImm());

  //  copy0MBB:
  //   %FalseValue = ...
  //   # fallthrough to copy1MBB
  BB = copy0MBB;

  // Update machine-CFG edges
  BB->addSuccessor(copy1MBB);

  //  copy1MBB:
  //   %Result = phi [ %FalseValue, copy0MBB ], [ %TrueValue, thisMBB ]
  //  ...
  BB = copy1MBB;
  BuildMI(*BB, BB->begin(), dl, TII.get(ARC::PHI),
          MI->getOperand(0).getReg())
    .addReg(MI->getOperand(2).getReg()).addMBB(copy0MBB)
    .addReg(MI->getOperand(1).getReg()).addMBB(thisMBB);

  MI->eraseFromParent();   // The pseudo instruction is gone now.
  return BB;
}

#include <iostream>
/// Do target-specific dag combines on SELECT_CC nodes.
static SDValue PerformSELECTCCCombine(SDNode *N, SelectionDAG &DAG,
    TargetLowering::DAGCombinerInfo &DCI) {
  DebugLoc dl = N->getDebugLoc();
  SDValue LHS = N->getOperand(0);
  SDValue RHS = N->getOperand(1);
  //ISD::CondCode CC = cast<CondCodeSDNode>(N->getOperand(4))->get();

  EVT ValueType = LHS.getValueType();   // LHS and RHS have same VT.

  // Try to form max/min nodes.
  //N->dump();
  //std::cerr << std::endl << "CondCode: " << CC << std::endl;

  return SDValue();
}

SDValue ARCompactTargetLowering::PerformDAGCombine(SDNode *N,
    DAGCombinerInfo &DCI) const {
  SelectionDAG &DAG = DCI.DAG;
  //std::cerr << "PerformDAGCombine" << std::endl;
  //N->dump();
  switch (N->getOpcode()) {
    case ISD::SELECT_CC:
      //std::cerr << "Select!" << std::endl;
      return PerformSELECTCCCombine(N, DAG, DCI);
    default:
      break;
  }

  return SDValue();
}
