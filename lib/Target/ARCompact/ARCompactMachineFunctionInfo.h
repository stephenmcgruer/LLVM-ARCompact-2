//===- ARCompactMachineFuctionInfo.h - ARCompact machine function info -*- C++ -*-==//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file declares ARCompact-specific per-machine-function information.
//
//===----------------------------------------------------------------------===//

#ifndef ARCompactMACHINEFUNCTIONINFO_H
#define ARCompactMACHINEFUNCTIONINFO_H

#include "llvm/CodeGen/MachineFunction.h"

namespace llvm {

/// ARCompactMachineFunctionInfo - This class is derived from MachineFunction and
/// contains private ARCompact target-specific information for each MachineFunction.
class ARCompactMachineFunctionInfo : public MachineFunctionInfo {
  virtual void anchor();

  /// VarArgsRegSaveSize - Size of the register save area for vararg functions.
  unsigned VarArgsRegSaveSize;

  /// VarArgsFrameIndex - FrameIndex for start of varargs area.
  int VarArgsFrameIndex;

public:
  ARCompactMachineFunctionInfo()
      : VarArgsRegSaveSize(0),
        VarArgsFrameIndex(0) {
  }

  explicit ARCompactMachineFunctionInfo(MachineFunction &MF)
      : VarArgsRegSaveSize(0),
        VarArgsFrameIndex(0) {
  }

  unsigned getVarArgsRegSaveSize() const { return VarArgsRegSaveSize; }
  void setVarArgsRegSaveSize(unsigned s) { VarArgsRegSaveSize = s; }

  int getVarArgsFrameIndex() const { return VarArgsFrameIndex; }
  void setVarArgsFrameIndex(int Index) { VarArgsFrameIndex = Index; }
};

} // End llvm namespace

#endif
