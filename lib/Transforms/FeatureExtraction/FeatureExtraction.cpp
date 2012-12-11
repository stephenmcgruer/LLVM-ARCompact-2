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

  bool FunctionFeatureExtraction::runOnFunction(Function &F) {
    FunctionCount++;
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
    return false;
  }

  bool FunctionFeatureExtraction::doFinalization(Module &M) {
    errs() << "FunctionCount: " << FunctionCount << "\n";
    errs() << "NumberIntegers: " << NumberIntegers << "\n";
    errs() << "NumberFloats: " << NumberFloats << "\n";
    errs() << "UsesFloatAndIntegerVariables: ";
    if (NumberIntegers > 0 && NumberFloats > 0) {
      errs() << "1\n";
    } else {
      errs() << "0\n";
    }
    return false;
  }
  
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

    LS->IteratorVariable = GetIteratorVariable(L);
    ParseLoopBounds(LS, L, LI);
    LS->IsNested = L->getParentLoop() != NULL;
    LS->NestDepth = L->getLoopDepth();

    for (Loop::block_iterator BI = L->block_begin(), BE = L->block_end();
        BI != BE; ++BI) {
      const BasicBlock* BB = *BI;
      if (LI.getLoopDepth(BB) != LS->NestDepth) {
        // In a nested loop.
        continue;
      }
      LS->NumberInstructions += BB->size();

      for (BasicBlock::const_iterator I = BB->begin(), E = BB->end(); I != E;
          ++I) {
        if (dyn_cast<LoadInst>(I)) {
          LS->NumberLoads++;
        } else if (const StoreInst* SI = dyn_cast<StoreInst>(I)) {
          LS->NumberStores++;

          if (dyn_cast<LoadInst>(SI->getValueOperand()) && 
              dyn_cast<AllocaInst>(SI->getPointerOperand())) {
            LS->NumberMemoryCopies++;
          }
        } else if (dyn_cast<CmpInst>(I)) {
          LS->NumberCompares++;
        } else if (const BranchInst* BI = dyn_cast<BranchInst>(I)) {
          LS->NumberBranches++;

          // Spotting If statements is difficult in LLVM IR. If a branch
          // statement can only result in locations that are in the same
          // loop and at the same loop level, it is an If statement of
          // some kind.
          // This likely also counts some other cases, but that's ok.
          if (BI->isConditional()) {
            bool IsIfStatement = true;
            unsigned int NumberSuccessors = BI->getNumSuccessors();
            for (unsigned int i = 0; i < NumberSuccessors; i++) {
              if (!L->contains(BI->getSuccessor(i)) || 
                  LI.getLoopDepth(BI->getSuccessor(i)) != LS->NestDepth) {
                IsIfStatement = false;
              }
            }
            LS->ContainsIfStatement = 
                LS->ContainsIfStatement || IsIfStatement;
          }
        } else if (const IndirectBrInst* IBI = dyn_cast<IndirectBrInst>(I)) {
          LS->NumberBranches++;

          // Spotting If statements is difficult in LLVM IR. If a branch
          // statement can only result in locations that are in the same
          // loop and at the same loop level, it is an If statement of
          // some kind.
          // This likely also counts some other cases, but that's ok.
          bool IsIfStatement = true;
          unsigned int NumberDestinations = IBI->getNumDestinations();
          for (unsigned int i = 0; i < NumberDestinations; i++) {
            if (!L->contains(IBI->getDestination(i)) || 
                LI.getLoopDepth(IBI->getDestination(i)) != LS->NestDepth) {
              IsIfStatement = false;
            }
          }

          LS->ContainsIfStatement = 
              LS->ContainsIfStatement || IsIfStatement;
        } else if (const BinaryOperator* BO = dyn_cast<BinaryOperator>(I)) {
          if (BO->getOpcode() == Instruction::SDiv ||
              BO->getOpcode() == Instruction::UDiv) {
            LS->NumberDivides++;
          }

          switch (BO->getOpcode()) {
            case Instruction::Mul:  // a * b
            case Instruction::URem: // a % b
            case Instruction::SRem: // a % b
            case Instruction::Add:  // a + b
            case Instruction::Sub:  // a - b
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
        } else if (dyn_cast<CallInst>(I)) {
          LS->NumberCalls++;
        } else if (
            const GetElementPtrInst* GEP = dyn_cast<GetElementPtrInst>(I)) {
          // Nicely, clang on -O0 will produce a GEP for *every* array
          // access, even to the same constant location multiple times in a
          // row.
          if (dyn_cast<PointerType>(GEP->getPointerOperandType()) &&
              dyn_cast<ArrayType>(GEP->getPointerOperandType()->getPointerElementType())) {

            // Not so nicely, it'll create one for each index in a multi-dim
            // array. For counting array instructions and references, we should
            // only look at the outer-most access, where the pointer operand is
            // an alloca rather than another GEP.
            if (dyn_cast<AllocaInst>(GEP->getPointerOperand())) {
              LS->NumberArrayInstructions++;

              // Check if the array index is constant.
              LS->AllArrayIndicesConstant = 
                  LS->AllArrayIndicesConstant && GEP->hasAllConstantIndices();

              // Check if we've seen the array reference before.
              StringRef ArrayName = GEP->getPointerOperand()->getName();
              std::set<StringRef>::iterator I = 
                  LS->ReferencedArrays.find(ArrayName);
              if (I == LS->ReferencedArrays.end()) {
                LS->ReferencedArrays.insert(ArrayName);
                LS->NumberArrayReferences++;
              }
            }

            // For all array accesses, check if the index is the iterator,
            // and what dimension it strides on. For an array the access is
            // the final item in the idx list.
            GetElementPtrInst::const_op_iterator I = GEP->idx_end();
            --I; // Move back to the final item.

            if (Instruction* Inst = dyn_cast<Instruction>(*I)) {
              while (isa<CastInst>(Inst)) {
                // Work through any casts.
                Inst = dyn_cast<Instruction>(Inst->getOperand(0));
              }

              if (LoadInst* LI = dyn_cast<LoadInst>(Inst)) {
                if (LI->getPointerOperand() == LS->IteratorVariable) {
                  LS->LoopIteratorIsArrayIndex = true;

                  // Need to check if we're the outermost access; if not,
                  // then we don't only stride on leading dimensions. The
                  // outermost access has an element type that is *not*
                  // ArrayType.
                  ArrayType* AT = dyn_cast<ArrayType>(
                      GEP->getPointerOperandType()->getPointerElementType());
                  Type* ElementType = AT->getElementType();
                  if (dyn_cast<ArrayType>(ElementType)) {
                    // We are striding on a non-leading dimension!
                    LS->StridesOnlyOnLeadingDimensions = false;
                  }
                }
              }
            
              // Check for linear array access. Looks like one of the
              // following:
              //   * constant*var (order invariant)
              //   * constant*var +- constant (order invariant)
              //   * var +- constant (order invariant)
              //   * var
              //   * constant
              if (BinaryOperator* BO = dyn_cast<BinaryOperator>(Inst)) {
                errs() << *BO << "\n";
                if (BO->getOpcode() == Instruction::Add ||
                    BO->getOpcode() == Instruction::Sub) {
                  // LHS or RHS must be constant. For simplicity, move any
                  // constants to the RHS.
                  if (isa<Constant>(BO->getOperand(0))) {
                    BO->swapOperands();
                  }

                  if (isa<Constant>(BO->getOperand(1))) {
                    // LHS must now be either var or constant*var.
                    if (!IsLinearMult(BO->getOperand(0)) &&
                        !isa<LoadInst>(BO->getOperand(0))) {
                      LS->HasNonLinearArrayAccess = true;
                    }
                  } else {
                    // RHS isn't constant.
                    LS->HasNonLinearArrayAccess = true;
                  }

                } else if (BO->getOpcode() == Instruction::Mul) {
                  if (!IsLinearMult(BO)) {
                    LS->HasNonLinearArrayAccess = true;
                  }
                } else {
                  LS->HasNonLinearArrayAccess = true;
                }
              } else if (!isa<LoadInst>(Inst) && !isa<Constant>(Inst)) {
                LS->HasNonLinearArrayAccess = true;
              }
            }
          }
        }
      }
    }
    return false;
  }

  bool LoopFeatureExtraction::doFinalization() {
    PostProcessLoops();

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
      llvm::errs() //<< "\tLoop step in body: " << LS->LoopStepInBody << "\n"
          << "\tNestDepth " << LS->NestDepth << "\n"
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

      // Third set, booleans.
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
    return false;
  }

  void LoopFeatureExtraction::getAnalysisUsage(AnalysisUsage &AU) const {
    AU.addRequired<LoopInfo>();
    AU.addPreserved<LoopInfo>();
  }

  bool LoopFeatureExtraction::IsLinearMult(Value* V) {
    BinaryOperator* BO = dyn_cast<BinaryOperator>(V);
    if (!BO) {
      return false;
    }

    if (BO->getOpcode() == Instruction::Mul) {
      // Make any constants be on the RHS.
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
    // The lower bound is actually found in the block before the header.
    BasicBlock* BeforeHeader = L->getLoopPreheader();
    if (BeforeHeader) {
      BasicBlock::iterator I = BeforeHeader->end();
      while (!isa<StoreInst>(*I) && I != BeforeHeader->begin()) {
        --I;
      }

      StoreInst* SI = dyn_cast<StoreInst>(I);
      if (SI && 
          SI->getPointerOperand() == LS->IteratorVariable &&
          isa<Constant>(SI->getValueOperand())) {
        LS->HasConstantLowerBound = true;
      }
    }

    // The upper bound is defined by a compare in the header.
    BasicBlock* Header = L->getHeader();
    if (Header) {
      BasicBlock::iterator I = Header->begin();
      while (isa<PHINode>(I)) {
        ++I;
      }
      ++I;

      Instruction* SecondInst = &*I;
      if (CmpInst* CI = dyn_cast<CmpInst>(SecondInst)) {
        if (isa<Constant>(CI->getOperand(1))) {
          LS->HasConstantUpperBound = true;
        }
      }
    }

    // The stride is found in the increment section. Best way to get
    // there is from the header, although it's not pretty.
    if (Header) {
      for (pred_iterator PI = pred_begin(Header), E = pred_end(Header);
          PI != E; PI++) {
        if (*PI == BeforeHeader) {
          continue;
        }

        // The inc block has one successor; the header.
        succ_iterator I = succ_begin(*PI), E = succ_end(*PI);
        --E; // Move back one.
        if (I == E && *I == Header) {
          BasicBlock* Inc = *PI;

          BasicBlock::iterator II = Inc->begin();
          while (!isa<PHINode>(II) && !isa<LoadInst>(II) &&
              II != Inc->end()) {
            ++II;
          }

          LoadInst* LI = dyn_cast<LoadInst>(II);
          if (LI->getPointerOperand() == LS->IteratorVariable) {
            // Move on and find an increment. Hopefully.
            ++II;
            if (BinaryOperator* BO = dyn_cast<BinaryOperator>(II)) {
              if (BO->getOperand(0) == LI && BO->getOpcode() == Instruction::Add) {
                Value* RHS = BO->getOperand(1);
                if (ConstantInt* CI = dyn_cast<ConstantInt>(RHS)) {
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
  }

  void LoopFeatureExtraction::PostProcessLoops() {
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
        errs() << "BIG PROBLEM HERE: findLoopStruct failed!\n";
      }

      if (ChildLS->ContainsIfStatement) {
        return true;
      } else if (IfInForStatement(ChildLS)) {
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