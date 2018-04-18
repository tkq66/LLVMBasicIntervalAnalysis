#ifndef SEPTRK_H
#define SEPTRK_H

#include <string>
#include <tuple>
#include "ValueTracker.h"
#include "llvm/IR/Instruction.h"

using namespace llvm;

class SeparationTracker {
    public:
        typedef std::tuple<std::string, std::string> var_pair_t;

        SeparationTracker(std::string varNameOne, std::string varNameTwo);
        double processNewEntry(Instruction *i);
        void printSeparationReport();
        void printVariableTracker();
        double calculateSeparation();
        double getSeparation();

    private:
        var_pair_t variableNames;
        ValueTracker valueTracker;
        double separation;
};

#endif
