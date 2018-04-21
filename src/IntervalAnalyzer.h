#ifndef ITVATRK_H
#define ITVATRK_H

#include <string>
#include <tuple>
#include "llvm/IR/Instruction.h"
#include "IntervalTracker.h"

using namespace llvm;

class IntervalAnalyzer : public IntervalTracker {
    private:
        std::string variableName;
        IntervalTracker::interval_t interval;

    public:
        IntervalAnalyzer(std::string varName);
        IntervalTracker::interval_t processNewEntry(Instruction *i);
        void printIntervalReport();
        void printIntervalTracker();
        IntervalTracker::interval_t getInterval();
};

#endif
