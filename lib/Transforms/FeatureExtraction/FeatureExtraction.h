//===- FeatureExtraction.h ------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// LLVM pass for extracting features.
//
//===----------------------------------------------------------------------===//

#ifndef FEATURE_EXTRACTION_H_
#define FEATURE_EXTRACTION_H_

#include "llvm/Analysis/LoopPass.h"
#include "llvm/Pass.h"
#include "llvm/Constants.h"
#include "llvm/Function.h"
#include "llvm/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/Statistic.h"

#include <set>
#include <vector>

using namespace llvm;

namespace {

  struct LoopStruct {
    // Struct that holds information about a loop, most from Agakov et al (2006).
    //
    // Tracks:
    //   * Is the loop simple?
    //   * Is the loop nested?
    //   * Is the loop perfectly nested?
    //   * Does the loop have a constant lower bound?
    //   * Does the loop have a constant upper bound?
    //   * Does the loop have a constant stride?
    //   * Does the loop have a unit stride?
    //   * The loop nest depth.
    //   * The number of (unique) array references in the loop.
    //   * The number of instructions in the loop.
    //   * The number of load instructions in the loop.
    //   * The number of store instructions in the loop.
    //   * The number of compare instructions in the loop.
    //   * The number of branch instructions in the loop.
    //   * The number of divide instructions in the loop.
    //   * The number of call instructions in the loop.
    //   * The number of generic instructions in the loop.
    //   * The number of array instructions in the loop.
    //   * The number of memory copy instructions in the loop.
    //   * The number of other instructions in the loop.
    //   * Does the loop contain an if-construct?
    //   * Does the loop contain an if-statement in a for-construct?
    //   * Is the loop iterator an array index?
    //   * Are all array indices constants?
    //   * Is any array accessed in a non-linear manner?
    //   * Are the loop strides on leading array dimensions only?
    //   * Does the loop have calls?
    //   * Does the loop have branches?
    //   * Does the loop have regular control flow?

    Loop* TheLoop;
    Instruction* IteratorVariable;
    std::set<StringRef> ReferencedArrays;

    bool IsSimple;
    bool IsNested;
    bool IsPerfectlyNested;
    bool HasConstantLowerBound;
    bool HasConstantUpperBound;
    bool HasConstantStride;
    bool HasUnitStride;

    unsigned NestDepth;
    unsigned NumberArrayReferences;
    unsigned NumberInstructions;
    unsigned NumberLoads;
    unsigned NumberStores;
    unsigned NumberCompares;
    unsigned NumberBranches;
    unsigned NumberDivides;
    unsigned NumberCalls;
    unsigned NumberGenericInstructions;
    unsigned NumberArrayInstructions;
    unsigned NumberMemoryCopies;
    int NumberOtherInstructions;

    bool ContainsIfStatement;
    bool ContainsIfInForStatement;
    bool LoopIteratorIsArrayIndex;
    bool AllArrayIndicesConstant;
    bool HasNonLinearArrayAccess;
    bool StridesOnlyOnLeadingDimensions;
    bool HasCalls;
    bool HasBranches;
    bool HasRegularControlFlow;


    LoopStruct(Loop* ALoop) {
      TheLoop = ALoop;
      IteratorVariable = NULL;

      // Default values:

      IsSimple = false;
      IsNested = false;
      IsPerfectlyNested = false;
      HasConstantLowerBound = false;
      HasConstantUpperBound = false;
      HasConstantStride = false;
      HasUnitStride = false;

      NestDepth = 0;
      NumberArrayReferences = 0;
      NumberInstructions = 0;
      NumberLoads = 0;
      NumberStores = 0;
      NumberCompares = 0;
      NumberBranches = 0;
      NumberDivides = 0;
      NumberCalls = 0;
      NumberGenericInstructions = 0;
      NumberArrayInstructions = 0;
      NumberMemoryCopies = 0;
      NumberOtherInstructions = 0;

      ContainsIfStatement = false;
      ContainsIfInForStatement = false;
      LoopIteratorIsArrayIndex = false;
      AllArrayIndicesConstant = true; // Turns false if non-constant index found.
      HasNonLinearArrayAccess = false;
      StridesOnlyOnLeadingDimensions = true; // Turns false if other case found.
      HasCalls = false;
      HasBranches = false;
      HasRegularControlFlow = false;
    }
  };

  class FunctionFeatureExtraction : public FunctionPass {
  public:
    static char ID; // Pass identification, replacement for typeid
    FunctionFeatureExtraction() 
        : FunctionPass(ID), FunctionCount(0), NumberFloats(0),
          NumberIntegers(0) {}

    virtual bool runOnFunction(Function &F);
    virtual bool doFinalization(Module &M);

  private:
    int FunctionCount;
    int NumberFloats;
    int NumberIntegers;
  };

  class LoopFeatureExtraction : public LoopPass {
  public:
    static char ID; // Pass identification, replacement for typeid
    LoopFeatureExtraction() : LoopPass(ID) {}
    ~LoopFeatureExtraction();

    virtual bool runOnLoop(Loop *L, LPPassManager &LPM);
    virtual bool doFinalization();
    virtual void getAnalysisUsage(AnalysisUsage &AU) const;

  private:
    std::vector<LoopStruct*> LoopStructs;

    bool IsLinearMult(Value* V);
    Instruction* GetIteratorVariable(Loop* L);
    void ParseLoopBounds(LoopStruct* LS, Loop* L, LoopInfo& LI);
    void PostProcessLoops();
    bool IfInForStatement(LoopStruct* LS);
    bool IsPerfectlyNested(LoopStruct* LS);
    LoopStruct* findLoopStruct(Loop* L);
  };
}

#endif  // FEATURE_EXTRACTION_H_