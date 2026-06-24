#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Function.h"
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

    // Ovde cuvam blokove koje kasnije treba obrisati
    std::vector<BasicBlock*> BlocksToRemove;

    // Obican DFS kroz CFG funkcije
    void DFS(BasicBlock* Current) {
        if (ReachableBlocks.find(Current) != ReachableBlocks.end()) {
            return;
        }

        ReachableBlocks.insert(Current);

        // Idemo kroz sve naslednike trenutnog bloka
        for (BasicBlock* Successor : successors(Current)) {
            DFS(Successor);
        }
    }

    bool runOnFunction(Function& F) override {
        ReachableBlocks.clear();
        BlocksToRemove.clear();

        // Obilazak uvek krece od entry bloka funkcije
        DFS(&F.getEntryBlock());

        for (BasicBlock& BB : F) {
            // Ako blok nije posecen DFS-om, onda je nedostizan
            if (ReachableBlocks.find(&BB) == ReachableBlocks.end()) {
                BlocksToRemove.push_back(&BB);
            }
        }

        if (BlocksToRemove.empty()) {
            return false;
        }

        errs() << "Function " << F.getName() << " has unreachable blocks:\n";

        for (BasicBlock* BB : BlocksToRemove) {
            errs() << "  " << BB->getName() << "\n";
        }

        // Za sada samo pronalazimo blokove, brisanje ide u sledecem commitu
        return false;
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
