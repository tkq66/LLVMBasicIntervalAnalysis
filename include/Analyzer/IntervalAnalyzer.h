#ifndef ITVAANA_H
#define ITVAANA_H

#include <string>
#include <tuple>
#include "llvm/IR/Instruction.h"
#include "../Tracker/ValueTracker.h"
#include "../Tracker/IntervalTracker.h"

using namespace llvm;

class IntervalAnalyzer : public IntervalTracker {
    private:
        std::string variableName;
        IntervalTracker::interval_t interval;

    public:
        IntervalAnalyzer(std::string varName);
        IntervalAnalyzer(const IntervalAnalyzer& intervalAnalyzer);
        IntervalTracker::interval_t processNewInstruction(Instruction *i);
        void printIntervalReport();
        void printIntervalTracker();
        std::string getVariableName() const;
        IntervalTracker::interval_t getInterval() const;
        IntervalTracker::interval_t getUpdatedInterval();
};

#endif
