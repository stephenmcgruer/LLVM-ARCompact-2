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
    llvm_unreachable("Unknown operand in ARCompactInstPrinter::printOperand!");
  }
}