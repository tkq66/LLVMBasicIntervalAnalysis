#ifndef TRK_H
#define TRK_H

#include "llvm/IR/Instruction.h"

using namespace llvm;

class Tracker {
    public:
        virtual ~Tracker(){}

        virtual void printTracker() = 0;
        virtual void* processNewEntry(Instruction *i) = 0;
        virtual void* allocateNewVariable(AllocaInst* i) = 0;
        virtual void* storeValueIntoVariable(StoreInst* i) = 0;
        virtual void* loadVariableIntoRegister(LoadInst* i) = 0;
        virtual void* processCalculation(BinaryOperator* i) = 0;
};

#endif
