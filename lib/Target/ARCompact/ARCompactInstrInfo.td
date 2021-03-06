//===--- ARCompactInstrInfo.td - Target Description for ARCompact Target --===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file describes the ARCompact instructions in TableGen format.
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// Instruction format superclass
//===----------------------------------------------------------------------===//

include "ARCompactInstrFormats.td"

//===----------------------------------------------------------------------===//
// Type Profiles.
//===----------------------------------------------------------------------===//

// Call-related.
def SDT_ARCCall         : SDTypeProfile<0, -1, [SDTCisVT<0, iPTR>]>;
def SDT_ARCCallSeqStart : SDCallSeqStart<[SDTCisVT<0, i32>]>;
def SDT_ARCCallSeqEnd   : SDCallSeqEnd<[SDTCisVT<0, i32>, SDTCisVT<1, i32>]>;

// Wrapper for Global Addresses etc.
def SDT_ARCWrapper      : SDTypeProfile<1, 1, [SDTCisSameAs<0, 1>,
                                               SDTCisPtrTy<0>]>;
// Compare, branch and select.
def SDT_ARCCmp          : SDTypeProfile<0, 2, [SDTCisSameAs<0, 1>]>;
def SDT_ARCBrCC         : SDTypeProfile<0, 2, [SDTCisVT<0, OtherVT>,
                                               SDTCisVT<1, i32>]>;
def SDT_ARCSelectCC     : SDTypeProfile<1, 3, [SDTCisSameAs<0, 1>,
                                               SDTCisSameAs<1, 2>,
                                               SDTCisVT<3, i32>]>;

//===----------------------------------------------------------------------===//
// ARCompact-specific node definitions.
//===----------------------------------------------------------------------===//

// Call-related.
def ARCcall :
    SDNode<"ARCISD::CALL", SDT_ARCCall, [SDNPHasChain, SDNPOutGlue,
                                         SDNPOptInGlue, SDNPVariadic]>;
def ARCcallseq_start :
    SDNode<"ISD::CALLSEQ_START", SDT_ARCCallSeqStart, [SDNPHasChain,
                                                       SDNPOutGlue]>;
def ARCcallseq_end :
    SDNode<"ISD::CALLSEQ_END", SDT_ARCCallSeqEnd, [SDNPHasChain, SDNPOptInGlue,
                                                   SDNPOutGlue]>;

// Wrapper for Global Addresses etc.
def ARCWrapper : SDNode<"ARCISD::Wrapper", SDT_ARCWrapper>;

// Compare, branch, and select.
def ARCcmp      : SDNode<"ARCISD::CMP", SDT_ARCCmp, [SDNPOutGlue]>;
def ARCbrcc     : SDNode<"ARCISD::BR_CC", SDT_ARCBrCC, [SDNPHasChain,
                                                        SDNPInGlue]>;
def ARCselectcc : SDNode<"ARCISD::SELECT_CC", SDT_ARCSelectCC, [SDNPInGlue]>;

// The return flag for a function.
def ARCretflag : SDNode<"ARCISD::RET_FLAG", SDTNone, [SDNPHasChain,
                                                      SDNPOptInGlue]>;

//===----------------------------------------------------------------------===//
// ARCompact Complex Pattern Definitions.
//===----------------------------------------------------------------------===//

// Patterns which match different immediate types.
// TODO: Are long-immediates signed or unsigned?
def uimm6 : PatLeaf<(imm), [{ return isUInt<6>(N->getZExtValue()); }]>;
def simm9 : PatLeaf<(imm), [{ return isInt<9>(N->getSExtValue()); }]>;
def simm12 : PatLeaf<(imm), [{ return isInt<12>(N->getSExtValue()); }]>;
def limm32 : PatLeaf<(imm), [{ return isInt<32>(N->getSExtValue()); }]>;

// Patterns which match addressing modes.
def ADDRri : ComplexPattern<i32, 2, "SelectADDRri",  [frameindex], []>;
def ADDRri2 : ComplexPattern<i32, 2, "SelectADDRri2", [frameindex], []>;
def ADDRli : ComplexPattern<i32, 1, "SelectADDRli",  [frameindex], []>;
def ADDRrr : ComplexPattern<i32, 2, "SelectADDRrr",  [frameindex], []>;
def ADDRrli : ComplexPattern<i32, 2, "SelectADDRrli", [frameindex], []>;
def ADDRlir : ComplexPattern<i32, 2, "SelectADDRlir", [frameindex], []>;

class BinOpFrag<dag res> : PatFrag<(ops node:$LHS, node:$RHS), res>;
class UnOpFrag <dag res> : PatFrag<(ops node:$Src), res>;

//===----------------------------------------------------------------------===//
// ARCompact Operand Definitions.
//===----------------------------------------------------------------------===//

// Memory addressing operands.

// Register + signed immediate.
def MEMri : Operand<i32> {
  let PrintMethod = "printMemOperand";
  let MIOperandInfo = (ops CPURegs, simm9);
}

// Long immediate.
def MEMli : Operand<i32> {
  let PrintMethod = "printMemOperand";
  let MIOperandInfo = (ops limm32);
}

// Register + register.
def MEMrr : Operand<i32> {
  let PrintMethod = "printMemOperand";
  let MIOperandInfo = (ops CPURegs, CPURegs);
}

// Register + long immediate.
def MEMrli : Operand<i32> {
  let PrintMethod = "printMemOperand";
  let MIOperandInfo = (ops CPURegs, limm32);
}

// Long immediate + register.
def MEMlir : Operand<i32> {
  let PrintMethod = "printMemOperand";
  let MIOperandInfo = (ops limm32, CPURegs);
}

// Condition code operands.
def cc : Operand<i32> {
  let PrintMethod = "printCCOperand";
}

// Branch targets have OtherVT type.
def brtarget : Operand<OtherVT>;

//===----------------------------------------------------------------------===//
// ARCompact Multi-Classes.
//===----------------------------------------------------------------------===//

// Captures ARCompact general purpose instructions, which have the following
// types:
//
//   * Register1 = Register2 op Register3
//   * Register1 = Register2 op Unsigned Immediate
//   * Register1 = Register1 op Signed Immediate
//   * Register1 = Register2 op Long Immediate
//
multiclass GenPurposeInst<string opstring, SDNode OpNode> {
  // TODO: Only define pred in the case where $dst = $src1
  def rr : Pseudo<(outs CPURegs:$dst),
                  (ins CPURegs:$src1, CPURegs:$src2, pred:$p),
                  !strconcat(opstring, "$p $dst,$src1,$src2"),
                  [(set CPURegs:$dst, (OpNode CPURegs:$src1, CPURegs:$src2))]>;

  // TODO: Only define pred in the case where $dst = $src1
  def rui : Pseudo<(outs CPURegs:$dst),
                   (ins CPURegs:$src1, i32imm:$src2, pred:$p),
                   !strconcat(opstring, "$p $dst,$src1,$src2"),
                   [(set CPURegs:$dst, (OpNode CPURegs:$src1, uimm6:$src2))]>;

  // In the signed-immediate case, the source and destination registers must be
  // the same register, due to encoding constraints.
  let Constraints = "$src1 = $dst" in {
    def rsi : Pseudo<(outs CPURegs:$dst), (ins CPURegs:$src1, i32imm:$src2),
                        !strconcat(opstring, " $dst,$src1,$src2"),
                        [(set CPURegs:$dst,
                            (OpNode CPURegs:$src1, simm12:$src2))]>;
  }

  def rli : Pseudo<(outs CPURegs:$dst), (ins CPURegs:$src1, i32imm:$src2),
                   !strconcat(opstring, " $dst,$src1,$src2"),
                   [(set CPURegs:$dst, (OpNode CPURegs:$src1, limm32:$src2))]>;

  // TODO: Define .f versions (all), .cc versions (rr, rui, rli), and .cc.f
  //       versions (rr, rui, rli). These *maybe* should go in a different
  //       multi-class.
}

// Models generic ALU operations that do not have a 16-bit version. (See
// page 91.)
//
// Note the enforcement of $src1 = $dst1 for <.cc> is done by isPredicable in
// ARCompactInstrInfo.cpp.
multiclass ALUOp_no16<string opstring, PatFrag OpNode> {
  def rr : Pseudo<(outs CPURegs:$dst),
                  (ins CPURegs:$src1, CPURegs:$src2, pred:$cc),
                  !strconcat(opstring, "$cc $dst,$src1,$src2"),
                  [(set CPURegs:$dst, (OpNode CPURegs:$src1, CPURegs:$src2))]>;

  def rui : Pseudo<(outs CPURegs:$dst),
                   (ins CPURegs:$src1, i32imm:$src2, pred:$cc),
                   !strconcat(opstring, "$cc $dst,$src1,$src2"),
                   [(set CPURegs:$dst, (OpNode CPURegs:$src1, uimm6:$src2))]>;

  let Constraints = "$src1 = $dst" in {
    def rsi : Pseudo<(outs CPURegs:$dst),
                     (ins CPURegs:$src1, i32imm:$src2),
                     !strconcat(opstring, " $dst,$src1,$src2"),
                     [(set CPURegs:$dst,
                         (OpNode CPURegs:$src1, simm12:$src2))]>;
  }

  //def lir : Pseudo<(outs CPURegs:$dst),
  //                 (ins i32imm:$src1, CPURegs:$src2),
  //                 !strconcat(opstring, " $dst,$src1,$src2"),
  //                 [(set CPURegs:$dst, (OpNode limm32:$src1, CPURegs:$src2))]>;

  def rli : Pseudo<(outs CPURegs:$dst),
                   (ins CPURegs:$src1, i32imm:$src2, pred:$cc),
                   !strconcat(opstring, "$cc $dst,$src1,$src2"),
                   [(set CPURegs:$dst, (OpNode CPURegs:$src1, limm32:$src2))]>;
}

// Models generic ALU operations that do have a 16-bit version. (See pages
// 90-91).
//
// Note the enforcement of $src1 = $dst1 for <.cc> is done by isPredicable in
// ARCompactInstrInfo.cpp.
multiclass ALUOp<string opstring, PatFrag OpNode> : ALUOp_no16<opstring, OpNode> {
  // Plus define the 16-bit version.
  // TODO: Define the 16-bit version.
  def alu_placeholder : Pseudo<(outs), (ins), "", []>;
}

// Models generic single operand operations that do not have a 16-bit version.
// (See page 101.)
multiclass SingleOperandOp_no16<string OpString, PatFrag OpNode> {
  def r : Pseudo<(outs CPURegs:$dst),
                 (ins CPURegs:$src),
                 !strconcat(OpString, " $dst,$src"),
                 [(set CPURegs:$dst, (OpNode CPURegs:$src))]>;

  def ui : Pseudo<(outs CPURegs:$dst),
                  (ins i32imm:$src),
                  !strconcat(OpString, " $dst,$src"),
                  [(set CPURegs:$dst, (OpNode uimm6:$src))]>;

  def li : Pseudo<(outs CPURegs:$dst),
                  (ins i32imm:$src),
                  !strconcat(OpString, " $dst,$src"),
                  [(set CPURegs:$dst, (OpNode limm32:$src))]>;
}

// Models generic single operand operations that have a 16-bit version. (See
// page 101.)
multiclass SingleOperandOp<string OpString, PatFrag OpNode>
    : SingleOperandOp_no16<OpString, OpNode> {
  // Plus define the 16-bit version.
  // TODO: Define the 16-bit version.
  def singleop_placeholder : Pseudo<(outs), (ins), "", []>;
}


//===----------------------------------------------------------------------===//
// ARCompact Instructions.
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// Misc Instructions.
//

// ADJCALLSTACKDOWN/UP implicitly use/def SP because they may be expanded into
// a stack adjustment and the codegen must know that they may modify the stack
// pointer before prolog-epilog rewriting occurs.
let Defs = [SP], Uses = [SP] in {
  def ADJCALLSTACKDOWN : Pseudo<(outs), (ins i32imm:$amt),
                                "#ADJCALLSTACKDOWN",
                                [(ARCcallseq_start timm:$amt)]>;
  def ADJCALLSTACKUP   : Pseudo<(outs), (ins i32imm:$amt1, i32imm:$amt2),
                                "#ADJCALLSTACKUP",
                                [(ARCcallseq_end timm:$amt1, timm:$amt2)]>;
}

// Used to manually insert comments into the generated assembly.
def COMMENT : Pseudo<(outs), (ins CPURegs:$fake), "$fake", []>;

// The manual defines the preferred NOP asm as mov 0,0.
def NOP : Pseudo<(outs), (ins), "mov 0,0", []>;

// A return is modelled as an explicit jump from BLINK.
let isReturn = 1, isTerminator = 1, hasDelaySlot = 1, isBarrier = 1 in {
    def RET : Pseudo<(outs), (ins), "j [blink]", [(ARCretflag)]>;
}

let usesCustomInserter = 1 in {
  def Select : Pseudo<(outs CPURegs:$dst),
                      (ins CPURegs:$src, CPURegs:$src2, cc:$cc),
                      "; Select PSEUDO",
                      [(set CPURegs:$dst,
                       (ARCselectcc CPURegs:$src, CPURegs:$src2, imm:$cc))]>;
}

//===----------------------------------------------------------------------===//
// General Instructions - Alphabetical Order.
//

// ADC - Page 179.
//    Add two source operands together, along with the carry value, and place
//    the result in the destination register.

// TODO: Also model the case where the carry is used but not generated
//       (i.e. matching 'add' not 'adde').
let Defs = [STATUS32], Uses = [STATUS32] in {
    defm ADC : GenPurposeInst<"adc.f", adde>;
}

// ADD - Page 180.
//    Add two source operands together, and place the result in the destination
//    register.
//
//    ADD also supports a "add a,limm,c" format, but LLVM knows that add is
//    commutative and so does not allow immediates on the LHS. Instead, this
//    situation is converted to "add a,b,limm" automatically.

// Should be ALUOp, but ISelDAGToDAG emits an ADDrli, and I cant work out how to
// make the predicate implicit...
//defm ADD : ALUOp<"add", BinOpFrag<(add node:$LHS, node:$RHS)>>;
defm ADD : GenPurposeInst<"add", add>;


// Carry-producing add.
// TODO: Fold into a multi-class.
let Defs = [STATUS32] in {
  def ADDrr_f : Pseudo<(outs CPURegs:$dst), (ins CPURegs:$src1, CPURegs:$src2),
                       "add.f $dst,$src1,$src2",
                       [(set CPURegs:$dst,
                            (addc CPURegs:$src1, CPURegs:$src2))]>;
}

// ADD1 - Page 182
//    Add the first source operand to the second source operand bit-shifted
//    left once (src2 << 1), and place the result in the destination register.

defm ADD1 : ALUOp<"add1", BinOpFrag<(add node:$LHS, (shl node:$RHS, 1))>>;

// ADD2 - Page 182
//    Add the first source operand to the second source operand bit-shifted
//    left twice (src2 << 2), and place the result in the destination register.

defm ADD2 : ALUOp<"add2", BinOpFrag<(add node:$LHS, (shl node:$RHS, 2))>>;

// ADD3 - Page 182
//    Add the first source operand to the second source operand bit-shifted
//    left three bits (src2 << 3), and place the result in the destination
//    register.

defm ADD3 : ALUOp<"add3", BinOpFrag<(add node:$LHS, (shl node:$RHS, 3))>>;

// AND - Page 191.
//    Takes the logical bitwise AND of two source operands, and places the
//    result into the destination register.

defm AND : ALUOp<"and", BinOpFrag<(and node:$LHS, node:$RHS)>>;

// ASL - Page 192.
//    Arithmetically shifts the source operand left, and places the result in
//    the destination register. The shift amount can be a constant 1 (in the
//    two-operand version), or an amount given by a second source (in the
//    three-operand version). If a second source is used, only the bottom
//    5 bits of the source will be used.

// Single-shift versions.
defm ASL : SingleOperandOp<"asl", UnOpFrag<(shl node:$Src, 1)>>;

// Multiple-shift versions.
// TODO: Model the 5-bit limit somehow.
defm ASL : ALUOp<"asl", BinOpFrag<(shl node:$LHS, node:$RHS)>>;

// ASR - Page 197.
//    Arithmetically shifts the source operand right, and places the result in
//    the destination register. The shift amount can be a constant 1 (in the
//    two-operand version), or an amount given by a second source (in the
//    three-operand version). If a second source is used, only the bottom
//    5 bits of the source will be used.

// Single-shift versions.
defm ASR : SingleOperandOp<"asr", UnOpFrag<(sra node:$Src, 1)>>;

// Multiple-shift versions.
// TODO: Model the 5-bit limit somehow?
defm ASR : ALUOp<"asr", BinOpFrag<(sra node:$LHS, node:$RHS)>>;

// ASR also supports a "ASR a,limm,c" format.
def ASRlir : Pseudo<(outs CPURegs:$dst), (ins i32imm:$src1, CPURegs:$src2),
                    "asr $dst,$src1,$src2",
                    [(set CPURegs:$dst, (sra limm32:$src1, CPURegs:$src2))]>;

// Bcc - Page 206.
//    Branches to another location, with an optional condition code. The
//    condition is evaluated against the state of the condition bits in
//    STATUS32; that is, the actual source of the condition must be computed
//    previously.

let isBranch = 1, isTerminator = 1 in {
  // Unconditional branches.
  let isBarrier = 1 in {
      def B : Pseudo<(outs), (ins brtarget:$dst),
                     "b @$dst",
                     [(br bb:$dst)]>;
  } // isBarrier

  // Conditional branches.
  let Uses = [STATUS32] in {
      def BCC : Pseudo<(outs), (ins brtarget:$dst, cc:$cc),
                       "b$cc @$dst",
                       [(ARCbrcc bb:$dst, imm:$cc)]>;
  } // Uses = [STATUS32]
} // isBranch, isTerminator

// BCLR - Page 210.
//    Clear a bit in the value given by the first source operand; the position
//    is given by the value in the second source operand. The result is placed
//    in the destination register.

// TODO: Not actually an ALUOp, should specify correct class.
defm BCLR : ALUOp<"bclr", BinOpFrag<(and node:$LHS,
                                        (not (shl 1, node:$RHS)))>>;

// BIC - Page 211.
//    Takes the logical bitwise AND of the first source operand with the
//    inverse (logical NOT) of the second source operand, and places the
//    result into the destination register.

defm BIC : ALUOp<"bic", BinOpFrag<(and node:$LHS, (not node:$RHS))>>;

// BLcc - Page 212.
//    When the specified condition code is met (cc = true in the unconditional
//    case), the program jumps to the given label and the address of the BLcc
//    instruction is stored in the link register BLINK.

// TODO: Add clobbered registers.
// TODO: Should this clobber STATUS32 or not?
// TODO: Add conditional branch + links.
let isCall = 1 in {
  // All calls clobber the non-callee saved registers. SP is marked as
  // a use to prevent stack-pointer assignments that appear immediately
  // before calls from potentially appearing dead.
  let Defs = [R0, R1, R2, R3, R4, R5, R6, R7, T0, T1, T2, T3, T4, STATUS32],
      Uses = [SP] in {
    // Unconditional jump.
    def BLi : Pseudo<(outs), (ins i32imm:$dst, variable_ops),
                       "bl @$dst",
                       [(ARCcall imm:$dst)]>;
    // Branch and link can't handle a register source. Jump can.
    def JLr : Pseudo<(outs), (ins CPURegs:$dst, variable_ops),
                       "jl [$dst]",
                       [(ARCcall CPURegs:$dst)]>;
  } // Defs = [STATUS32], Uses = [SP]
} // isCall

// BMSK - Page 214.
//    Applies a bitmask made up of logical 1s starting from the LSB up to and
//    including the position specified by the second source operand to the
//    value specified in the first source operand. The result is placed in
//    the destination register.

// TODO: Not actually ALUOp, should specify correct class.
// TODO: Doesn't seem to match test.
defm BMSK : ALUOp<"bmsk", BinOpFrag<(and node:$LHS,
                                        (add (shl 1, (add node:$RHS, 1)), -1))>>;

// BSET - Page 222.
//    Sets a bit in the value given by the first source operand; the position
//    is given by the value in the second source operand. The result is placed
//    in the destination register.

// TODO: Not actually ALUOp, should specify correct class.
defm BSET : ALUOp<"bset", BinOpFrag<(or node:$LHS, (shl 1, node:$RHS))>>;

// BXOR - Page 224.
//    Toggles a bit in the value given by the first source operand; the
//    position is given by the value in the second source operand. The result
//    is placed in the destination register.

// TODO: Not actually ALUOp, should specify correct class.
defm BXOR : ALUOp<"bxor", BinOpFrag<(xor node:$LHS, (shl 1, node:$RHS))>>;

// EXTB - Page 230.
//    Zero extend the byte value in the source operand and write the result
//    into the destination register.
//
//    EXTB is a special case as LLVM turns 'zext $src to i8' into
//    'and $src, 255'. Therefore, EXTB is properly matched by an
//    anonymous pattern below.
def EXTBr : Pseudo<(outs CPURegs:$dst), (ins CPURegs:$src),
                   "extb $dst,$src",
                   []>;

def EXTBui : Pseudo<(outs CPURegs:$dst), (ins i32imm:$src),
                   "extb $dst,$src",
                   []>;

def EXTBli : Pseudo<(outs CPURegs:$dst), (ins i32imm:$src),
                   "extb $dst,$src",
                   []>;

// EXTW - Page 231.
//    Zero extend the word value in the source operand and write the result
//    into the destination register.
//
//    EXTW is a special case as LLVM turns 'zext $src to i16' into
//    'and $src, 65535'. Therefore, EXTW is properly matched by an
//    anonymous pattern below.
def EXTWr : Pseudo<(outs CPURegs:$dst), (ins CPURegs:$src),
                   "extw $dst,$src",
                   []>;

def EXTWui : Pseudo<(outs CPURegs:$dst), (ins i32imm:$src),
                   "extw $dst,$src",
                   []>;


def EXTWli : Pseudo<(outs CPURegs:$dst), (ins i32imm:$src),
                   "extw $dst,$src",
                   []>;


// CMP - Page 225.
//    Performs a comparison by subtracting the second source operand from the
//    first, updating the condition flags based on the result. The actual
//    result of the subtraction is discarded.

let Defs = [STATUS32] in {
  def CMPrr : Pseudo<(outs), (ins CPURegs:$src1, CPURegs:$src2),
                      "cmp $src1,$src2",
                      [(ARCcmp CPURegs:$src1, CPURegs:$src2)]>;

  def CMPrsi : Pseudo<(outs), (ins CPURegs:$src1, i32imm:$src2),
                      "cmp $src1,$src2",
                      [(ARCcmp CPURegs:$src1, simm12:$src2)]>;

  def CMPrui : Pseudo<(outs), (ins CPURegs:$src1, i32imm:$src2),
                      "cmp $src1,$src2",
                      [(ARCcmp CPURegs:$src1, uimm6:$src2)]>;

  def CMPrli : Pseudo<(outs), (ins CPURegs:$src1, i32imm:$src2),
                      "cmp $src1,$src2",
                      [(ARCcmp CPURegs:$src1, limm32:$src2)]>;

  def CMPlir : Pseudo<(outs), (ins i32imm:$src1, CPURegs:$src2),
                      "cmp $src1,$src2",
                      [(ARCcmp limm32:$src1, CPURegs:$src2)]>;
} // Defs = [STATUS32]

// LD - Page 239.
//    Loads the value stored at a memory address into the destination register.
//    The memory address may be given as a single long immediate, or as two
//    operands whose values will be added together to get the address.
//
//    The suffixes given here refer to the memory address format - for example,
//    ri means an address given by a register + a signed 9-bit immediate.

def LDri : Pseudo<(outs CPURegs:$dst), (ins MEMri:$addr),
                  "ld $dst,$addr",
                  [(set CPURegs:$dst, (load ADDRri:$addr))]>;

def LDli : Pseudo<(outs CPURegs:$dst), (ins MEMli:$addr),
                   "ld $dst,$addr",
                   [(set CPURegs:$dst, (load ADDRli:$addr))]>;

def LDrr : Pseudo<(outs CPURegs:$dst), (ins MEMrr:$addr),
                    "ld $dst,$addr",
                    [(set CPURegs:$dst, (load ADDRrr:$addr))]>;

//def LDrli : Pseudo<(outs CPURegs:$dst), (ins MEMrli:$addr),
//                    "ld $dst,$addr",
//                    [(set CPURegs:$dst, (load ADDRrli:$addr))]>;

def LDlir : Pseudo<(outs CPURegs:$dst), (ins MEMlir:$addr),
                    "ld $dst,$addr",
                    [(set CPURegs:$dst, (load ADDRlir:$addr))]>;

// Zero-extend versions of LDri.
// TODO: Add LDli, LDrr, LDrli, LDlir.

def LDri_extb : Pseudo<(outs CPURegs:$dst), (ins MEMri:$addr),
                       "ldb $dst,$addr",
                       [(set CPURegs:$dst, (zextloadi8 ADDRri:$addr))]>;

def LDri_extw : Pseudo<(outs CPURegs:$dst), (ins MEMri:$addr),
                       "ldw $dst,$addr",
                       [(set CPURegs:$dst, (zextloadi16 ADDRri:$addr))]>;

// Sign-extend versions of LDri.
// TODO: Add LDli, LDrr, LDrli, LDlir.

def LDri_sextb : Pseudo<(outs CPURegs:$dst), (ins MEMri:$addr),
                        "ldb.x $dst,$addr",
                        [(set CPURegs:$dst, (sextloadi8 ADDRri:$addr))]>;

def LDri_sextw : Pseudo<(outs CPURegs:$dst), (ins MEMri:$addr),
                        "ldw.x $dst,$addr",
                        [(set CPURegs:$dst, (sextloadi16 ADDRri:$addr))]>;

// Address-write back versions of ST. These are not pattern matched yet but
// are provided for use by the prologue/epilogue emitters.
let mayLoad = 1, hasSideEffects = 1 in {
def LDri_a : Pseudo<(outs CPURegs:$src), (ins MEMri:$addr),
                    "ld.a $src,$addr",
                    []>;

def LDri_ab : Pseudo<(outs CPURegs:$src), (ins MEMri:$addr),
                    "ld.ab $src,$addr",
                    []>;
}

// LSR - Page 254.
//    Logically shifts the source operand right, and places the result in the
//    destination register. The shift amount can be a constant 1 (in the
//    two-operand version), or an amount given by a second source (in the
//    three-operand version). If a second source is used, only the bottom
//    5 bits of the source will be used.

// Single-shift versions.
defm LSR : SingleOperandOp<"lsr", UnOpFrag<(srl node:$Src, 1)>>;

// Multiple-shift versions.
// TODO: Model the 5-bit limit somehow?
defm LSR : ALUOp<"lsr", BinOpFrag<(srl node:$LHS, node:$RHS)>>;

// MOV - Page 262.
//    The contents of the source are moved into the destination register.

// No pattern is defined for register-to-register moves, as LLVM is unable to
// match them. Instead, copyPhysReg in ARCompactInstrInfo.cpp is responsible
// for emitting this instruction when appropriate.
let neverHasSideEffects = 1 in {
  def MOVrr : Pseudo<(outs CPURegs:$dst), (ins CPURegs:$src),
                     "mov $dst,$src",
                     []>;
}

let isAsCheapAsAMove = 1 in {
  def MOVrui : Pseudo<(outs CPURegs:$dst), (ins i32imm:$imm),
                      "mov $dst,$imm",
                      [(set CPURegs:$dst, uimm6:$imm)]>;

  def MOVrsi : Pseudo<(outs CPURegs:$dst), (ins i32imm:$imm),
                      "mov $dst,$imm",
                      [(set CPURegs:$dst, simm12:$imm)]>;

  def MOVrli : Pseudo<(outs CPURegs:$dst), (ins i32imm:$imm),
                      "mov $dst,$imm",
                      [(set CPURegs:$dst, limm32:$imm)]>;
}

// MPY - Page 263.
//    Performs a signed 32-bit by 32-bit multiply of the source operands
//    and places the least significant bits of the result in the destination
//    register.
// TODO: Support other formats than register, register -> register.
def MPYrr : Pseudo<(outs CPURegs:$dst), (ins CPURegs:$src1, CPURegs:$src2),
                    "mpy $dst,$src1,$src2",
                    [(set CPURegs:$dst, (mul CPURegs:$src1, CPURegs:$src2))]>;

// MPYHU - Page 267.
//    Performs an unsinged 32-bit by 32-bit multiply of the source operands
//    and places the most significant bits of the result in the destination
//    register.
// TODO: Support other formats than register, register -> register.
def MPYHUrr : Pseudo<(outs CPURegs:$dst), (ins CPURegs:$src1, CPURegs:$src2),
                     "mpyhu $dst,$src1,$src2",
                     [(set CPURegs:$dst, (mulhu CPURegs:$src1, CPURegs:$src2))]>;

// MPYH - Page 265.
//    Performs a signed 32-bit by 32-bit multiple of the source operands and
//    places the most significant bits of the result in the destination 
//    register.
// TODO: Support other formats than register, register -> register.
def MPYHrr : Pseudo<(outs CPURegs:$dst), (ins CPURegs:$src1, CPURegs:$src2),
                    "mpyh $dst,$src1,$src2",
                    [(set CPURegs:$dst, (mulhs CPURegs:$src1, CPURegs:$src2))]>;

// NEG - page 275.
//    The source value is subtracted from 0 and the result placed in the
//    destination register.
//
//    NEG only has a register-to-register version.

// Note the enforcement of $src = $dst for <.cc> is done by isPredicable in
// ARCompactInstrInfo.cpp.
def NEGrr : Pseudo<(outs CPURegs:$dst), (ins CPURegs:$src, pred:$cc),
                   "neg$cc $dst,$src",
                   [(set CPURegs:$dst, (ineg CPURegs:$src))]>;

// NOT - page 283.
//    Takes the logical bitwise NOT of the source operand, and places the
//    result into the destination register.

defm NOT : SingleOperandOp<"not", UnOpFrag<(not node:$Src)>>;

// OR - page 284.
//    Takes the logical bitwise OR of the source operands, and places the
//    result into the destination register.

defm OR : ALUOp<"or", BinOpFrag<(or node:$LHS, node:$RHS)>>;

// SBC - page 302.
//    Subtracts the second source operand from the first, and then also
//    subtracts the carry from this value. The result is placed in the
//    destination register.

// TODO: Also model the case where the carry is used but not generated
//       (i.e. matching 'sub' not 'sube').
let Defs = [STATUS32], Uses = [STATUS32] in {
    defm SBC : GenPurposeInst<"sbc.f", sube>;
}

// SEXB - page 304.
//    Sign extends the byte contained in the source operand to the most
//    significant bit in a long word (32-bits) and places the result in
//    the destination register.

// TODO: These may not be right. Maybe should use sext?
defm SEXB : SingleOperandOp<"sexb", UnOpFrag<(sext_inreg node:$Src, i8)>>;

// SEXW - page 304.
//    Sign extends the word (16-bits) contained in the source operand to the
//    most significant bit in a long word (32-bits) and places the result in
//    the destination register.

// TODO: These may not be right. Maybe should use sext?
defm SEXW : SingleOperandOp<"sexw", UnOpFrag<(sext_inreg node:$Src, i16)>>;

// ST - page 310.
//    Stores the value stored in the source operand in the destination memory
//    address. The source operand may either be a register or a long immediate
//    value. The memory address may be given as a single long immediate, or
//    as two operands whose values will be added together to get the address.
//
//    The suffixes given here refer to the whole instruction format - for
//    example, rri means store a value from a register into an address given by
//    a register + a signed 9-bit immediate.

def STrri : Pseudo<(outs), (ins MEMri:$addr, CPURegs:$src),
                  "st $src,$addr",
                  [(store CPURegs:$src, ADDRri:$addr)]>;

def STrli : Pseudo<(outs), (ins MEMli:$addr, CPURegs:$src),
                  "st $src,$addr",
                  [(store CPURegs:$src, ADDRli:$addr)]>;

def STliri : Pseudo<(outs), (ins MEMri:$addr, i32imm:$src),
                    "st $src,$addr",
                    [(store limm32:$src, ADDRri2:$addr)]>;

// Truncuated versions of STrri.
// TODO: Add STrli, STliri.
def STrri_i8 : Pseudo<(outs), (ins MEMri:$addr, CPURegs:$src),
                      "stb $src,$addr",
                      [(truncstorei8 CPURegs:$src, ADDRri:$addr)]>;

def STrri_i16 : Pseudo<(outs), (ins MEMri:$addr, CPURegs:$src),
                      "stw $src,$addr",
                      [(truncstorei16 CPURegs:$src, ADDRri:$addr)]>;

// Address-write back versions of ST. These are not pattern matched yet but
// are provided for use by the prologue/epilogue emitters.
let mayStore = 1, hasSideEffects = 1 in {
def STrri_a : Pseudo<(outs), (ins MEMri:$addr, CPURegs:$src),
                      "st.a $src,$addr",
                      []>;
}

// SUB - Page 312.
//    Subtracts the second source operand from the first, and places the result
//    into the destination register.

defm SUB : ALUOp<"sub", BinOpFrag<(sub node:$LHS, node:$RHS)>>;

// SUB also supports a "SUB a,limm,c" format.
def SUBlir : Pseudo<(outs CPURegs:$dst), (ins i32imm:$src1, CPURegs:$src2),
                      "sub $dst,$src1,$src2",
                      [(set CPURegs:$dst, (sub limm32:$src1, CPURegs:$src2))]>;

// Carry-producing sub.
// TODO: Fold into a multi-class.
let Defs = [STATUS32] in {
  def SUBrr_f : Pseudo<(outs CPURegs:$dst), (ins CPURegs:$src1, CPURegs:$src2),
                       "sub.f $dst,$src1,$src2",
                       [(set CPURegs:$dst,
                            (subc CPURegs:$src1, CPURegs:$src2))]>;
}

// SUB1 - Page 314.
//    Subtracts the second source operand bit-shifted left once (src2 << 1)
//    from the first source operand, and places the result in the destination
//    register.

defm SUB1 : ALUOp<"sub1", BinOpFrag<(sub node:$LHS, (shl node:$RHS, 1))>>;

// SUB2 - Page 314.
//    Subtracts the second source operand bit-shifted left twice (src2 << 2)
//    from the first source operand, and places the result in the destination
//    register.

defm SUB2 : ALUOp<"sub2", BinOpFrag<(sub node:$LHS, (shl node:$RHS, 2))>>;

// SUB3 - Page 314.
//    Subtracts the second source operand bit-shifted left three bits
//    (src2 << 3) from the first source operand, and places the result in
//    the destination register.

defm SUB3 : ALUOp<"sub3", BinOpFrag<(sub node:$LHS, (shl node:$RHS, 3))>>;

// XOR - page 332.
//    Takes the logical bitwise XOR of the source operands, and places the
//    result into the destination register.

defm XOR : ALUOp<"xor", BinOpFrag<(xor node:$LHS, node:$RHS)>>;

//===----------------------------------------------------------------------===//
// ARCompact Non-Instruction Patterns.
//===----------------------------------------------------------------------===//

// GlobalAddress patterns.
def : Pat<(i32 (ARCWrapper tglobaladdr:$dst)), (MOVrli tglobaladdr:$dst)>;
def : Pat<(add CPURegs:$src1, (ARCWrapper tglobaladdr:$src2)),
          (ADDrli CPURegs:$src1, tglobaladdr:$src2)>;

// Calls.
def : Pat<(ARCcall (i32 tglobaladdr:$dst)), (BLi tglobaladdr:$dst)>;

// EXTB is modelled by llvm as 'and $src, 255'.
// The second two of these definitions are unlikely ever to be seen (LLVM just
// turns them into constant movs), but they are included for completeness.
def : Pat<(and CPURegs:$src, 255), (EXTBr CPURegs:$src)>;
def : Pat<(and uimm6:$src, 255), (EXTBui uimm6:$src)>;
def : Pat<(and limm32:$src, 255), (EXTBli limm32:$src)>;

// EXTW is modelled by llvm as 'and $src, 65535'.
// The second two of these definitions are unlikely ever to be seen (LLVM just
// turns them into constant movs), but they are included for completeness.
def : Pat<(and CPURegs:$src, 65535), (EXTWr CPURegs:$src)>;
def : Pat<(and uimm6:$src, 65535), (EXTWui uimm6:$src)>;
def : Pat<(and limm32:$src, 65535), (EXTWli limm32:$src)>;

// Integer extloads are mapped to to zextloads.
def : Pat<(i32 (extloadi8 ADDRri:$src)), (LDri_extb ADDRri:$src)>;
def : Pat<(i32 (extloadi16 ADDRri:$src)), (LDri_extw ADDRri:$src)>;
