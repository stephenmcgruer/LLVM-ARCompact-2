//===-- ARCompactMCTargetDesc.cpp - ARCompact Target Descriptions ---------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file provides ARCompact specific target descriptions.
//
//===----------------------------------------------------------------------===//

#include "ARCompactMCTargetDesc.h"
#include "ARCompactMCAsmInfo.h"
#include "InstPrinter/ARCompactInstPrinter.h"
#include "llvm/MC/MCCodeGenInfo.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/TargetRegistry.h"

#define GET_INSTRINFO_MC_DESC
#include "ARCompactGenInstrInfo.inc"

#define GET_SUBTARGETINFO_MC_DESC
#include "ARCompactGenSubtargetInfo.inc"

#define GET_REGINFO_MC_DESC
#include "ARCompactGenRegisterInfo.inc"

using namespace llvm;

static MCInstrInfo *createARCompactMCInstrInfo() {
  MCInstrInfo *X = new MCInstrInfo();
  InitARCompactMCInstrInfo(X);
  return X;
}

static MCRegisterInfo *createARCompactMCRegisterInfo(StringRef TT) {
  MCRegisterInfo *X = new MCRegisterInfo();
  InitARCompactMCRegisterInfo(X, ARC::BLINK);
  return X;
}

static MCSubtargetInfo *createARCompactMCSubtargetInfo(StringRef TT, StringRef CPU,
                                                    StringRef FS) {
  MCSubtargetInfo *X = new MCSubtargetInfo();
  InitARCompactMCSubtargetInfo(X, TT, CPU, FS);
  return X;
}

static MCCodeGenInfo *createARCompactMCCodeGenInfo(StringRef TT, Reloc::Model RM,
                                                CodeModel::Model CM,
                                                CodeGenOpt::Level OL) {
  MCCodeGenInfo *X = new MCCodeGenInfo();
  X->InitMCCodeGenInfo(RM, CM, OL);
  return X;
}

static MCInstPrinter *createARCompactMCInstPrinter(const Target &T,
                                                unsigned SyntaxVariant,
                                                const MCAsmInfo &MAI,
                                                const MCInstrInfo &MII,
                                                const MCRegisterInfo &MRI,
                                                const MCSubtargetInfo &STI) {
  if (SyntaxVariant == 0)
    return new ARCompactInstPrinter(MAI, MII, MRI);
  return 0;
}

extern "C" void LLVMInitializeARCompactTargetMC() {
  // Register the MC asm info.
  RegisterMCAsmInfo<ARCompactMCAsmInfo> X(TheARCompactTarget);

  // Register the MC codegen info.
  TargetRegistry::RegisterMCCodeGenInfo(TheARCompactTarget,
                                        createARCompactMCCodeGenInfo);

  // Register the MC instruction info.
  TargetRegistry::RegisterMCInstrInfo(TheARCompactTarget, createARCompactMCInstrInfo);

  // Register the MC register info.
  TargetRegistry::RegisterMCRegInfo(TheARCompactTarget,
                                    createARCompactMCRegisterInfo);

  // Register the MC subtarget info.
  TargetRegistry::RegisterMCSubtargetInfo(TheARCompactTarget,
                                          createARCompactMCSubtargetInfo);

  // Register the MCInstPrinter.
  TargetRegistry::RegisterMCInstPrinter(TheARCompactTarget,
                                        createARCompactMCInstPrinter);
}
