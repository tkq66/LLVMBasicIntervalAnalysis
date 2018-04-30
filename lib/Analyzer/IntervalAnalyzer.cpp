#include <string>
#include <tuple>
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Constants.h"
#include "../../include/Analyzer/IntervalAnalyzer.h"
#include "../../include/Tracker/IntervalTracker.h"
#include "../../include/Tracker/ValueTracker.h"

IntervalAnalyzer::IntervalAnalyzer(std::string varName) {
    variableName = varName;
    interval = std::make_tuple(std::nan("-infinity"), std::nan("+infinity"));
}

IntervalAnalyzer::IntervalAnalyzer(const IntervalAnalyzer& intervalAnalyzer) {
    variableName = intervalAnalyzer.getVariableName();
    interval = IntervalTracker::interval_t(intervalAnalyzer.getInterval());
    IntervalTracker::switchLoopState(intervalAnalyzer.IntervalTracker::isInLoop());
    ValueTracker::var_map_t vTracker = intervalAnalyzer.IntervalTracker::getValueTracker();
    IntervalTracker::var_map_t iTracker = intervalAnalyzer.IntervalTracker::getIntervalsTracker();
    IntervalTracker::setTracker(iTracker, vTracker);
}

IntervalTracker::interval_t IntervalAnalyzer::processNewInstruction(Instruction* i) {
    IntervalTracker::processNewEntry(i);
    interval = getUpdatedInterval();
    return interval;
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

std::string IntervalAnalyzer::getVariableName() const {
    return variableName;
}

IntervalTracker::interval_t IntervalAnalyzer::getInterval() const {
    return interval;
}

IntervalTracker::interval_t IntervalAnalyzer::getUpdatedInterval() {
    return IntervalTracker::interval_t(IntervalTracker::getVariableInterval(variableName));
}
