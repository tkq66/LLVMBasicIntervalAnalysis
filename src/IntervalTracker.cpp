#include <cstdio>
#include <iostream>
#include <string>
#include <sstream>
#include <tuple>
#include <unordered_map>
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Constants.h"
#include "ValueTracker.h"
#include "IntervalTracker.h"

var_t IntervalTracker::getVarEntryFromPtr(void* ptr) {
    // TODO: Add some error handling if ptr == nullptr
    return *static_cast<var_t*>(ptr);
}

void* IntervalTracker::getPtrFromVariableName(std::string name) {
    var_it_t variableIterator = variablesTracker.find(name);
    if (variableIterator == variablesTracker.end()) {
        return nullptr;
    }
    std::string varName = variableIterator->first;
    interval_t varValue = std::make_tuple(std::get<0>(variableIterator->second), std::get<1>(variableIterator->second));
    var_t variable = std::make_pair(varName, varValue);
    void* elementPtr = &variable;
    return elementPtr;
}

interval_t IntervalTracker::getVariableInterval(std::string name) {
    return (intervalsTracker.find(name) != intervalsTracker.end()) ? intervalsTracker[name] : std::make_tuple(std::nan("-infinity"), std::nan("+infinity"));
}

void IntervalTracker::printTracker() {
    for (auto variable = intervalsTracker.begin(); variable != intervalsTracker.end(); ++variable) {
        printf("Key: %s - [ %lf , %lf ]\n", variable->first.c_str(), std::get<0>(variable->second), std::get<1>(variable->second));
    }
    printf("\n");
}

void* IntervalTracker::processNewEntry(Instruction* i) {
    if (isa<AllocaInst>(i)) {
        return allocateNewVariable(dyn_cast<AllocaInst>(i));
    }
    else if (isa<StoreInst>(i)) {
        return storeValueIntoVariable(dyn_cast<StoreInst>(i));
    }
    else if (isa<LoadInst>(i)) {
        return loadVariableIntoRegister(dyn_cast<LoadInst>(i));
    }
    else if (isa<BinaryOperator>(i)) {
        return processCalculation(dyn_cast<BinaryOperator>(i));
    }
}

void* IntervalTracker::allocateNewVariable(AllocaInst* i) {
    void* variablePtr = valueTracker.allocateNewVariable(i);
    ValueTracker::var_t variable = ValueTracker::getVariableFromPtr(variablePtr);
    std::string varName = variable->first;
    interval_t varValue = std::make_tuple(std::nan("-infinity"), std::nan("infinity");
    var_t variableInterval = std::make_pair(varName, varValue);
    intervalsTracker.insert(variableInterval);

    // Returns reference to newly created entry
    return getPtrFromVariableName(varName);
}

void* IntervalTracker::storeValueIntoVariable(StoreInst* i) {
    void* variablePtr = valueTracker.storeValueIntoVariable(i);
    ValueTracker::var_t variable = ValueTracker::getVariableFromPtr(variablePtr);
    std::string varName = variable->first;
    interval_t varValue = std::make_tuple(variable->second, variable->second);
    intervalsTracker[varName] = varValue;

    // Returns reference to newly created entry
    return getPtrFromVariableName(varName);
}

void* IntervalTracker::loadVariableIntoRegister(LoadInst* i) {
    void* variablePtr = valueTracker.loadVariableIntoRegister(i);
    ValueTracker::var_t variable = ValueTracker::getVariableFromPtr(variablePtr);
    std::string registerName = variable->first;
    interval_t varValue = std::make_tuple(variable->second, variable->second);
    var_t variableInterval = std::make_pair(registerName, varValue);
    intervalsTracker.insert(variableInterval);

    // Returns reference to recently added register entry
    return getPtrFromVariableName(registerName);
}

void* IntervalTracker::processCalculation(BinaryOperator* i) {
    void* variablePtr = valueTracker.processCalculation(i);

    arithmetic_function_t calculation;
    switch (i->getOpcode()) {
        case Instruction::Add:
            calculation = std::bind(&IntervalTracker::addCallback, this, std::placeholders::_1, std::placeholders::_2);
            break;
        case Instruction::Mul:
            calculation = std::bind(&IntervalTracker::mulCallback, this, std::placeholders::_1, std::placeholders::_2);
            break;
        case Instruction::Sub:
            calculation = std::bind(&IntervalTracker::subCallback, this, std::placeholders::_1, std::placeholders::_2);
            break;
        case Instruction::SRem:
            calculation = std::bind(&IntervalTracker::sremCallback, this, std::placeholders::_1, std::placeholders::_2);
            break;
        default:
            break;
    }
    var_t variable = calculateArithmetic(i, calculation);

    // Returns reference to recently modified entry
    return getPtrFromVariableName(variable->first);
}

var_t IntervalTracker::calculateArithmetic(BinaryOperator* i, arithmetic_function_t callback) {
    interval_t destInterval;
    for (auto val = i->value_op_begin(); val != i->value_op_end(); ++val) {
        std::string currentName;
        double currentValue;
        if (val->hasName()) {
            currentName = val->getName().str();
            currentValue = variablesTracker[currentName];
        }
        else if (ConstantInt* numConstant = dyn_cast<ConstantInt>(*val)) {
            currentValue = numConstant->getZExtValue();
        }
        else {
            std::stringstream registerValue;
            registerValue << *val;
            currentName = registerValue.str();
            currentValue = variablesTracker[currentName];
        }
        destInterval = (val == i->value_op_begin()) ? std::make_tuple(currentValue, currentValue) : callback(destInterval, currentValue);
    }
    std::string destName = i->getName().str();
    std::pair<std::string, double> calculatedVariable = std::make_pair(destName, destInterval);
    variablesTracker.insert(calculatedVariable);
}

interval_t IntervalTracker::addCallback(interval_t accumulator, double current) {
    double min = std::get<0>(accumulator);
    double max = std::get<1>(accumulator);
    double resultMin;
    double resultMax;
    if (std::isnan(current)) {
        resultMin = std::nan("-infinity")
        resultMax = std::nan("+infinity")
    }
    else {
        if (std::isnan(min)) {
            resultMin = std::nan("-infinity")
        }
        else {
            resultMin = min + current;
        }
        if (std::isnan(max)) {
            resultMax = std::nan("+infinity")
        }
        else {
            resultMax = max + current;
        }
    }
    return std::make_tuple(resultMin, resultMax);
}

interval_t IntervalTracker::subCallback(interval_t accumulator, double current) {
    double min = std::get<0>(accumulator);
    double max = std::get<1>(accumulator);
    double resultMin;
    double resultMax;
    if (std::isnan(current)) {
        resultMin = std::nan("-infinity")
        resultMax = std::nan("+infinity")
    }
    else {
        if (std::isnan(min)) {
            resultMin = std::nan("-infinity")
        }
        else {
            resultMin = min - current;
        }
        if (std::isnan(max)) {
            resultMax = std::nan("+infinity")
        }
        else {
            resultMax = max - current;
        }
    }
    return std::make_tuple(resultMin, resultMax);
}

interval_t IntervalTracker::mulCallback(interval_t accumulator, double current) {
    double min = std::get<0>(accumulator);
    double max = std::get<1>(accumulator);
    double resultMin;
    double resultMax;
    if (std::isnan(current)) {
        resultMin = std::nan("-infinity")
        resultMax = std::nan("+infinity")
    }
    else {
        if (std::isnan(min)) {
            resultMin = std::nan("-infinity")
        }
        else {
            resultMin = min * current;
        }
        if (std::isnan(max)) {
            resultMax = std::nan("+infinity")
        }
        else {
            resultMax = max * current;
        }
    }
    return std::make_tuple(resultMin, resultMax);
}

interval_t IntervalTracker::sremCallback(interval_t accumulator, double current) {
    double resultMin;
    double resultMax;
    if (std::isnan(current)) {
        resultMin = std::nan("-infinity")
        resultMax = std::nan("+infinity")
    }
    else {
        resultMin = 0;
        resultMax = current;
    }
    return std::make_tuple(resultMin, resultMax);
}
