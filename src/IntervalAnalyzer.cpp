#include <string>
#include <tuple>
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Constants.h"
#include "IntervalAnalyzer.h"
#include "IntervalTracker.h"

IntervalAnalyzer::IntervalAnalyzer(std::string varName) {
    variableName = varName;
    interval = std::make_tuple(std::nan("-infinity"), std::nan("+infinity"));
}

IntervalTracker::interval_t IntervalAnalyzer::processNewInstruction(Instruction* i) {
    IntervalTracker::processNewEntry(i);
    return getInterval();
}

void IntervalAnalyzer::printIntervalReport() {
    const char* varName = variableName.c_str();
    double varValue = IntervalTracker::getVariableValue(variableName);
    IntervalTracker::interval_t variableInterval = getInterval();
    double min = std::get<0>(variableInterval);
    double max = std::get<1>(variableInterval);
    std::string minString = (std::isnan(min)) ? "-infinity" : std::to_string(min);
    std::string maxString = (std::isnan(max)) ? "+infinity" : std::to_string(max);
    const char* minText = minString.c_str();
    const char* maxText = maxString.c_str();
    printf("Interval of variable %s = [ %s , %s] --- True value = %lf", varName, minText, maxText, varValue);
    printf("\n");
}

void IntervalAnalyzer::printIntervalTracker() {
    IntervalTracker::printTracker();
}

IntervalTracker::interval_t IntervalAnalyzer::getInterval() {
    interval = IntervalTracker::getVariableInterval(variableName);
    return interval;
}
