#ifndef ITVTRK_H
#define ITVTRK_H

#include <string>
#include <unordered_map>
#include <tuple>
#include "ValueTracker.h"
#include "llvm/IR/Instruction.h"

using namespace llvm;

class IntervalTracker : public ValueTracker {
    private:
        var_map_t intervalsTracker;

        void calculateArithmetic(BinaryOperator* i, std::function<double(double, double)> callback);
        double addCallback(double accumulator, double current);
        double subCallback(double accumulator, double current);
        double mulCallback(double accumulator, double current);
        double sremCallback(double accumulator, double current);

    public:
        typedef std::tuple<double, double> interval_t;
        typedef std::unordered_map<std::string, interval_t> var_map_t;
        typedef std::pair<std::string, interval_t> var_t;

        interval_t selectInterval(std::string name);

        void printTracker();
        void processNewEntry(Instruction *i);

        void allocateNewVariable(AllocaInst* i);
        void storeValueIntoVariable(StoreInst* i);
        void loadVariableIntoRegister(LoadInst* i);
        void processCalculation(BinaryOperator* i);
};

#endif
