#include <backend/codegenpublic.h>

#include <frontend/irtranslation.h>
#include <frontend/PrintVisitor.h>
#include <frontend/genIL.h>
#include <frontend/genllvm.h>
#include <frontend/findentryfunc.h>

#include "unistd.h"
#include <common/debug.h>
#include <midend/optimizeIR.h>
#include "Dot/dotwriter.h"

#include <fstream>
#include <cstdlib>
#include <iostream>
#include <cstdio>
#include <string>

#include <common/llvm_warnings_push.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/PassManager.h>
#include <common/llvm_warnings_pop.h>

extern IcarusModule* ParseFile(const char *filename); //using this for now. need to create a standard header file for lex

static Trace& gTrace = Trace::getInstance();
static Debug& gDebug = Debug::getInstance();

using namespace std;

//global variables
IcarusModule *pIcarusModule;

llvm::Function* findEntryFunc(llvm::Module& M)
{
    llvm::PassManager mpm;
    FindEntryFunc* pEntryFunc = new FindEntryFunc();
    mpm.add(pEntryFunc);
    mpm.run(M);
    return pEntryFunc->GetEntryFunc();
}

void genExecutable(CodeGenModule& module, std::string filename)
{
    GenerateCode(module, filename);
}

int Compile(char *fileName, char *pOutputFileName)
{
    gTrace <<"Compiling file: " << fileName;
    pIcarusModule = ParseFile(fileName);
    
    if(pIcarusModule == nullptr)
        return -1; //there was some syntax error. Hence we skip all other checks

    GenIL *myILGen = new GenIL(*pIcarusModule);
    pIcarusModule = myILGen->generateIL();

    if(gDebug.isDotGen())
    {
        DotWriter d;
        std::string filename = "postgenIL.dot";
        d.writeDotFile(filename, *pIcarusModule);
    }

    GenLLVM genLLVM;
    genLLVM.generateLLVM(*pIcarusModule);
    llvm::Module& llvmModule = genLLVM.getModule();

    // Find the Entry Function
    llvm::Function* pEntryFunc = findEntryFunc(llvmModule);

    //Dispose old module
    if(pIcarusModule != nullptr)
    {
        delete pIcarusModule;
        pIcarusModule = nullptr;
    }

    g_outputStream.stream() << "-------------------------------------------------------------------" << std::endl;
    g_outputStream.stream() << "Before Optimization" << std::endl;
    g_outputStream.stream() << "-------------------------------------------------------------------" << std::endl;
    g_outputStream.flush();
    if (gDebug.isDebuggable())
    {
        llvmModule.print(g_outputStream.raw_stream(), nullptr);
        g_outputStream.flush_raw_stream();
    }

    if(gDebug.isOptimizing())
    {
        OptimizeIR(llvmModule);
    }
    
    g_outputStream.stream() << "-------------------------------------------------------------------" << std::endl;
    g_outputStream.stream() << "After Optimization" << std::endl;
    g_outputStream.stream() << "-------------------------------------------------------------------" << std::endl;
    g_outputStream.flush();
    if(gDebug.isDebuggable())
    {
        llvmModule.print(g_outputStream.raw_stream(), nullptr);
        g_outputStream.flush_raw_stream();
    }

    std::string moduleStr;
    llvm::raw_string_ostream string(moduleStr);
    fstream moduleDumpFile;
    moduleDumpFile.open("temp.ll", fstream::in | fstream::out | fstream::trunc);
    if(moduleDumpFile.is_open())
    {
        llvmModule.print(string, NULL);
        moduleDumpFile<<moduleStr;
        moduleDumpFile.close();
    }

    CodeGenModule codeGenModule;
    codeGenModule.setLLVMModule(&llvmModule);
    codeGenModule.setLLVMEntryFunction(pEntryFunc);

    genExecutable(codeGenModule, std::string(pOutputFileName));
    
    return 0;
}

int main(int argc, char *argv[])
{
    gDebug.setDebug(true);
    gDebug.setTrace(false);
    gDebug.setYaccTrace(false);
    gDebug.setDotGen(true);
    gDebug.setCodeOptimization(true); //we need to allow setting levels

    gTrace << "Verbose on!\n";

    g_outputStream.addOutputStreamSubscriber(&g_consoleStreamSubscriber);
    g_outputStream.addOutputStreamSubscriber(&g_outputDebugStringSubscriber);

    char *pfilename = argv[1];
    char *pOutput = argv[2];

    assert(pfilename != nullptr && "No input file specified");
    assert(pOutput != nullptr && "No output file specified");

    Compile(pfilename, pOutput);
    system("pause");
    return 0;
}
