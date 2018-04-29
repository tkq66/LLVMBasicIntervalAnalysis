#include <cstdio>
#include <iostream>
#include <string>
#include <sstream>
#include <unordered_map>
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Constants.h"
#include "../../include/Tracker/ValueTracker.h"

ValueTracker::var_t ValueTracker::getVariableFromPtr(void* ptr) {
    // TODO: Add some error handling if ptr == nullptr
    return *static_cast<var_t*>(ptr);
}

void* ValueTracker::getPtrFromVariableName(std::string name) {
    var_it_t variableIterator = variablesTracker.find(name);
    if (variableIterator == variablesTracker.end()) {
        return nullptr;
    }
    var_t variable = std::make_pair(variableIterator->first, variableIterator->second);
    void* elementPtr = &variable;
    return elementPtr;
}

double ValueTracker::getVariableValue(std::string name) {
    return (variablesTracker.find(name) != variablesTracker.end()) ? variablesTracker[name] : std::nan("undefined");
}

void ValueTracker::editVariable(std::string name, double value) {
    variablesTracker[name] = value;
}

ValueTracker::var_map_t ValueTracker::getValueTracker() const {
    return variablesTracker;
}

void ValueTracker::setTracker(var_map_t tracker) {
    variablesTracker = var_map_t(tracker);
}

void ValueTracker::printTracker() {
    for (auto variable = variablesTracker.begin(); variable != variablesTracker.end(); ++variable) {
        printf("Key: %s - Value: %lf\n", variable->first.c_str(), variable->second);
    }
    printf("\n");
}

void* ValueTracker::processNewEntry(Instruction* i) {
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
    return nullptr;
}

void* ValueTracker::allocateNewVariable(AllocaInst* i) {
    std::string varName;
    if (!i->hasName()) {
        return nullptr;
    }
    varName = i->getName().str();
    var_t variable = std::make_pair(varName, std::nan("inifinity"));
    variablesTracker.insert(variable);

    // Returns reference to newly created entry
    return getPtrFromVariableName(varName);
}

void* ValueTracker::storeValueIntoVariable(StoreInst* i) {
    double src;
    if (i->getOperand(0)->hasName()) {
        var_map_t::const_iterator existingVariable = variablesTracker.find(i->getOperand(0)->getName().str());
        src = existingVariable->second;
    }
    else {
        ConstantInt* ci = dyn_cast<ConstantInt>(i->getOperand(0));
        src = ci->getSExtValue();
    }
    std::string dest = i->getOperand(1)->getName().str();
    variablesTracker[dest] = src;

    // Returns reference to recently modified entry
    return getPtrFromVariableName(dest);
}

void* ValueTracker::loadVariableIntoRegister(LoadInst* i) {
    std::string variableName = i->getOperand(0)->getName().str();
    double variableValue = variablesTracker[variableName];
    std::stringstream registerValue;
    registerValue << (void*)i;
    std::string registerString = registerValue.str();
    var_t registerVariable = std::make_pair(registerString, variableValue);
    variablesTracker.insert(registerVariable);

    // Returns reference to recently added register entry
    return getPtrFromVariableName(registerString);
}

void* ValueTracker::compareValues(CmpInst* i) {
    arithmetic_function_t comparison;
    switch (i->getPredicate()) {
        case CmpInst::ICMP_EQ:
            comparison = std::bind(&ValueTracker::equalToCallback, this, std::placeholders::_1, std::placeholders::_2);
            break;
        case CmpInst::ICMP_NE:
            comparison = std::bind(&ValueTracker::notEqualToCallback, this, std::placeholders::_1, std::placeholders::_2);
            break;
        case CmpInst::ICMP_UGT:
            comparison = std::bind(&ValueTracker::greaterThanCallback, this, std::placeholders::_1, std::placeholders::_2);
            break;
        case CmpInst::ICMP_UGE:
            comparison = std::bind(&ValueTracker::greaterThanOrEqualCallback, this, std::placeholders::_1, std::placeholders::_2);
            break;
        case CmpInst::ICMP_ULT:
            comparison = std::bind(&ValueTracker::lessThanCallback, this, std::placeholders::_1, std::placeholders::_2);
            break;
        case CmpInst::ICMP_ULE:
            comparison = std::bind(&ValueTracker::lessThanOrEqualCallback, this, std::placeholders::_1, std::placeholders::_2);
            break;
        case CmpInst::ICMP_SGT:
            comparison = std::bind(&ValueTracker::greaterThanCallback, this, std::placeholders::_1, std::placeholders::_2);
            break;
        case CmpInst::ICMP_SGE:
            comparison = std::bind(&ValueTracker::greaterThanOrEqualCallback, this, std::placeholders::_1, std::placeholders::_2);
            break;
        case CmpInst::ICMP_SLT:
            comparison = std::bind(&ValueTracker::lessThanCallback, this, std::placeholders::_1, std::placeholders::_2);
            break;
        case CmpInst::ICMP_SLE:
            comparison = std::bind(&ValueTracker::lessThanOrEqualCallback, this, std::placeholders::_1, std::placeholders::_2);
            break;
        default:
            printf("\nCompare Instruction with Predicate %d not supported.\n", i->getPredicate());
            break;
    }
    var_t variable = calculateArithmetic(i, comparison);

    // Returns reference to recently modified entry
    return getPtrFromVariableName(variable.first);
}

void* ValueTracker::processCalculation(BinaryOperator* i) {
    arithmetic_function_t calculation;
    switch (i->getOpcode()) {
        case Instruction::Add:
            calculation = std::bind(&ValueTracker::addCallback, this, std::placeholders::_1, std::placeholders::_2);
            break;
        case Instruction::Mul:
            calculation = std::bind(&ValueTracker::mulCallback, this, std::placeholders::_1, std::placeholders::_2);
            break;
        case Instruction::Sub:
            calculation = std::bind(&ValueTracker::subCallback, this, std::placeholders::_1, std::placeholders::_2);
            break;
        case Instruction::SRem:
            calculation = std::bind(&ValueTracker::sremCallback, this, std::placeholders::_1, std::placeholders::_2);
            break;
        default:
            break;
    }
    var_t variable = calculateArithmetic(i, calculation);

    // Returns reference to recently modified entry
    return getPtrFromVariableName(variable.first);
}

ValueTracker::var_t ValueTracker::calculateArithmetic(Instruction* i, arithmetic_function_t callback) {
    double destValue;
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
        destValue = (val == i->value_op_begin()) ? currentValue : callback(destValue, currentValue);
    }
    std::string destName = i->getName().str();
    var_t calculatedVariable = std::make_pair(destName, destValue);
    variablesTracker.insert(calculatedVariable);

    // Returns reference to recently modified entry
    return calculatedVariable;
}

double ValueTracker::addCallback(double accumulator, double current) {
    if (std::isnan(accumulator) ||
        std::isnan(current)) {
        return std::nan("inifinity");
    }
    else {
        return accumulator + current;
    }
}

double ValueTracker::subCallback(double accumulator, double current) {
    if (std::isnan(accumulator) ||
        std::isnan(current)) {
        return std::nan("inifinity");
    }
    else {
        return accumulator - current;
    }
}

double ValueTracker::mulCallback(double accumulator, double current) {
    if (std::isnan(accumulator) ||
        std::isnan(current)) {
        return std::nan("inifinity");
    }
    else {
        return accumulator * current;
    }
}

double ValueTracker::sremCallback(double accumulator, double current) {
    if (std::isnan(current)) {
        return std::nan("inifinity");
    }
    else {
        return current;
    }
}

double ValueTracker::equalToCallback(double accumulator, double current) {
    if (std::isnan(accumulator) ||
        std::isnan(current)) {
        return std::nan("undefined");
    }
    else {
        return (accumulator == current) ? 1.0 : 0.0;
    }
}

double ValueTracker::notEqualToCallback(double accumulator, double current) {
    if (std::isnan(accumulator) ||
        std::isnan(current)) {
        return std::nan("undefined");
    }
    else {
        return (accumulator != current) ? 1.0 : 0.0;
    }
}

double ValueTracker::greaterThanCallback(double accumulator, double current) {
    if (std::isnan(accumulator) ||
        std::isnan(current)) {
        return std::nan("undefined");
    }
    else {
        return (accumulator > current) ? 1.0 : 0.0;
    }
}

double ValueTracker::greaterThanOrEqualCallback(double accumulator, double current) {
    if (std::isnan(accumulator) ||
        std::isnan(current)) {
        return std::nan("undefined");
    }
    else {
        return (accumulator >= current) ? 1.0 : 0.0;
    }
}

double ValueTracker::lessThanCallback(double accumulator, double current) {
    if (std::isnan(accumulator) ||
        std::isnan(current)) {
        return std::nan("undefined");
    }
    else {
        return (accumulator < current) ? 1.0 : 0.0;
    }
}

double ValueTracker::lessThanOrEqualCallback(double accumulator, double current) {
    if (std::isnan(accumulator) ||
        std::isnan(current)) {
        return std::nan("undefined");
    }
    else {
        return (accumulator <= current) ? 1.0 : 0.0;
    }
}
