//===- FeatureExtraction.cpp ----------------------------------------------===//
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

#include "FeatureExtraction.h"

using namespace llvm;

namespace {

  // =========== FunctionFeatureExtraction ===========

  bool FunctionFeatureExtraction::runOnFunction(Function &F) {
    FunctionCount++;

    // Count the number of allocated floats and integers.
    for (Function::iterator FI = F.begin(), FE = F.end(); FI != FE; ++FI) {
      for (BasicBlock::const_iterator I = FI->begin(), E = FI->end(); I != E;
          ++I) {
        if (const AllocaInst *AI = dyn_cast<AllocaInst>(I)) {
          if (AI->getAllocatedType()->isFloatingPointTy()) {
            NumberFloats++;
          } else if (AI->getAllocatedType()->isIntegerTy()) {
            NumberIntegers++;
          }
        }
      }
    }

    // We do not edit the CFG.
    return false;
  }

  bool FunctionFeatureExtraction::doFinalization(Module &M) {
    bool UsesFloatAndIntegers = NumberIntegers > 0 && NumberFloats > 0;

    // Dump the information.
    errs() << "FunctionCount: " << FunctionCount << "\n";
    errs() << "NumberIntegers: " << NumberIntegers << "\n";
    errs() << "NumberFloats: " << NumberFloats << "\n";
    errs() << "UsesFloatAndIntegerVariables: " << UsesFloatAndIntegers << "\n";

    // We do not edit the CFG.
    return false;
  }

  // =========== LoopFeatureExtraction ===========
  
  LoopFeatureExtraction::~LoopFeatureExtraction() {
    while (!LoopStructs.empty()) {
      LoopStruct* LS = LoopStructs.back();
      delete(LS);
      LS = NULL;
      LoopStructs.pop_back();
    }
  }

  bool LoopFeatureExtraction::runOnLoop(Loop *L, LPPassManager &LPM) {
    LoopInfo &LI = getAnalysis<LoopInfo>();

    LoopStruct* LS = new LoopStruct(L);
    LoopStructs.push_back(LS);

    // Add basic information about the iterator variable, loop bounds,
    // etc.
    LS->IteratorVariable = GetIteratorVariable(L);
    ParseLoopBounds(LS, L, LI);
    LS->IsNested = L->getParentLoop() != NULL;
    LS->NestDepth = L->getLoopDepth();

    // Parse each BasicBlock.
    for (Loop::block_iterator BI = L->block_begin(), BE = L->block_end();
        BI != BE; ++BI) {
      const BasicBlock* BB = *BI;

      if (LI.getLoopFor(BB) != L) {
        // This BasicBlock belongs to a child loop, so ignore it.
        continue;
      }

      runOnBasicBlock(BB, LS, LI);
    }

    // We do not edit the CFG.
    return false;
  }

  void LoopFeatureExtraction::runOnBasicBlock(const BasicBlock* BB,
      LoopStruct* LS, LoopInfo& LI) {
    Loop* L = LS->TheLoop;

    LS->NumberInstructions += BB->size();

    // Parse each instruction in the BasicBlock.
    for (BasicBlock::const_iterator I = BB->begin(), E = BB->end(); I != E;
        ++I) {

      // NumberLoads.
      if (isa<LoadInst>(I)) {
        LS->NumberLoads++;
      }

      // NumberStores, NumberMemoryCopies.
      if (const StoreInst* SI = dyn_cast<StoreInst>(I)) {
        LS->NumberStores++;

        // Check if the source and destination are both memory locations,
        // which equates to copying memory.
        if (isa<LoadInst>(SI->getValueOperand()) && 
            isa<AllocaInst>(SI->getPointerOperand())) {
          LS->NumberMemoryCopies++;
        }
      }

      // NumberCompares.
      if (isa<CmpInst>(I)) {
        LS->NumberCompares++;
      }

      // NumberBranches, ContainsIfStatement.
      if (const BranchInst* BI = dyn_cast<BranchInst>(I)) {
        LS->NumberBranches++;

        // Spotting if-statements is difficult in LLVM IR. If a branch can only
        // result in locations that are in the same loop, it is an if-statement
        // of some kind.
        if (BI->isConditional()) {
          // Initially assume every BranchInst is an if-statement.
          bool IsIfStatement = true;

          // Look for successors who aren't in this loop.
          unsigned int NumberSuccessors = BI->getNumSuccessors();
          for (unsigned int i = 0; i < NumberSuccessors; i++) {
            BasicBlock* Successor = BI->getSuccessor(i);

            if (LI.getLoopFor(Successor) != L) {
              IsIfStatement = false;
            }
          }

          LS->ContainsIfStatement = LS->ContainsIfStatement || IsIfStatement;
        }
      }

      // NumberBranches.
      if (isa<IndirectBrInst>(I)) {
        LS->NumberBranches++;
      }

      // NumberDivides, NumberGenericInstructions.
      if (const BinaryOperator* BO = dyn_cast<BinaryOperator>(I)) {
        if (BO->getOpcode() == Instruction::SDiv ||
            BO->getOpcode() == Instruction::UDiv) {
          LS->NumberDivides++;
        }

        // Check for generic instructions.
        switch (BO->getOpcode()) {
          case Instruction::Add:  // a + b
          case Instruction::Sub:  // a - b
          case Instruction::Mul:  // a * b
          case Instruction::URem: // a % b (unsigned)
          case Instruction::SRem: // a % b (signed)
          case Instruction::Shl:  // a << b
          case Instruction::LShr: // a >> b
          case Instruction::AShr: // a >>> b
          case Instruction::And:  // a & b
          case Instruction::Or:   // a | b
          case Instruction::Xor:  // a ^ b
            LS->NumberGenericInstructions++;
          default:
            // Do nothing.
            break;
        }
      }

      // NumberCalls.
      if (isa<CallInst>(I)) {
        LS->NumberCalls++;
      }

      // NumberArrayInstructions, NumberArrayReferences,
      // AllArrayIndicesConstant, LoopIteratorIsArrayIndex,
      // StridesOnlyOnLeadingDimensions.
      if (const GetElementPtrInst* GEP = dyn_cast<GetElementPtrInst>(I)) {
        const Value* GEPPointer = GEP->getPointerOperand();
        Type* GEPPointerType = GEP->getPointerOperandType();

        // Nicely, clang on -O0 will produce a GEP for *every* array access,
        // even to the same constant location multiple times in a row. This
        // means that every array access is represented by a GEP of type
        // 'Pointer To Array'.
        if (isa<PointerType>(GEPPointerType) &&
            isa<ArrayType>(GEPPointerType->getPointerElementType())) {

          // A GEP is created for each index in a multi-dimensional array.
          // For counting array instructions and references, use the case where
          // the pointer operand is an alloca rather than another GEP to
          // uniquely identify array instructions.
          if (isa<AllocaInst>(GEPPointer)) {
            LS->NumberArrayInstructions++;

            // Check if the array index is constant.
            LS->AllArrayIndicesConstant = 
                LS->AllArrayIndicesConstant && GEP->hasAllConstantIndices();

            // Check if we've seen the array reference before.
            StringRef Name = GEPPointer->getName();
            std::set<StringRef>::iterator I = LS->ReferencedArrays.find(Name);
            if (I == LS->ReferencedArrays.end()) {
              LS->ReferencedArrays.insert(Name);
              LS->NumberArrayReferences++;
            }
          }

          // For all array accesses check if the index is the iterator, and
          // what dimension it strides on. The index is the last element in
          // the GEP idx list.
          GetElementPtrInst::const_op_iterator I = GEP->idx_end();
          --I;

          if (Instruction* Inst = dyn_cast<Instruction>(*I)) {
            while (isa<CastInst>(Inst)) {
              // Work through any casts.
              Inst = dyn_cast<Instruction>(Inst->getOperand(0));
            }

            if (LoadInst* LI = dyn_cast<LoadInst>(Inst)) {
              // The index is a variable.

              if (LI->getPointerOperand() == LS->IteratorVariable) {
                LS->LoopIteratorIsArrayIndex = true;

                // Need to check if we're the outermost access; if not, then we
                // don't only stride on leading dimensions. The outermost
                // access has an element type that is NOT ArrayType.
                ArrayType* AT = dyn_cast<ArrayType>(
                    GEPPointerType->getPointerElementType());
                if (isa<ArrayType>(AT->getElementType())) {
                  // Using the iterator variable as a non-outermost index!
                  LS->StridesOnlyOnLeadingDimensions = false;
                }
              }
            }
          
            // Check for linear array access.
            BinaryOperator* BO = dyn_cast<BinaryOperator>(Inst);
            if (BO && !IsLinearBinaryOperator(BO)) {
              LS->HasNonLinearArrayAccess = true;
            } else if (!isa<LoadInst>(Inst) && !isa<Constant>(Inst)) {
              LS->HasNonLinearArrayAccess = true;
            }
          }
        }
      }
    }
  }

  bool LoopFeatureExtraction::doFinalization() {
    // Finalize the LoopStructs.
    PostProcessLoopStructs();

    // Output the information.
    errs() << "LoopCount: " << LoopStructs.size() << "\n";
    int count = 0;
    for (std::vector<LoopStruct*>::iterator I = LoopStructs.begin(),
        E = LoopStructs.end(); I != E; ++I) {
      LoopStruct* LS = *I;

      llvm::errs() << "Loop " << count << "\n";

      // First set, booleans.
      llvm::errs() << "\tIsSimple " << LS->IsSimple << "\n"
          << "\tIsNested " << LS->IsNested << "\n"
          << "\tIsPerfectlyNested " << LS->IsPerfectlyNested << "\n"
          << "\tHasConstantLowerBound " << LS->HasConstantLowerBound << "\n"
          << "\tHasConstantUpperBound " << LS->HasConstantUpperBound << "\n"
          << "\tHasConstantStride " << LS->HasConstantStride << "\n"
          << "\tHasUnitStride " << LS->HasUnitStride << "\n";

      // Second set, counts.
      llvm::errs() << "\tNestDepth " << LS->NestDepth << "\n"
          << "\tNumberArrayReferences " << LS->NumberArrayReferences << "\n"
          << "\tNumberInstructions " << LS->NumberInstructions << "\n"
          << "\tNumberLoads " << LS->NumberLoads << "\n"
          << "\tNumberStores " << LS->NumberStores << "\n"
          << "\tNumberCompares " << LS->NumberCompares << "\n"
          << "\tNumberBranches " << LS->NumberBranches << "\n"
          << "\tNumberDivides " << LS->NumberDivides << "\n"
          << "\tNumberCalls " << LS->NumberCalls << "\n"
          << "\tNumberGenericInstructions " << LS->NumberGenericInstructions
          << "\n"
          << "\tNumberArrayInstructions " << LS->NumberArrayInstructions
          << "\n"
          << "\tNumberMemoryCopies " << LS->NumberMemoryCopies << "\n"
          << "\tNumberOtherInstructions " << LS->NumberOtherInstructions
          << "\n";

      // Third set, more booleans.
      llvm::errs() << "\tContainsIfStatement " << LS->ContainsIfStatement
          << "\n"
          << "\tContainsIfInForStatement " << LS->ContainsIfInForStatement
          << "\n"
          << "\tLoopIteratorIsArrayIndex " << LS->LoopIteratorIsArrayIndex
          << "\n"
          << "\tAllArrayIndicesConstant " << LS->AllArrayIndicesConstant
          << "\n"
          << "\tHasNonLinearArrayAccess " << LS->HasNonLinearArrayAccess
          << "\n"
          << "\tStridesOnlyOnLeadingDimensions "
          << LS->StridesOnlyOnLeadingDimensions << "\n"
          << "\tHasCalls " << LS->HasCalls << "\n"
          << "\tHasBranches " << LS->HasBranches << "\n"
          << "\tHasRegularControlFlow " << LS->HasRegularControlFlow << "\n";

      count++;
    }

    // We don't change the CFG.
    return false;
  }

  void LoopFeatureExtraction::getAnalysisUsage(AnalysisUsage &AU) const {
    AU.addRequired<LoopInfo>();
    AU.addPreserved<LoopInfo>();
  }

  bool LoopFeatureExtraction::IsLinearBinaryOperator(BinaryOperator* BO) {
    // For simplicity, move any constants to the RHS.
    if (isa<Constant>(BO->getOperand(0))) {
      BO->swapOperands();
    }

    Value* LHS = BO->getOperand(0);
    Value* RHS = BO->getOperand(1);
    
    if (BO->getOpcode() == Instruction::Add ||
        BO->getOpcode() == Instruction::Sub) {

      if (isa<Constant>(RHS)) {
        // To be linear, LHS must now be either const*var or var.
        return isa<LoadInst>(LHS) || IsLinearMult(LHS);
      }

    } else if (BO->getOpcode() == Instruction::Mul) {
      return IsLinearMult(BO);
    }

    return false;
  }

  bool LoopFeatureExtraction::IsLinearMult(Value* V) {
    BinaryOperator* BO = dyn_cast<BinaryOperator>(V);
    if (BO && BO->getOpcode() == Instruction::Mul) {
      // For simplicity, move any constants to the RHS.
      if (isa<Constant>(BO->getOperand(0))) {
        BO->swapOperands();
      }

      // A linear multiplication is defined as var*constant.
      return isa<LoadInst>(BO->getOperand(0)) &&
          isa<Constant>(BO->getOperand(1));
    }

    return false;
  }

  Instruction* LoopFeatureExtraction::GetIteratorVariable(Loop* L) {
    // The iterator variable is usually the first load of the loop header.
    BasicBlock* Header = L->getHeader();
    if (Header) {
      Instruction* FirstInst = Header->getFirstNonPHI();
      if (LoadInst* LI = dyn_cast<LoadInst>(FirstInst)) {
        if (AllocaInst* AI = dyn_cast<AllocaInst>(LI->getPointerOperand())) {
          return AI;
        }
      }
    }

    return NULL;
  }

  void LoopFeatureExtraction::ParseLoopBounds(LoopStruct* LS, Loop* L, LoopInfo& LI) {
    BasicBlock* Preheader = L->getLoopPreheader();
    BasicBlock* Header = L->getHeader();

    // The lower bound can assumed to be the last store instruction to
    // the loop variable before the loop start.
    if (Preheader) {
      BasicBlock::iterator I = Preheader->end();
      while (!isa<StoreInst>(*I) && I != Preheader->begin()) {
        --I;
      }

      // Check if a constant is stored to the loop variable.
      StoreInst* SI = dyn_cast<StoreInst>(I);
      if (SI && SI->getPointerOperand() == LS->IteratorVariable &&
          isa<Constant>(SI->getValueOperand())) {
        LS->HasConstantLowerBound = true;
      }
    }

    // The upper bound can be assumed to be the last compare instruction in
    // the loop header.
    if (Header) {
      BasicBlock::iterator I = Header->end();
      while (!isa<CmpInst>(*I) && I != Header->begin()) {
        --I;
      }

      // Check if the compare is with a constant.
      if (CmpInst* CI = dyn_cast<CmpInst>(I)) {
        // Move any constant to RHS for simplicity.
        if (isa<Constant>(CI->getOperand(0))) {
          CI->swapOperands();
        }

        Value* RHS = CI->getOperand(1);
        if (isa<Constant>(RHS)) {
          LS->HasConstantUpperBound = true;
        }
      }
    }

    // The stride is found in the increment section. Best way to get
    // there is from the header, although it's not pretty.
    if (Header) {
      // Check the Header's predecessors for the Inc block.
      for (pred_iterator PI = pred_begin(Header), E = pred_end(Header);
          PI != E; PI++) {
        // Ignore the Preheader.
        if (*PI == Preheader) {
          continue;
        }

        // To be sure we have the Inc block, check that it has one successor;
        // the Header.
        succ_iterator I = succ_begin(*PI), E = succ_end(*PI);
        --E; // Move to one before the end of the list.
        if (I == E && *I == Header) {
          BasicBlock* Inc = *PI;

          // The increment can usually be found by locating the first load
          // instruction and looking just past it. This should catch x++,
          // x += const, and x = x + const. (As well as the minus versions.)

          // Locate the first load.
          BasicBlock::iterator II = Inc->begin();
          while (!isa<PHINode>(II) && !isa<LoadInst>(II) && II != Inc->end()) {
            ++II;
          }

          LoadInst* LI = dyn_cast<LoadInst>(II);
          if (LI->getPointerOperand() == LS->IteratorVariable) {
            // The next instruction should be an increment/decrement.
            ++II;

            if (BinaryOperator* BO = dyn_cast<BinaryOperator>(II)) {
              Value* LHS = BO->getOperand(0);
              Value* RHS = BO->getOperand(1);
              Instruction::BinaryOps Opcode = BO->getOpcode();

              ConstantInt* CI = dyn_cast<ConstantInt>(RHS);
              if (CI && LHS == LI &&
                  (Opcode == Instruction::Add || Opcode == Instruction::Sub)) {
                LS->HasConstantStride = true;
                LS->HasUnitStride = CI->isOne();
                break;
              }
            }
          }
        }
      }
    }

  }

  void LoopFeatureExtraction::PostProcessLoopStructs() {
    for (std::vector<LoopStruct*>::iterator I = LoopStructs.begin(),
        E = LoopStructs.end(); I != E; ++I) {
      LoopStruct* LS = *I;

      LS->NumberOtherInstructions = LS->NumberInstructions
          - LS->NumberArrayReferences
          - LS->NumberLoads
          - LS->NumberStores
          - LS->NumberCompares
          - LS->NumberBranches
          - LS->NumberDivides
          - LS->NumberCalls
          - LS->NumberGenericInstructions
          - LS->NumberArrayInstructions
          - LS->NumberMemoryCopies;
      if (LS->NumberOtherInstructions < 0) {
        LS->NumberOtherInstructions = 0;
      }

      LS->HasCalls = LS->NumberCalls > 0;

      // A 'no branch' loop has 3 branches: cond -> {body, end}, body -> inc,
      // inc -> cond.
      LS->HasBranches = LS->NumberBranches > 3;

      // TODO: This is a lazy hack and not representative.
      LS->HasRegularControlFlow = !LS->HasBranches;

      // IsSimple: Not nested, has no function calls.
      // TODO: Again probably not right.
      LS->IsSimple = !LS->IsNested && !LS->HasCalls;

      LS->ContainsIfInForStatement = IfInForStatement(LS);

      LS->IsPerfectlyNested = IsPerfectlyNested(LS);
    }
  }

  bool LoopFeatureExtraction::IfInForStatement(LoopStruct* LS) {
    Loop* L = LS->TheLoop;

    for (Loop::iterator I = L->begin(), E = L->end(); I != E; I++) {
      Loop* Child = *I;
      LoopStruct* ChildLS = findLoopStruct(Child);
      if (!ChildLS) {
        errs() << "PROBLEM: findLoopStruct failed!\n";
      }

      if (ChildLS->ContainsIfStatement ||
          IfInForStatement(ChildLS)) {
        return true;
      }
    }

    return false;
  }

  bool LoopFeatureExtraction::IsPerfectlyNested(LoopStruct* LS) {
    Loop* L = LS->TheLoop;

    if ((L->end() - L->begin()) == 0) {
      // A loop with no children is perfect if it doesnt have any
      // conditional statements or jumps.

      return !LS->HasCalls && !LS->HasBranches && !LS->ContainsIfStatement;
    } else {
      // Otherwise, we require that we have only one, perfectly nested child,
      // and no other instructions.
      if ((L->end() - L->begin()) != 1) {
        return false;
      }

      Loop* Child = *L->begin();
      LoopStruct* ChildLS = findLoopStruct(Child);

      // A perfectly nested loop with child has 4 branches: one from the
      // condition to either the body or the exit, one from the increment
      // step back to the condition, one from the end of the child loop to
      // the increment, and one from the body to the child loop's condition.
      bool CorrectNumberBranches = LS->NumberBranches == 4;

      // A perfectly nested loop with child has two loads; one in the
      // condition and one in the increment.
      // TODO: Not true if loop doesnt take form:
      //      'if (var = const; var op const2; var++)'
      bool CorrectNumberLoads = LS->NumberLoads == 2;

      // A perfectly nested loop with child has two stores; one in the
      // body (storing the variable for the child), and one in the increment.
      // TODO: Not true if loop doesnt take form:
      //      'if (var = const; var op const2; var++)'
      bool CorrectNumberStores = LS->NumberStores == 2;

      // A perfectly nested loop with child has one compare, done in
      // the condition.
      bool CorrectNumberCompares = LS->NumberCompares == 1;

      if (!CorrectNumberBranches ||
          !CorrectNumberLoads ||
          !CorrectNumberStores ||
          !CorrectNumberCompares) {
        return false;
      }

      return IsPerfectlyNested(ChildLS);
    }
  }

  LoopStruct* LoopFeatureExtraction::findLoopStruct(Loop* L) {
    for (std::vector<LoopStruct*>::iterator I = LoopStructs.begin(),
        E = LoopStructs.end(); I != E; ++I) {
      LoopStruct* LS = *I;
      if (LS->TheLoop == L) {
        return LS;
      }
    }

    return NULL;
  }
}  // End anon namespace.

char FunctionFeatureExtraction::ID = 0;
static RegisterPass<FunctionFeatureExtraction> X("print-function-features",
    "Function Feature Extraction");

char LoopFeatureExtraction::ID = 0;
static RegisterPass<LoopFeatureExtraction> Y("print-loop-features", 
    "Loop Feature Extraction");