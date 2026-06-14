#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {

struct DeadBBElimPass : public FunctionPass {
    static char ID;

    DeadBBElimPass() : FunctionPass(ID) {}

    bool runOnFunction(Function& F) override {
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
