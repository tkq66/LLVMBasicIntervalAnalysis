#ifndef ITVATRK_H
#define ITVATRK_H

#include <string>
#include <tuple>
#include "IntervalTracker.h"
#include "llvm/IR/Instruction.h"

using namespace llvm;

class IntervalAnalyzer {
    private:
        std::string variableName;
        IntervalTracker intervalTracker;
        std::tuple<double, double> interval;

    public:
        IntervalAnalyzer(std::string varName);
        std::tuple<double, double> processNewEntry(Instruction *i);
        void printIntervalReport();
        void printIntervalTracker();
        void printVariableTracker();
        double getInterval();
};

#endif
