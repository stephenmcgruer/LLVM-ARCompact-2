//===-- ARCompactMCInstLower.cpp - Convert ARCompact MachineInstr to an MCInst --===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains code to lower ARCompact MachineInstrs to their corresponding
// MCInst records.
//
//===----------------------------------------------------------------------===//

#include "ARCompactMCInstLower.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCContext.h"
#include "llvm/Support/ErrorHandling.h"
using namespace llvm;

void ARCompactMCInstLower::Lower(const MachineInstr *MI, MCInst &OutMI) const {
  OutMI.setOpcode(MI->getOpcode());

  for (unsigned i = 0, e = MI->getNumOperands(); i != e; ++i) {
    const MachineOperand &MO = MI->getOperand(i);

    MCOperand MCOp;
    switch (MO.getType()) {
      default:
        MI->dump();
        llvm_unreachable("Unknown operand type!");
    }

    OutMI.addOperand(MCOp);
  }
}
