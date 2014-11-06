// 15-745 S14 Assignment 2: liveness.h
// Group: akbhanda, zheq
///////////////////////////////////////////////////////////////////////////////

#ifndef __LIVENESS_H__
#define __LIVENESS_H__

#include "common/llvm_warnings_push.h"
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "common/llvm_warnings_pop.h"

#include "dataflow.h"
#include "util.h"

class Liveness : public DataFlowPass
{
 public:
  static char ID;
  Liveness();
  Assignments generate(const llvm::BasicBlock& block);
  Assignments kill(const llvm::BasicBlock& block);
  void transferFn(const Assignments& generate, const Assignments& kill,
    const Assignments& input, Assignments& output);
};

Liveness *createNewLivenessPass();

#endif
