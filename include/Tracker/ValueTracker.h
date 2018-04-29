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
        typedef std::unordered_map<std::string, double>::iterator var_it_t;
        typedef std::pair<std::string, double> var_t;
        typedef std::function<double(double, double)> arithmetic_function_t;

        // Static methods
        static var_t getVariableFromPtr(void* ptr);

        // Instance-bound methods
        void* getPtrFromVariableName(std::string name);
        double getVariableValue(std::string name);
        void editVariable(std::string name, double value);
        var_map_t getValueTracker() const;
        void setTracker(var_map_t tracker);

        // Tracker methods
        void printTracker() override;
        void* processNewEntry(Instruction *i) override;
        void* allocateNewVariable(AllocaInst* i) override;
        void* storeValueIntoVariable(StoreInst* i) override;
        void* loadVariableIntoRegister(LoadInst* i) override;
        void* processCalculation(BinaryOperator* i) override;

    private:
        // Core tracker state
        var_map_t variablesTracker;

        // Caclulation helpers
        var_t calculateArithmetic(BinaryOperator* i, arithmetic_function_t callback);
        double addCallback(double accumulator, double current);
        double subCallback(double accumulator, double current);
        double mulCallback(double accumulator, double current);
        double sremCallback(double accumulator, double current);
};

#endif
