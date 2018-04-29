/**
 *  IntervalPass.cpp
 *
 *  Performs interval analysis on simple programs over an LLVM IR pass.
 *
 *  Created by CS5218 - Principles of Program Analysis
 *  Modified by Teekayu Klongtruajrok (A0174348X)
 *  Contact: e0210381@u.nus.edu
 */
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <unordered_map>
#include <set>
#include <stack>
#include <sstream>
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Constants.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"
#include "../include/Analyzer/IntervalAnalyzer.h"
#include "../include/Tracker/IntervalTracker.h"

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

#define DEPTH_SEPARATOR '/'
#define MAIN_FUNCTION "main"
#define LOOP_BEGIN_BLOCK_NAME "while.cond"
#define LOOP_END_BLOCK_NAME "while.end"

using namespace llvm;

enum AnalyzeLoopBackedgeSwtch {
    ON,
    OFF
};

typedef std::tuple<IntervalTracker::var_map_t, IntervalAnalyzer> analysis_package_t;

analysis_package_t generateCFG (BasicBlock*, IntervalAnalyzer* intervalAnalyzer, std::stack<BasicBlock*>, AnalyzeLoopBackedgeSwtch, std::string);
IntervalAnalyzer* analyzeInterval (BasicBlock*, IntervalAnalyzer* intervalAnalyzer);
IntervalTracker::var_map_t getLeafNodes(IntervalTracker::var_map_t);
void printIntervalReport(IntervalTracker::var_map_t);
bool isSameBlock (BasicBlock*, BasicBlock*);
bool isMainFunction (const char*);
bool isBeginLoop (const char*);
bool isEndLoop (const char*);
void printVars (std::set<Instruction*>);
void printInsts(std::set<Instruction*>);
void printLLVMValue (Value* v);

int main (int argc, char **argv) {
    // Read the IR file.
    LLVMContext Context;
    SMDiagnostic Err;
    std::unique_ptr<Module> M = parseIRFile(argv[1], Err, Context);
    if (M == nullptr) {
        fprintf(stderr, "error: failed to load LLVM IR file \"%s\"", argv[1]);
        return EXIT_FAILURE;
    }

    IntervalAnalyzer* intervalAnalyzer = new IntervalAnalyzer(argv[2]);

    for (auto &F: *M) {
        if (isMainFunction(F.getName().str().c_str())) {
            BasicBlock* BB = dyn_cast<BasicBlock>(F.begin());
            std::stack<BasicBlock*> loopCallStack;

            analysis_package_t analysisPackage = generateCFG(BB, intervalAnalyzer, loopCallStack, ON, "main");
            IntervalTracker::var_map_t variableIntervalEndpoints = std::get<0>(analysisPackage);
            IntervalTracker::var_map_t variableIntervalLeafNodes = getLeafNodes(variableIntervalEndpoints);
            printf("\nVar: %s Interval Report\n", argv[2]);
            printIntervalReport(variableIntervalEndpoints);
            printf("\n");
            printf("\n");
            printIntervalReport(variableIntervalLeafNodes);
        }
    }

    return 0;
}


analysis_package_t generateCFG (BasicBlock* BB,
                                IntervalAnalyzer* intervalAnalyzer,
                                std::stack<BasicBlock*> loopCallStack,
                                AnalyzeLoopBackedgeSwtch backedgeSwitch,
                                std::string parentContextName) {
  const char *blockName = BB->getName().str().c_str();
  std::string contextName = parentContextName + DEPTH_SEPARATOR + blockName;
  printf("Label Name:%s\n", blockName);

  // Create local copies of parameters that can be updated
  AnalyzeLoopBackedgeSwtch newBackedgeSwitch = backedgeSwitch;
  std::stack<BasicBlock*> newLoopCallStack = !loopCallStack.empty() ? std::stack<BasicBlock*>(loopCallStack) : std::stack<BasicBlock*>();

  // Track loop layer by pushing them into the stack
  if (isBeginLoop(blockName)) {
      newLoopCallStack.push(BB);
  }
  // Untrack the loop when a loop ends
  if (isEndLoop(blockName)) {
      newLoopCallStack.pop();
      // Turn back on to prepare for any outer loops
      newBackedgeSwitch = ON;
  }

  IntervalAnalyzer* newIntervalAnalyzer = analyzeInterval(BB, intervalAnalyzer);
  IntervalTracker::interval_t interval = newIntervalAnalyzer->getUpdatedInterval();
  IntervalTracker::var_map_t intervalEndpointTracker({{contextName, interval}});

  // Pass secretVars list to child BBs and check them
  const TerminatorInst *tInst = BB->getTerminator();
  int branchCount = tInst->getNumSuccessors();

  // Get the comparator variable to determine which branch to skip
  std::string branchComparatorName = tInst->getOperand(0)->getName().str();
  double branchComparatorValue = newIntervalAnalyzer->getVariableValue(branchComparatorName);

  printf("\n");
  IntervalAnalyzer propagatedIntervalAnalyzer(*newIntervalAnalyzer);
  for (int i = 0;  i < branchCount; ++i) {
      // Skip branch based on condition
      if (!std::isnan(branchComparatorValue) &&
          (i == branchComparatorValue)) {
              continue;
      }

      BasicBlock *next = tInst->getSuccessor(i);
      BasicBlock *prevLoopBegin = !newLoopCallStack.empty() ? newLoopCallStack.top() : nullptr;

      // If still analyzing loop backedge and the loop is going past the calling point, stop this recursion
      if (isEndLoop(next->getName().str().c_str()) &&
          (newBackedgeSwitch == OFF)) {
          return std::make_tuple(intervalEndpointTracker, propagatedIntervalAnalyzer);
      }
      // Analyze loop backedge by running through the loop one more time before ending
      // to complete taint analysis of variable dependencies
      if (isEndLoop(next->getName().str().c_str()) &&
          (newBackedgeSwitch == ON) &&
          !newLoopCallStack.empty()) {
          // prevent repeating backedge analysis loop
          newBackedgeSwitch = OFF;
          analysis_package_t analysisPackage = generateCFG(prevLoopBegin, &propagatedIntervalAnalyzer, newLoopCallStack, newBackedgeSwitch, contextName);
          IntervalTracker::var_map_t intervalEndpoint = std::get<0>(analysisPackage);
          intervalEndpointTracker.insert(intervalEndpoint.begin(), intervalEndpoint.end());
      }
      // Terminate looping condition to acheive least fixed point solution
      if (isSameBlock(prevLoopBegin, next)) {
          return std::make_tuple(intervalEndpointTracker, propagatedIntervalAnalyzer);
      }
      // Analyze the next instruction and get all the discovered from that analysis context,
      // isolating the consequence of the analyzer in that context from the outer context
      IntervalAnalyzer* subIntervalAnalyzer = new IntervalAnalyzer(*newIntervalAnalyzer);
      analysis_package_t analysisPackage = generateCFG(next, subIntervalAnalyzer, newLoopCallStack, newBackedgeSwitch, contextName);
      IntervalTracker::var_map_t intervalEndpoint = std::get<0>(analysisPackage);
      propagatedIntervalAnalyzer = IntervalAnalyzer(std::get<1>(analysisPackage));
      intervalEndpointTracker.insert(intervalEndpoint.begin(), intervalEndpoint.end());
  }

  return std::make_tuple(intervalEndpointTracker, propagatedIntervalAnalyzer);
}

IntervalAnalyzer* analyzeInterval (BasicBlock* BB, IntervalAnalyzer* intervalAnalyzer) {
    // Loop through instructions in BB
    IntervalAnalyzer::interval_t interval;
    for (auto &I: *BB) {
        interval = intervalAnalyzer->processNewInstruction(&I);
        intervalAnalyzer->printIntervalReport();
    }
    return intervalAnalyzer;
}

IntervalTracker::var_map_t getLeafNodes(IntervalTracker::var_map_t intervals) {
    int maxDepth = 0;
    for (auto& it :  intervals) {
        std::string contextName = it.first;
        size_t n = std::count(contextName.begin(), contextName.end(), DEPTH_SEPARATOR);
        int depth = static_cast<int>(n);
        maxDepth = (maxDepth < depth) ? depth : maxDepth;
    }

    IntervalTracker::var_map_t leafNodes;
    for (auto& it :  intervals) {
        std::string contextName = it.first;
        size_t n = std::count(contextName.begin(), contextName.end(), DEPTH_SEPARATOR);
        int depth = static_cast<int>(n);
        if (depth == maxDepth) {
            leafNodes.insert(it);
        }
    }
    return leafNodes;
}

void printIntervalReport(IntervalTracker::var_map_t intervals) {
    printf("\n");
    for (auto& it :  intervals) {
        std::string contextName = it.first;
        double min = std::get<0>(it.second);
        double max = std::get<1>(it.second);
        printf("Context: %s - [ %lf , %lf ]\n", contextName.c_str(), min, max);
    }
    printf("\n");
}

bool isSameBlock (BasicBlock* blockA, BasicBlock* blockB) {
    // Prevent nullptr reference
    if (!blockA || !blockB) {
        return false;
    }
    return blockA->getName().str() == blockB->getName().str();
}

bool isMainFunction (const char* functionName) {
    return strncmp(functionName, MAIN_FUNCTION, strlen(MAIN_FUNCTION)) == 0;
}

bool isBeginLoop (const char* instructionName) {
    return strncmp(instructionName, LOOP_BEGIN_BLOCK_NAME, strlen(LOOP_BEGIN_BLOCK_NAME)) == 0;
}

bool isEndLoop (const char* instructionName) {
    return strncmp(instructionName, LOOP_END_BLOCK_NAME, strlen(LOOP_END_BLOCK_NAME)) == 0;
}

void printVars (std::set<Instruction*> vars) {
    for (auto &S: vars) {
        printf("%s ", S->getName().str().c_str() );
    }
    printf("\n");
}

void printInsts(std::set<Instruction*> insts) {
    for (auto &S: insts) {
        printLLVMValue(dyn_cast<Value>(S));
    }
    printf("\n");
}

void printLLVMValue (Value* v) {
    std::string res;
    raw_string_ostream rso(res);
    v->print(rso, true);
    printf("%s\n", res.c_str());
}
