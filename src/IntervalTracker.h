#ifndef ITVTRK_H
#define ITVTRK_H

#include <string>
#include <unordered_map>
#include <tuple>
#include "Tracker.h"
#include "ValueTracker.h"
#include "llvm/IR/Instruction.h"

using namespace llvm;

class IntervalTracker {
    public:
        typedef std::tuple<double, double> interval_t;
        typedef std::unordered_map<std::string, interval_t> var_map_t;
        typedef std::unordered_map<std::string, interval_t>::iterator var_it_t;
        typedef std::pair<std::string, interval_t> var_t;
        typedef std::function<interval_t(interval_t, interval_t)> arithmetic_function_t;

        // Static methods
        static var_t getVarEntryFromPtr(void* ptr);

        // Instance-bound methods
        void* getPtrFromVariableName(std::string name);
        interval_t getVariableInterval(std::string name);
        double getVariableValue(std::string name);

        // Tracker methods
        void printTracker() override;
        void* processNewEntry(Instruction *i) override;
        void* allocateNewVariable(AllocaInst* i) override;
        void* storeValueIntoVariable(StoreInst* i) override;
        void* loadVariableIntoRegister(LoadInst* i) override;
        void* processCalculation(BinaryOperator* i) override;

    private:
        // Core tracker state
        ValueTracker valueTracker;
        var_map_t intervalsTracker;

        // Caclulation helpers
        var_t calculateArithmetic(BinaryOperator* i, arithmetic_function_t callback);
        interval_t addCallback(interval_t accumulator, double current);
        interval_t subCallback(interval_t accumulator, double current);
        interval_t mulCallback(interval_t accumulator, double current);
        interval_t sremCallback(interval_t accumulator, double current);
};

#endif
