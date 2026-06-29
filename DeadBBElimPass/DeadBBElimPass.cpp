#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Dominators.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"

#include <vector>
#include <unordered_set>

using namespace llvm;

namespace {

struct DeadBBElimPass : public FunctionPass {
    static char ID;

    DeadBBElimPass() : FunctionPass(ID) {}

    // Ovde pamtim blokove do kojih moze da se dodje iz entry bloka
    std::unordered_set<BasicBlock*> ReachableBlocks;

    // Ovde cuvam blokove koje treba obrisati
    std::vector<BasicBlock*> BlocksToRemove;

    // DFS od entry-ja
    void DFS(BasicBlock* Current) {
        if (ReachableBlocks.find(Current) != ReachableBlocks.end()) {
            return;
        }

        ReachableBlocks.insert(Current);

        // Obilazim sve naslednike trenutnog bloka
        for (BasicBlock* Successor : successors(Current)) {
            DFS(Successor);
        }
    }

    bool runOnFunction(Function& F) override {
        DominatorTree& DT = getAnalysis<DominatorTreeWrapperPass>().getDomTree();

        ReachableBlocks.clear();
        BlocksToRemove.clear();

        // Krecem od entry bloka jer je to pocetak svake funkcije
        DFS(&F.getEntryBlock());

        for (BasicBlock& BB : F) {
            // Ako blok nije posecen DFS-om ili ga entry blok ne dominira,
            if (ReachableBlocks.find(&BB) == ReachableBlocks.end() ||
                !DT.dominates(&F.getEntryBlock(), &BB)) {
                BlocksToRemove.push_back(&BB);
            }
        }

        if (BlocksToRemove.empty()) {
            return false;
        }

        errs() << "Function " << F.getName() << " has dead basic blocks:\n";

        for (BasicBlock* BB : BlocksToRemove) {
            errs() << "  removing block: " << BB->getName() << "\n";

        
            std::vector<BasicBlock*> Succs;

            for (BasicBlock* Succ : successors(BB)) {
                Succs.push_back(Succ);
            }

            for (BasicBlock* Succ : Succs) {
                Succ->removePredecessor(BB);
            }

            // brisem instrukcije iz bloka unazad
            while (!BB->empty()) {
                Instruction& I = BB->back();
                I.dropAllReferences();
                I.eraseFromParent();
            }

            BB->eraseFromParent();
        }

        return true;
    }

    void getAnalysisUsage(AnalysisUsage& AU) const override {
        AU.addRequired<DominatorTreeWrapperPass>();
    }
};

}

char DeadBBElimPass::ID = 0;

static RegisterPass<DeadBBElimPass> X(
    "dead-bb-elim",
    "Dead Basic Block Elimination Pass",
    false,
    false
);