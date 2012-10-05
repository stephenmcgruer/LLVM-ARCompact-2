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
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/Support/TargetRegistry.h"
using namespace llvm;

namespace {
  class ARCompactAsmPrinter : public AsmPrinter {
  public:
    ARCompactAsmPrinter(TargetMachine &TM, MCStreamer &Streamer)
        : AsmPrinter(TM, Streamer) {}

    virtual const char *getPassName() const {
      return "ARCompact Assembly Printer";
    }

    //void printOperand(const MachineInstr *MI, int OpNum,
    //                  raw_ostream &O, const char* Modifier = 0);
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


//void ARCompactAsmPrinter::printOperand(const MachineInstr *MI, int OpNum,
//                                    raw_ostream &O, const char *Modifier) {
//  const MachineOperand &MO = MI->getOperand(OpNum);
//  switch (MO.getType()) {
//  default: llvm_unreachable("Not implemented yet!");
//  case MachineOperand::MO_Register:
//    O << ARCompactInstPrinter::getRegisterName(MO.getReg());
//    return;
//  case MachineOperand::MO_Immediate:
//    if (!Modifier || strcmp(Modifier, "nohash"))
//      O << '#';
//    O << MO.getImm();
//    return;
//  case MachineOperand::MO_MachineBasicBlock:
//    O << *MO.getMBB()->getSymbol();
//    return;
//  case MachineOperand::MO_GlobalAddress: {
//    bool isMemOp  = Modifier && !strcmp(Modifier, "mem");
//    uint64_t Offset = MO.getOffset();
//
//    // If the global address expression is a part of displacement field with a
//    // register base, we should not emit any prefix symbol here, e.g.
//    //   mov.w &foo, r1
//    // vs
//    //   mov.w glb(r1), r2
//    // Otherwise (!) msp430-as will silently miscompile the output :(
//    if (!Modifier || strcmp(Modifier, "nohash"))
//      O << (isMemOp ? '&' : '#');
//    if (Offset)
//      O << '(' << Offset << '+';
//
//    O << *Mang->getSymbol(MO.getGlobal());
//
//    if (Offset)
//      O << ')';
//
//    return;
//  }
//  case MachineOperand::MO_ExternalSymbol: {
//    bool isMemOp  = Modifier && !strcmp(Modifier, "mem");
//    O << (isMemOp ? '&' : '#');
//    O << MAI->getGlobalPrefix() << MO.getSymbolName();
//    return;
//  }
//  }
//}

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
