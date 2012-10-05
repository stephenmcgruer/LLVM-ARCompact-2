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

// Largely taken from MSP430.

#include "ARCompactMCInstLower.h"

#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Target/Mangler.h"

using namespace llvm;

void ARCompactMCInstLower::Lower(const MachineInstr *MI, MCInst &OutMI) const {
  OutMI.setOpcode(MI->getOpcode());

  for (unsigned i = 0, e = MI->getNumOperands(); i != e; ++i) {
    const MachineOperand &MO = MI->getOperand(i);

    MCOperand MCOp;
    switch (MO.getType()) {
      // Taken from MSP430.
      case MachineOperand::MO_Register:
        // Ignore implicit register operands.
        if (MO.isImplicit()) {
          continue;
        }

        MCOp = MCOperand::CreateReg(MO.getReg());
        break;
      case MachineOperand::MO_Immediate:
        MCOp = MCOperand::CreateImm(MO.getImm());
        break;
      case MachineOperand::MO_MachineBasicBlock:
        MCOp = MCOperand::CreateExpr(MCSymbolRefExpr::Create(
            MO.getMBB()->getSymbol(), Ctx));
        break;
      case MachineOperand::MO_GlobalAddress:
        MCOp = LowerSymbolOperand(MO, GetGlobalAddressSymbol(MO));
        break;
      case MachineOperand::MO_ExternalSymbol:
        MCOp = LowerSymbolOperand(MO, GetExternalSymbolSymbol(MO));
        break;
      case MachineOperand::MO_BlockAddress:
        MCOp = LowerSymbolOperand(MO, GetBlockAddressSymbol(MO));
        break;
      case MachineOperand::MO_RegisterMask:
        continue;
      default:
        MI->dump();
        llvm_unreachable("unknown operand type");
    }

    OutMI.addOperand(MCOp);
  }
}


MCSymbol *ARCompactMCInstLower::GetGlobalAddressSymbol(
    const MachineOperand &MO) const {
  switch (MO.getTargetFlags()) {
    case 0:
      break;
    default:
      llvm_unreachable("Unknown target flag on GV operand");
  }

  return Printer.Mang->getSymbol(MO.getGlobal());
}

MCSymbol *ARCompactMCInstLower::GetExternalSymbolSymbol(
    const MachineOperand &MO) const {
  switch (MO.getTargetFlags()) {
    case 0:
      break;
    default:
      llvm_unreachable("Unknown target flag on GV operand");
  }

  return Printer.GetExternalSymbolSymbol(MO.getSymbolName());
}

MCSymbol *ARCompactMCInstLower::GetBlockAddressSymbol(
    const MachineOperand &MO) const {
  switch (MO.getTargetFlags()) {
    case 0:
      break;
    default:
      llvm_unreachable("Unknown target flag on GV operand");
  }

  return Printer.GetBlockAddressSymbol(MO.getBlockAddress());
}

MCOperand ARCompactMCInstLower::LowerSymbolOperand(const MachineOperand &MO,
    MCSymbol *Sym) const {
  // FIXME: We would like an efficient form for this, so we don't have to do a
  // lot of extra uniquing.
  const MCExpr *Expr = MCSymbolRefExpr::Create(Sym, Ctx);

  switch (MO.getTargetFlags()) {
    case 0: 
      break;
    default:
      llvm_unreachable("Unknown target flag on GV operand");
  }

  if (!MO.isJTI() && MO.getOffset())
    Expr = MCBinaryExpr::CreateAdd(Expr,
                                   MCConstantExpr::Create(MO.getOffset(), Ctx),
                                   Ctx);
  return MCOperand::CreateExpr(Expr);
}