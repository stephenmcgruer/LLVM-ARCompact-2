//===-- ARCompactSelectionDAGInfo.h - ARCompact SelectionDAG Info -----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the ARCompact subclass for TargetSelectionDAGInfo.
//
//===----------------------------------------------------------------------===//

#ifndef ARCompactSELECTIONDAGINFO_H
#define ARCompactSELECTIONDAGINFO_H

#include "llvm/Target/TargetSelectionDAGInfo.h"

namespace llvm {

class ARCompactTargetMachine;

class ARCompactSelectionDAGInfo : public TargetSelectionDAGInfo {
public:
  explicit ARCompactSelectionDAGInfo(const ARCompactTargetMachine &TM);
  ~ARCompactSelectionDAGInfo();
};

}

#endif
