//===-- ARCompactAsmPrinter.cpp - ARCompact LLVM assembly writer ----------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains a printer that converts from our internal representation
// of machine-dependent LLVM code to the ARCompact assembly language.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "asm-printer"
#include "ARCompactMCInstLower.h"
#include "ARCompactTargetMachine.h"
#include "InstPrinter/ARCompactInstPrinter.h"

#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/Metadata.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Target/Mangler.h"
using namespace llvm;

namespace {
  class ARCompactAsmPrinter : public AsmPrinter {
  public:
    ARCompactAsmPrinter(TargetMachine &TM, MCStreamer &Streamer)
        : AsmPrinter(TM, Streamer) {}

    virtual const char *getPassName() const {
      return "ARCompact Assembly Printer";
    }

    void printOperand(const MachineInstr *MI, int OpNum,
        raw_ostream &O, const char* Modifier = 0);
    //void printSrcMemOperand(const MachineInstr *MI, int OpNum,
    //                        raw_ostream &O);
    //bool PrintAsmOperand(const MachineInstr *MI, unsigned OpNo,
    //                     unsigned AsmVariant, const char *ExtraCode,
    //                     raw_ostream &O);
    //bool PrintAsmMemoryOperand(const MachineInstr *MI,
    //                           unsigned OpNo, unsigned AsmVariant,
    //                           const char *ExtraCode, raw_ostream &O);
    void EmitInstruction(const MachineInstr *MI);
  };
} // end of anonymous namespace


void ARCompactAsmPrinter::printOperand(const MachineInstr *MI, int OpNum,
    raw_ostream &O, const char *Modifier) {
  const MachineOperand &MO = MI->getOperand(OpNum);
  switch (MO.getType()) {
    // A register.
    case MachineOperand::MO_Register:
      O << StringRef(
          ARCompactInstPrinter::getRegisterName(MO.getReg())).lower();
      return;

    // An immediate.
    case MachineOperand::MO_Immediate:
      O << MO.getImm();
      return;

    // An entire basic block?
    case MachineOperand::MO_MachineBasicBlock:
      O << *MO.getMBB()->getSymbol();
      return;

    // A global address.
    case MachineOperand::MO_GlobalAddress:
      O << "@" << *Mang->getSymbol(MO.getGlobal());

      // Deal with any offsets.
      if (MO.getOffset() > 0) {
        O << "+" << MO.getOffset();
      } else if (MO.getOffset() < 0) {
        O << "-" << MO.getOffset();
      }

      return;

    // An external symbol.
    case MachineOperand::MO_ExternalSymbol:
      O << "@" << MO.getSymbolName();
      return;

    // A comment.
    case MachineOperand::MO_Metadata:
      // EOL comments need to be prefixed with a space.
      if (MI->getNumOperands() > 1) {
        O << " ";
      }
      O << "; " << cast<MDString>(MO.getMetadata()->getOperand(0))->getString();
      return;

    default:
      llvm_unreachable("Unknown operand type!");
  }
}

//void ARCompactAsmPrinter::printSrcMemOperand(const MachineInstr *MI, int OpNum,
//                                          raw_ostream &O) {
//  const MachineOperand &Base = MI->getOperand(OpNum);
//  const MachineOperand &Disp = MI->getOperand(OpNum+1);
//
//  // Print displacement first
//
//  // Imm here is in fact global address - print extra modifier.
//  if (Disp.isImm() && !Base.getReg())
//    O << '&';
//  printOperand(MI, OpNum+1, O, "nohash");
//
//  // Print register base field
//  if (Base.getReg()) {
//    O << '(';
//    printOperand(MI, OpNum, O);
//    O << ')';
//  }
//}
//
///// PrintAsmOperand - Print out an operand for an inline asm expression.
/////
//bool ARCompactAsmPrinter::PrintAsmOperand(const MachineInstr *MI, unsigned OpNo,
//                                       unsigned AsmVariant,
//                                       const char *ExtraCode, raw_ostream &O) {
//  // Does this asm operand have a single letter operand modifier?
//  if (ExtraCode && ExtraCode[0])
//    return true; // Unknown modifier.
//
//  printOperand(MI, OpNo, O);
//  return false;
//}
//
//bool ARCompactAsmPrinter::PrintAsmMemoryOperand(const MachineInstr *MI,
//                                             unsigned OpNo, unsigned AsmVariant,
//                                             const char *ExtraCode,
//                                             raw_ostream &O) {
//  if (ExtraCode && ExtraCode[0]) {
//    return true; // Unknown modifier.
//  }
//  printSrcMemOperand(MI, OpNo, O);
//  return false;
//}
//
//===----------------------------------------------------------------------===//
void ARCompactAsmPrinter::EmitInstruction(const MachineInstr *MI) {
  ARCompactMCInstLower MCInstLowering(OutContext, *this);

  MCInst TmpInst;
  MCInstLowering.Lower(MI, TmpInst);
  OutStreamer.EmitInstruction(TmpInst);
}

// Force static initialization.
extern "C" void LLVMInitializeARCompactAsmPrinter() {
  RegisterAsmPrinter<ARCompactAsmPrinter> X(TheARCompactTarget);
}
