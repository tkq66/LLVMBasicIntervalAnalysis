#ifndef VALTRK_H
#define VALTRK_H

#include <string>
#include <unordered_map>
#include <functional>
#include "llvm/IR/Instruction.h"
#include "Tracker.h"

using namespace llvm;

class ValueTracker : public Tracker {
    public:
        typedef std::unordered_map<std::string, double> var_map_t;
        typedef std::pair<std::string, double> var_t;
        typedef std::function<double(double, double)> arithmetic_function_t;

        double selectVariable(std::string name);
        void editVariable(std::string name, double value);

        void printTracker() override;
        void processNewEntry(Instruction *i) override;

        void allocateNewVariable(AllocaInst* i) override;
        void storeValueIntoVariable(StoreInst* i) override;
        void loadVariableIntoRegister(LoadInst* i) override;
        void processCalculation(BinaryOperator* i) override;

    private:
        var_map_t variablesTracker;

        void calculateArithmetic(BinaryOperator* i, arithmetic_function_t callback);
        double addCallback(double accumulator, double current);
        double subCallback(double accumulator, double current);
        double mulCallback(double accumulator, double current);
        double sremCallback(double accumulator, double current);
};

#endif
