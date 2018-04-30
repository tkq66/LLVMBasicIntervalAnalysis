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
#include "../../include/Tracker/ValueTracker.h"
#include "../../include/Tracker/IntervalTracker.h"

IntervalTracker::var_t IntervalTracker::getVarEntryFromPtr(void* ptr) {
    // TODO: Add some error handling if ptr == nullptr
    return *static_cast<var_t*>(ptr);
}

void* IntervalTracker::getPtrFromVariableName(std::string name) {
    var_it_t variableIterator = intervalsTracker.find(name);
    if (variableIterator == intervalsTracker.end()) {
        return nullptr;
    }
    std::string varName = variableIterator->first;
    interval_t varValue = std::make_tuple(std::get<0>(variableIterator->second), std::get<1>(variableIterator->second));
    var_t variable = std::make_pair(varName, varValue);
    void* elementPtr = &variable;
    return elementPtr;
}

IntervalTracker::interval_t IntervalTracker::getVariableInterval(std::string name) const {
    return (intervalsTracker.find(name) != intervalsTracker.end()) ? intervalsTracker.find(name)->second : std::make_tuple(std::nan("-infinity"), std::nan("+infinity"));
}

double IntervalTracker::getVariableValue(std::string name) {
    return valueTracker.getVariableValue(name);
}

IntervalTracker::var_map_t IntervalTracker::getIntervalsTracker() const {
    return intervalsTracker;
}

ValueTracker::var_map_t IntervalTracker::getValueTracker() const {
    return valueTracker.getValueTracker();
}

void IntervalTracker::setTracker(ValueTracker::var_map_t tracker) {
    valueTracker.setTracker(tracker);
}

void IntervalTracker::setTracker(IntervalTracker::var_map_t tracker) {
    intervalsTracker = var_map_t(tracker);
}

void IntervalTracker::setTracker(IntervalTracker::var_map_t iTracker, ValueTracker::var_map_t vTracker) {
    setTracker(iTracker);
    setTracker(vTracker);
}

void IntervalTracker::switchLoopState(bool state) {
    loopState = state;
}

bool IntervalTracker::isInLoop() const {
    return loopState;
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
    else if (isa<CmpInst>(i)) {
        return compareValues(dyn_cast<CmpInst>(i));
    }
    else if (isa<BinaryOperator>(i)) {
        return processCalculation(dyn_cast<BinaryOperator>(i));
    }
    else {
        return nullptr;
    }
}

void* IntervalTracker::allocateNewVariable(AllocaInst* i) {
    void* variablePtr = valueTracker.allocateNewVariable(i);
    ValueTracker::var_t variable = ValueTracker::getVariableFromPtr(variablePtr);
    std::string varName = variable.first;
    interval_t varValue = std::make_tuple(std::nan("-infinity"), std::nan("infinity"));
    var_t variableInterval = std::make_pair(varName, varValue);
    intervalsTracker.insert(variableInterval);

    // Returns reference to newly created entry
    return getPtrFromVariableName(varName);
}

void* IntervalTracker::storeValueIntoVariable(StoreInst* i) {
    valueTracker.storeValueIntoVariable(i);
    interval_t src;
    if (i->getOperand(0)->hasName()) {
        var_map_t::const_iterator existingVariable = intervalsTracker.find(i->getOperand(0)->getName().str());
        src = existingVariable->second;
    }
    else {
        ConstantInt* ci = dyn_cast<ConstantInt>(i->getOperand(0));
        int constant = ci->getSExtValue();
        src = std::make_tuple(constant, constant);
    }
    std::string dest = i->getOperand(1)->getName().str();

    if (!isInLoop()) {
        intervalsTracker[dest] = src;
    }
    else {
        interval_t destInterval = intervalsTracker.find(dest)->second;
        double destMin = std::get<0>(destInterval);
        double destMax = std::get<1>(destInterval);
        double srcMin = std::get<0>(src);
        double srcMax = std::get<1>(src);
        double resultMin = ((std::isnan(destMin) && std::isnan(srcMin)) || (!std::isnan(destMin) && std::isnan(srcMin))) ? std::nan("-infinity") : ((destMin < srcMin) ? destMin: srcMin);
        double resultMax = ((std::isnan(destMax) && std::isnan(srcMax)) || (!std::isnan(destMax) && std::isnan(srcMax))) ? std::nan("+infinity") : ((destMax > srcMax) ? destMax: srcMax);
        intervalsTracker[dest] = std::make_tuple(resultMin, resultMax);
    }

    // Returns reference to recently modified entry
    return getPtrFromVariableName(dest);
}

void* IntervalTracker::loadVariableIntoRegister(LoadInst* i) {
    void* variablePtr = valueTracker.loadVariableIntoRegister(i);
    ValueTracker::var_t variable = ValueTracker::getVariableFromPtr(variablePtr);
    std::string registerName = variable.first;
    interval_t varValue = std::make_tuple(variable.second, variable.second);
    var_t variableInterval = std::make_pair(registerName, varValue);
    if (intervalsTracker.find(registerName) == intervalsTracker.end()) {
        intervalsTracker.insert(variableInterval);
    }
    else {
        intervalsTracker[registerName] = varValue;
    }

    // Returns reference to recently added register entry
    return getPtrFromVariableName(registerName);
}

void* IntervalTracker::compareValues(CmpInst* i) {
    return valueTracker.compareValues(i);
}

void* IntervalTracker::processCalculation(BinaryOperator* i) {
    valueTracker.processCalculation(i);

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
    return getPtrFromVariableName(variable.first);
}

IntervalTracker::var_t IntervalTracker::calculateArithmetic(Instruction* i, arithmetic_function_t callback) {
    interval_t destInterval;
    for (auto val = i->value_op_begin(); val != i->value_op_end(); ++val) {
        std::string currentName;
        interval_t currentInterval;
        if (val->hasName()) {
            currentName = val->getName().str();
            currentInterval = intervalsTracker[currentName];
        }
        else if (ConstantInt* numConstant = dyn_cast<ConstantInt>(*val)) {
            int number = numConstant->getZExtValue();
            currentInterval = std::make_tuple(number, number);
        }
        else {
            std::stringstream registerValue;
            registerValue << *val;
            currentName = registerValue.str();
            currentInterval = intervalsTracker[currentName];
        }
        destInterval = (val == i->value_op_begin()) ? currentInterval : callback(destInterval, currentInterval);
    }
    std::string destName = i->getName().str();
    var_t calculatedInterval = std::make_pair(destName, destInterval);
    if (intervalsTracker.find(destName) == intervalsTracker.end()) {
        intervalsTracker.insert(calculatedInterval);
    }
    else {
        intervalsTracker[destName] = destInterval;
    }

    // Returns reference to recently modified entry
    return calculatedInterval;
}

IntervalTracker::interval_t IntervalTracker::addCallback(interval_t accumulator, interval_t current) {
    double min = std::get<0>(accumulator);
    double max = std::get<1>(accumulator);
    double currentMin = std::get<0>(current);
    double currentMax = std::get<1>(current);
    double resultMin = 0.0;
    double resultMax = 0.0;
    if (!isInLoop()) {
        resultMin = (std::isnan(currentMin) || std::isnan(min)) ? std::nan("-infinity") : (min + currentMin);
        resultMax = (std::isnan(currentMax) || std::isnan(max)) ? std::nan("+infinity") : (max + currentMax);
    }
    else {
        resultMin = (std::isnan(max)) ? min : max;
        resultMin = (std::isnan(resultMin)) ? std::nan("-infinity") : resultMin;
        resultMax = std::nan("+infinity");
    }
    return std::make_tuple(resultMin, resultMax);
}

IntervalTracker::interval_t IntervalTracker::subCallback(interval_t accumulator, interval_t current) {
    double min = std::get<0>(accumulator);
    double max = std::get<1>(accumulator);
    double currentMin = std::get<0>(current);
    double currentMax = std::get<1>(current);
    double resultMin = 0.0;
    double resultMax = 0.0;
    if (!isInLoop()) {
        resultMin = (std::isnan(currentMin) || std::isnan(min)) ? std::nan("-infinity") : (min - currentMin);
        resultMax = (std::isnan(currentMax) || std::isnan(max)) ? std::nan("+infinity") : (max - currentMax);
    }
    else {
        resultMin = std::nan("-infinity");
        resultMax = (std::isnan(min)) ? max : min;
        resultMax = (std::isnan(resultMax)) ? std::nan("+infinity") : resultMax;
    }
    return std::make_tuple(resultMin, resultMax);
}

IntervalTracker::interval_t IntervalTracker::mulCallback(interval_t accumulator, interval_t current) {
    double min = std::get<0>(accumulator);
    double max = std::get<1>(accumulator);
    double currentMin = std::get<0>(current);
    double currentMax = std::get<1>(current);
    double resultMin = 0.0;
    double resultMax = 0.0;
    if (!isInLoop()) {
        resultMin = (std::isnan(currentMin) || std::isnan(min)) ? std::nan("-infinity") : (min * currentMin);
        resultMax = (std::isnan(currentMax) || std::isnan(max)) ? std::nan("+infinity") : (max * currentMax);
    }
    else {
        if (((min < 1) && (min > 0)) ||
            ((max < 1) && (max > 0)) ||
            ((currentMin < 1) && (currentMin > 0)) ||
            ((currentMax < 1) && (currentMax > 0))) {
            resultMin = 0;
            resultMax = max;
        }
        else if (((min > -1) && (min < 0)) ||
                ((max > -1) && (max < 0)) ||
                ((currentMin > -1) && (currentMin < 0)) ||
                ((currentMax > -1) && (currentMax < 0))) {
            resultMin = min;
            resultMax = 0;
        }
        else if (((min >= 1) && (max >= 1) && (currentMin >= 1) && (currentMax >= 1)) ||
                 ((min <= -1) && (max <= -1) && (currentMin <= -1) && (currentMax <= -1))) {
            resultMin = min;
            resultMax = std::nan("+infinity");
        }
        else if (((min >= 1) && (max >= 1) && (currentMin <= 1) && (currentMax <= 1)) ||
                 ((min <= -1) && (max <= -1) && (currentMin >= -1) && (currentMax >= -1))) {
            resultMin = std::nan("-infinity");
            resultMax = max;
        }
        else {
            resultMin = min;
            resultMax = max;
        }
    }
    return std::make_tuple(resultMin, resultMax);
}

IntervalTracker::interval_t IntervalTracker::sremCallback(interval_t accumulator, interval_t current) {
    double currentMin = std::get<0>(current);
    double currentMax = std::get<1>(current);
    double resultMin = (std::isnan(currentMin)) ? std::nan("-infinity") : 0;
    double resultMax = (std::isnan(currentMax)) ? std::nan("+infinity") : currentMax;
    return std::make_tuple(resultMin, resultMax);
}
