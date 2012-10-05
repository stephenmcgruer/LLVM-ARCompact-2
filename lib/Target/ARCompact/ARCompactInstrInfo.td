//===-- ARCompactInstrInfo.td - ARCompact Instruction defs -------*- tablegen -*-===//
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
// ARCompact-specific node definitions.
//===----------------------------------------------------------------------===//

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
  def rr : Pseudo<(outs CPURegs:$dst),
                  (ins CPURegs:$src1, CPURegs:$src2),
                  !strconcat(opstring, " $dst,$src1,$src2"),
                  [(set CPURegs:$dst, (OpNode CPURegs:$src1, CPURegs:$src2))]>;

  def rui : Pseudo<(outs CPURegs:$dst),
                   (ins CPURegs:$src1, i32imm:$src2),
                   !strconcat(opstring, " $dst,$src1,$src2"),
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
multiclass ALUOp_no16<string opstring, PatFrag OpNode> {
  def rr : Pseudo<(outs CPURegs:$dst),
                  (ins CPURegs:$src1, CPURegs:$src2),
                  !strconcat(opstring, " $dst,$src1,$src2"),
                  [(set CPURegs:$dst, (OpNode CPURegs:$src1, CPURegs:$src2))]>;

  def rui : Pseudo<(outs CPURegs:$dst),
                   (ins CPURegs:$src1, i32imm:$src2),
                   !strconcat(opstring, " $dst,$src1,$src2"),
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
                   (ins CPURegs:$src1, i32imm:$src2),
                   !strconcat(opstring, " $dst,$src1,$src2"),
                   [(set CPURegs:$dst, (OpNode CPURegs:$src1, limm32:$src2))]>;
}

// Models generic ALU operations that do have a 16-bit version. (See pages
// 90-91).
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

// The manual defines the preferred NOP asm as mov 0,0.
def NOP : Pseudo<(outs), (ins), "mov 0,0", []>;

// A return is modelled as an explicit jump from BLINK.
let isReturn = 1, isTerminator = 1, hasDelaySlot = 1, isBarrier = 1 in {
    def RET : Pseudo<(outs), (ins), "j [blink]", [(ARCretflag)]>;
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

defm ADD : GenPurposeInst<"add", add>;

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

def LDrli : Pseudo<(outs CPURegs:$dst), (ins MEMrli:$addr),
                    "ld $dst,$addr",
                    [(set CPURegs:$dst, (load ADDRrli:$addr))]>;

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

// NEG - page 275.
//    The source value is subtracted from 0 and the result placed in the
//    destination register.
//
//    NEG only has a register-to-register version.

def NEGrr : Pseudo<(outs CPURegs:$dst), (ins CPURegs:$src),
                   "neg $dst,$src",
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
def STrri_a : Pseudo<(outs), (ins MEMri:$addr, CPURegs:$src),
                      "st.a $src,$addr",
                      []>;

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
