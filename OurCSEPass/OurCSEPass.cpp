#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Pass.h>
#include <llvm/IR/Dominators.h>
#include "llvm/IR/Operator.h"

#include "llvm/Support/raw_ostream.h"

#include <unordered_map>
#include <vector>

using namespace llvm;

namespace{
    

    struct OurCSEPass : public FunctionPass {

        static char ID;

        std::unordered_map<Value*, Instruction*> LastSeenLoadInstruction;
        
        std::unordered_map<std::string, Instruction*> ExpressionMap;

        std::vector<Instruction*> InstructionsToRemove;
        OurCSEPass() : FunctionPass(ID) {}

        bool isInteresting(Instruction *I) {
            if (BinaryOperator *BO = dyn_cast<BinaryOperator>(I)) {
                unsigned Opcode = BO->getOpcode();
                return Opcode == Instruction::Add || 
                    Opcode == Instruction::Sub || 
                    Opcode == Instruction::Mul || 
                    Opcode == Instruction::SDiv;
            }
    
            if (isa<ICmpInst>(I)) {
                return true;
            }
    
            return false;
        }
        bool isFPType(Instruction *I) {
            if (BinaryOperator* BO = dyn_cast<BinaryOperator>(I)) {
                return BO->getType()->isFloatingPointTy();
            }
            if (ICmpInst* Cmp = dyn_cast<ICmpInst>(I)) {
                if (Cmp->getOperand(0)->getType()->isFloatingPointTy()) {
                    return true; 
                }
            }
            return false;
        }

        bool isCommutative(Instruction *I) {
            if (isa<ICmpInst>(I)) {
                ICmpInst *Cmp = dyn_cast<ICmpInst>(I);
                CmpInst::Predicate Pred = Cmp->getPredicate();
                return Pred == ICmpInst::ICMP_EQ || Pred == ICmpInst::ICMP_NE;
            }
            return isa<AddOperator>(I) || isa<MulOperator>(I);
        }

        std::string getOperandKey(Value *V) {
            std::string Result;
            raw_string_ostream RSO(Result);

            if (!V->getName().empty()) {
                RSO << V->getName().str();
                return RSO.str();
            }

            if (ConstantInt *CI = dyn_cast<ConstantInt>(V)) {
                RSO << "C" << CI->getSExtValue(); 
                return RSO.str();
            }
            
            RSO << V->getType()->getTypeID() << "_id" << (uintptr_t)V;
            return RSO.str();
        }

        std::string generateExpressionKey(Instruction *I) {
            std::string Key;
            raw_string_ostream RSO(Key);
            
            if (isa<ICmpInst>(I)) {
                ICmpInst *Cmp = dyn_cast<ICmpInst>(I);
                RSO << "icmp_" << Cmp->getPredicate() << "_"; 
            } else if (isa<BinaryOperator>(I)) {
                BinaryOperator *BO = dyn_cast<BinaryOperator>(I);
                RSO << BO->getOpcodeName() << " ";
            } else {
                return "";
            }
            
            Value* LeftOp = I->getOperand(0);
            Value* RightOp = I->getOperand(1);
            
            if (isCommutative(I)) {
                std::string LeftKey = getOperandKey(LeftOp);
                std::string RightKey = getOperandKey(RightOp);
                
                if (LeftKey > RightKey) {
                    std::swap(LeftOp, RightOp);
                }
            }
            
            RSO << getOperandKey(LeftOp) << ", " << getOperandKey(RightOp);
            
            return RSO.str();
        }

        bool isTheSameInstruction(Instruction *I1, Instruction *I2) {
            if (I1->getOpcode() != I2->getOpcode()) {
                return false;
            }

            if (isa<ICmpInst>(I1) && isa<ICmpInst>(I2)) {
                ICmpInst *Cmp1 = dyn_cast<ICmpInst>(I1);
                ICmpInst *Cmp2 = dyn_cast<ICmpInst>(I2);
                if (Cmp1->getPredicate() != Cmp2->getPredicate()) {
                    return false; 
                }
            }
            
            return generateExpressionKey(I1) == generateExpressionKey(I2);
        }

        bool runOnFunction(Function &F) override {
            
            DominatorTree &DT = getAnalysis<DominatorTreeWrapperPass>().getDomTree();

            bool Changed = false;

            errs() << "Pokrenut je CSE Pass za funkciju: " << F.getName() << "\n";

            LastSeenLoadInstruction.clear();
            ExpressionMap.clear();
            InstructionsToRemove.clear();

            for(BasicBlock &BB : F){
                for(Instruction &I : BB){
                    if(LoadInst* LoadInstr = dyn_cast<LoadInst>(&I)) {
                        if (LoadInstr->isVolatile()) {
                            continue;
                        }
                        if (LoadInstr->getOrdering() != AtomicOrdering::NotAtomic) {
                            continue;
                        }
                        Value* PointerOperand = LoadInstr->getPointerOperand();
                        if(LastSeenLoadInstruction.find(PointerOperand) != LastSeenLoadInstruction.end()) {
                            Instruction* PreviousInstr = LastSeenLoadInstruction[PointerOperand];

                            if(DT.dominates(PreviousInstr, LoadInstr)) {
                                LoadInstr->replaceAllUsesWith(PreviousInstr);
                                InstructionsToRemove.push_back(&I);
                                Changed = true;
                                continue;
                            }
                        }
                        LastSeenLoadInstruction[PointerOperand] = &I;
                    }else if(StoreInst* StoreInstr = dyn_cast<StoreInst>(&I)) {
                         if (StoreInstr->isVolatile()) {
                            continue;
                        }
                        if (StoreInstr->getOrdering() != AtomicOrdering::NotAtomic) {
                            continue;
                        }
                        LastSeenLoadInstruction.clear();
                    }else if (CallInst* CI = dyn_cast<CallInst>(&I)) {
                        if (!CI->onlyReadsMemory()) {
                            LastSeenLoadInstruction.clear();
                        }
                    }else if(BinaryOperator* BO = dyn_cast<BinaryOperator>(&I)) {
                        if (isFPType(BO)) {
                            continue;
                        }
                        
                        if (!isInteresting(BO)) {
                            continue;
                        }
                        std::string ExpressionKey = generateExpressionKey(BO);

                        if(ExpressionMap.find(ExpressionKey) != ExpressionMap.end()){
                            Instruction* PreviousInstr = ExpressionMap[ExpressionKey];
                            
                            if (!isTheSameInstruction(BO, PreviousInstr)) {
                                ExpressionMap[ExpressionKey] = BO;
                                continue;
                            }
                            
                            if(DT.dominates(PreviousInstr, BO)){
                                BO->replaceAllUsesWith(PreviousInstr);
                                InstructionsToRemove.push_back(&I);
                                Changed = true;
                            }
                        } else {
                            ExpressionMap[ExpressionKey] = BO;
                        }
                    }else if(ICmpInst* Cmp = dyn_cast<ICmpInst>(&I)){
                        if (!isInteresting(Cmp)) {
                            continue;
                        }
                        if (isFPType(Cmp)) {
                            continue;
                        }
                        std::string ExpressionKey = generateExpressionKey(Cmp);
                        
                        if(ExpressionMap.find(ExpressionKey) != ExpressionMap.end()){
                            Instruction* PreviousInstr = ExpressionMap[ExpressionKey];

                            if (!isTheSameInstruction(Cmp, PreviousInstr)) {
                                ExpressionMap[ExpressionKey] = Cmp;
                                continue;
                            }
                            
                            if(DT.dominates(PreviousInstr, Cmp)){
                                Cmp->replaceAllUsesWith(PreviousInstr);
                                InstructionsToRemove.push_back(&I);
                                Changed = true;
                            }
                        } else {
                            ExpressionMap[ExpressionKey] = Cmp;
                        }
                    }
                }
            }

            for(Instruction* I : InstructionsToRemove) {
                I->eraseFromParent();
            }
            return Changed;
        }

        void getAnalysisUsage(AnalysisUsage &AU) const override {
            AU.addRequired<DominatorTreeWrapperPass>();

            AU.setPreservesCFG();
        }
    };
}

char OurCSEPass::ID = 0;

static RegisterPass<OurCSEPass> X("cse-pass", "Our CSE Pass", false, false); 