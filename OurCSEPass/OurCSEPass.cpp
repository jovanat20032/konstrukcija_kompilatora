#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Pass.h>
#include <llvm/IR/Dominators.h>
#include "llvm/IR/Operator.h"

#include "llvm/Support/raw_ostream.h"

#include <unordered_map>
#include <vector>
#include <unordered_map>

using namespace llvm;

namespace{
    

    struct OurCSEPass : public FunctionPass {

        static char ID; //nije bitna vrednost, nego samo adresa

        std::unordered_map<Value*, Instruction*> LastSeenLoadInstruction; //za svaki pokazivac pamti poslednju load instrukciju koja je ucitana sa te adrese

        std::unordered_map<std::string, Instruction*> ExpressionMap;// pamti izraze i instrukciju koja ih je izracunala

        std::vector<Instruction*> InstructionsToRemove;//vektor u koji skupljamo instrukcije za brisanje
        OurCSEPass() : FunctionPass(ID) {}

        bool runOnFunction(Function &F) override {
            
            DominatorTree &DT = getAnalysis<DominatorTreeWrapperPass>().getDomTree(); //dohvata dominator stablo

            bool Changed = false;

            errs() << "Pokrenut je CSE Pass za funkciju: " << F.getName() << "\n";

            LastSeenLoadInstruction.clear();
            ExpressionMap.clear();//cistimo strukture na početku svake funkcije
            InstructionsToRemove.clear();

            for(BasicBlock &BB : F){ //prolaz kroz sve basic blockove funkcije
                for(Instruction &I : BB){ //prolaz kroz sve instrukcije basic blocka
                    if(LoadInst* LoadInstr = dyn_cast<LoadInst>(&I)) {//citanje iz memorije
                        Value* PointerOperand = LoadInstr->getPointerOperand();
                        if(LastSeenLoadInstruction.find(PointerOperand) != LastSeenLoadInstruction.end()) {//da li u mapi vec imamo citanje sa ove adrese
                            Instruction* PreviousInstr = LastSeenLoadInstruction[PointerOperand];

                            if(DT.dominates(PreviousInstr, LoadInstr)) {//da li staro citanje dominira novim
                                LoadInstr->replaceAllUsesWith(PreviousInstr); //svi koji koriste novo, neka koriste staro
                                InstructionsToRemove.push_back(&I); //pakujemo za brisanje
                                Changed = true;
                            }
                        }
                        LastSeenLoadInstruction[PointerOperand] = &I;//azuriramo mapu: ovo je sada najsvezije citanje za ovu adresu
                    }else if(StoreInst* StoreInstr = dyn_cast<StoreInst>(&I)) {//upis u memoriju
                        Value* PointerOperand = StoreInstr->getPointerOperand();

                        if(LastSeenLoadInstruction.find(PointerOperand) != LastSeenLoadInstruction.end()) {//ako neko upise novu vrednost, staro citanje vise ne važi i brisemo ga iz mape
                            LastSeenLoadInstruction.erase(PointerOperand);
                        }
                    }else if(BinaryOperator* BO = dyn_cast<BinaryOperator>(&I)) {
                        Value* LeftOp = BO->getOperand(0);
                        Value* RightOp = BO->getOperand(1);

                        if(BO->isCommutative() && LeftOp > RightOp) {//ako je operator komutativan sortira operande po adresi da dobijemo jedinstveni kljuc
                            std::swap(LeftOp, RightOp);
                        }

                        std::string Key;                  //string koji ce predstavljati jedinstveni izraz
                        raw_string_ostream RSO(Key);      //strim za lako konstruisanje stringa

                        RSO << BO->getOpcodeName() << " ";
                        LeftOp->print(RSO);
                        RSO << ", ";                     
                        RightOp->print(RSO);

                        std::string ExpressionKey = RSO.str();

                        if(ExpressionMap.find(ExpressionKey) != ExpressionMap.end()) {//proveri da li smo vec videli isti izraz (isti operator i isti operandi)
                            Instruction* PreviousInstr = ExpressionMap[ExpressionKey];//uzima prethodnu instrukciju koja je izracunala ovaj izraz
                            
                            if(DT.dominates(PreviousInstr, BO)) {//proveri da li prethodna instrukcija dominira trenutnu (da je prethodna uvek ranije)
                            BO->replaceAllUsesWith(PreviousInstr);
                            InstructionsToRemove.push_back(&I);
                            Changed = true;
                            }
                        } else {
                            ExpressionMap[ExpressionKey] = &I;//ako izraz nije viđen do sada, pamti ga u mapi
                        }
                    }
                }
            }

            for(Instruction* I : InstructionsToRemove) { //fizicko brisanje instrukcija tek na kraju, kada je potpuno bezbedno
                I->eraseFromParent();
            }
            return Changed;
        }

        void getAnalysisUsage(AnalysisUsage &AU) const override { //pre passa se obezbedjuje izracuvavanje dominator stabla i takodje se navodi da se CFG ne menja
            AU.addRequired<DominatorTreeWrapperPass>();

            AU.setPreservesCFG();
        }
    };
}

char OurCSEPass::ID = 0; //vrednost nije bitna, nego samo adresa

static RegisterPass<OurCSEPass> X("cse-pass", "Our CSE Pass", false, false); //argumenti su: argument za opt, ime passa, da li je CFG only, da li je analysis pass