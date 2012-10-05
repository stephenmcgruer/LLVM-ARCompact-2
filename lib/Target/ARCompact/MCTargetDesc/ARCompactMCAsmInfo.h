//===-- ARCompactMCAsmInfo.h - ARCompact asm properties --------------*- C++ -*--===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the declaration of the ARCompactMCAsmInfo class.
//
//===----------------------------------------------------------------------===//

#ifndef ARCompactTARGETASMINFO_H
#define ARCompactTARGETASMINFO_H

#include "llvm/MC/MCAsmInfo.h"

namespace llvm {
  class StringRef;
  class Target;

  class ARCompactMCAsmInfo : public MCAsmInfo {
    virtual void anchor();
  public:
    explicit ARCompactMCAsmInfo(const Target &T, StringRef TT);
  };

} // namespace llvm

#endif
