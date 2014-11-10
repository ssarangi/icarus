#include "midend/optimizeIR.h"

#include "midend/ConstantFolder.h"
#include "midend/DominanceTreeConstructor.h"
#include "midend/DominanceFrontier.h"
#include <midend/liveness.h>
#include <common/llvm_printer.h>

#include <common/llvm_warnings_push.h>
#include <llvm/PassManager.h>
#include <llvm/Pass.h>
#include <llvm/Transforms/Scalar.h>
#include <common/llvm_warnings_pop.h>

void OptimizeIR(llvm::Module& llvmModule)
{
    llvm::PassManager* pPassMgr;

    llvm::PassManager passMgr;

    PrintCFG *pCFG = new PrintCFG();
    pCFG->setOutputFile(std::string("cfg.dot"));

    //Analysis Passes
    passMgr.add(pCFG);
    passMgr.add(new DominanceTreeConstructor());
    passMgr.add(new DominanceFrontier());

    //Optimization Passes
    passMgr.add(new ConstantFolder());
    passMgr.add(llvm::createPromoteMemoryToRegisterPass());
    passMgr.run(llvmModule);
}