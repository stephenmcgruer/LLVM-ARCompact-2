//===-- ARCompactInstPrinter.cpp - Convert ARCompact MCInst to assembly syntax --===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This class prints an ARCompact MCInst to a .s file.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "asm-printer"
#include "ARCompact.h"
#include "ARCompactInstPrinter.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/Debug.h"
using namespace llvm;


// Include the auto-generated portion of the assembly writer.
#include "ARCompactGenAsmWriter.inc"

void ARCompactInstPrinter::printInst(const MCInst *MI, raw_ostream &O,
                                  StringRef Annot) {
  printInstruction(MI, O);
  printAnnotation(O, Annot);
}

void ARCompactInstPrinter::printOperand(const MCInst *MI, unsigned OpNo,
    raw_ostream &O, const char *Modifier) {
  assert((Modifier == 0 || Modifier[0] == 0) && "No modifiers supported");

  const MCOperand &Op = MI->getOperand(OpNo);
  if (Op.isReg()) {
    O << StringRef(getRegisterName(Op.getReg())).lower();
  } else if (Op.isImm()) {
    O << Op.getImm();
  } else {
    assert(Op.isExpr() && "unknown operand kind in printOperand");
    O << *Op.getExpr();
  }
}

void ARCompactInstPrinter::printMemOperand(const MCInst *MI, unsigned OpNo,
    raw_ostream &O) {
  O << "[";
  printOperand(MI, OpNo, O);

  if (MI->getNumOperands() > 1) {
    MCOperand MO = MI->getOperand(OpNo + 1);
    // Only print the second part if it is non-zero, i.e. never print [r1, 0]!
    if (!MO.isImm() || MO.getImm() != 0) {
      O << ",";
      printOperand(MI, OpNo + 1, O);
    }
  }

  O << "]";
}