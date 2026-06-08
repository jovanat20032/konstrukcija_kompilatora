#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <llvm/Pass.h>
#include <llvm/IR/Dominators.h>

#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace{
    

    struct OurCSEPass : public FunctionPass {

        static char ID; //nije bitna vrednost, nego samo adresa

        OurCSEPass() : FunctionPass(ID) {}

        bool runOnFunction(Function &F) override {
            
            DominatorTree &DT = getAnalysis<DominatorTreeWrapperPass>().getDomTree(); //dohvata dominator stablo

            bool Changed = false;

            errs() << "Pokrenut je CSE Pass za funkciju: " << F.getName() << "\n";

            for(BasicBlock &BB : F){ //prolaz kroz sve basic blockove funkcije
                for(Instruction &I : BB){ //prolaz kroz sve instrukcije basic blocka
                    
                }
            }

            return Changed;
        }

        void getAnalysisUsage(AnalysisUsage &AU) const override { //pre passa se obezbedjuje izracuvavanje dominator stabla i takodje se navodi da se CFG ne menja
            AU.addRequired<DominatorTreeWrapperPass>();

            AU.setPreservesCFG();
        }
    };
}

char OurCSEPass::ID = 0; //vrednka nije bitna, nego samo adresa

static RegisterPass<OurCSEPass> X("cse-pass", "Our CSE Pass", false, false); //argumenti su: argument za opt, ime passa, da li je CFG only, da li je analysis pass