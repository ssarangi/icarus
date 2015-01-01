#ifndef CODEGEN_H
#define CODEGEN_H

#include <common/icerr.h>
#include <frontend/IClassVisitor.h>
#include <frontend/CompEA.h>
#include <frontend/IcaType.h>

#include <common/llvm_warnings_push.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Module.h>
#include <common/llvm_warnings_pop.h>

#include <list>
#include <string>

using namespace std;

//definitions
/*
Everything should be a Value
a constant, an identifier, an expression, a function call etc , just like LLVM
*/

//forward declarations

class GenIL;
class GenLLVM;

//definitions
class IcaValue
{
public:
    virtual void accept(IClassVisitor &) {}
    virtual CompEA* codegen() = 0;
    virtual llvm::Value* genLLVM(GenLLVM*) = 0;
};

typedef std::vector<IcaValue*> IcaValueList;

class IcaExpression: public IcaValue 
{
public:
    virtual void accept(IClassVisitor &) = 0;
    virtual CompEA* codegen() = 0;
    virtual llvm::Value* genLLVM(GenLLVM*) = 0;
};

/*
Constant should have a data type and a value
*/
class IcaConstant: public IcaExpression 
{
public:

    IcaConstant(int value):m_value(value) {}
    
    virtual void accept(IClassVisitor &visitor)  { visitor.Visit(*this); }
    virtual CompEA* codegen();

    virtual llvm::Value* genLLVM(GenLLVM*);
    
    int getValue(){ return m_value; }
    
private:
    int m_value; //int for now. We need to change it 
    IcaConstant();
};

class IcaVariable: public IcaExpression 
{
public:
    IcaVariable(IcaSymbol& s)
        : m_symbol(s)
    {}

    //Getter-Setters
    IcaSymbol& getSymbol() { return m_symbol; }
    
    virtual void accept(IClassVisitor &visitor)  { visitor.Visit(*this); }
    virtual CompEA* codegen();
    virtual llvm::Value* genLLVM(GenLLVM*);
private:
    IcaSymbol& m_symbol;//check this
};

class IcaBinopExpression : public IcaExpression 
{
public:
    enum BinaryOperation
    { 
        Add,
        Sub,
        Mul,
        Div,
        EQ,
        NE,
        LT,
        GT,
        LTEQ,
        GTEQ
    };

    IcaBinopExpression(IcaValue& left,IcaValue& right, BinaryOperation op)
        : m_left(left)
        , m_right(right)
        , m_op(op)
    {}

    //Getter-Setters	
    IcaValue& getLeftValue() { return m_left; }
    IcaValue& getRightValue() { return m_right; }
    BinaryOperation getOperation() { return m_op; }

    //Visitors
    virtual void accept(IClassVisitor &visitor) { visitor.Visit(*this); }
    virtual CompEA* codegen();
    virtual llvm::Value* genLLVM(GenLLVM*);
private:
    IcaValue& m_left;
    IcaValue& m_right;
    BinaryOperation m_op;
    IcaBinopExpression();
};

class IcaFunctionCall : public IcaExpression 
{
public:
    IcaFunctionCall(IcaFunctionProtoType& fp, std::vector<IcaValue*>& params)
        : m_functionProtoType(fp)
        , m_paramList(params)
    {}

    //Getter-Setters
    IcaFunctionProtoType& getFunctionProtoType() { return m_functionProtoType; }
    std::vector<IcaValue*>& getParamList() { return m_paramList; }

    //Visitors
    virtual void accept(IClassVisitor &visitor)  { visitor.Visit(*this); }
    virtual CompEA* codegen();
    virtual llvm::Value* genLLVM(GenLLVM*);
private:
    IcaFunctionProtoType& m_functionProtoType;
    std::vector<IcaValue*> m_paramList;
};

class IcaStatement : public IcaValue
{
public:
    //Visitors
    virtual void accept(IClassVisitor &visitor) = 0;
    
private:
};

class IcaAssignment : public IcaStatement
{
public:
    IcaAssignment(IcaVariable& lVal, IcaValue& rVal)
        : m_lval(lVal)
        , m_rval(rVal)
    {}
    
    //Getter-Setters
    IcaVariable& getLVal() const { return m_lval; }
    IcaValue& getRVal() const { return m_rval; }

    //Visitors
    virtual void accept(IClassVisitor &visitor)  { visitor.Visit(*this); }
    virtual CompEA* codegen();
    virtual llvm::Value* genLLVM(GenLLVM*);
private:
    IcaVariable& m_lval;
    IcaValue& m_rval;
    IcaAssignment();
};

class IcaReturnStatement : public IcaStatement 
{
public:
    IcaReturnStatement(IcaFunction *pParent, IcaValue *pValue)
        : m_pParent(pParent)
    {
        if (pValue != nullptr)
            m_values.push_back(pValue);
    }

    IcaReturnStatement(IcaFunction *pParent, std::vector<IcaValue*>& values)
        : m_pParent(pParent)
    {
        m_values = values;
    }

    //Getter-Setters
    std::vector<IcaValue*> getReturnValue() { return m_values; }
    
    //Visitors
    virtual void accept(IClassVisitor &visitor)  { visitor.Visit(*this); }
    virtual CompEA* codegen();
    virtual llvm::Value* genLLVM(GenLLVM*);

private:
    std::vector<IcaValue*> m_values; // return statement can have NULL expression
    IcaFunction *m_pParent;
    IcaReturnStatement();
};

class IcaExpressionStatement : public IcaStatement 
{
public:
    IcaExpressionStatement(IcaExpression& expression)
        : m_expression(expression)
    {}

    IcaExpression& getExpression() { return m_expression; }

    //Visitors	
    virtual void accept(IClassVisitor &visitor)  { visitor.Visit(*this); }
    virtual CompEA* codegen();
    virtual llvm::Value* genLLVM(GenLLVM*);
private:
    IcaExpression& m_expression;
    IcaExpressionStatement();

};

class IcaBreakStatement : public IcaStatement 
{
public:
    //Visitors
    virtual void accept(IClassVisitor &visitor)  { visitor.Visit(*this); }
    virtual CompEA* codegen() { return nullptr; }
    virtual llvm::Value* genLLVM(GenLLVM*);
};

class IcaFunctionProtoType
{
public:
    IcaFunctionProtoType(const std::string& name, std::vector<IcaType*>& argTypeList, std::vector<IcaType*>& returnTypeList)
        : m_name(name)
        , m_argTypeList(argTypeList)
        , m_returnType(returnTypeList)
    {}

    //Getter-Setters
    std::string getName() const { return m_name; }
    std::vector<IcaType*>& getTypeList() { return m_argTypeList; }
    std::vector<IcaType*>& getReturnType() { return m_returnType; }

    //overloaded operators
    bool operator==(const IcaFunctionProtoType& fpOther) const;

    //Visitors
    virtual void accept(IClassVisitor &visitor)  { visitor.Visit(*this); }
    
private:
    std::string m_name;
    std::vector<IcaType*> m_argTypeList; //just store the datatypes; no need of names for the proto
    std::vector<IcaType*> m_returnType;
    IcaFunctionProtoType();
};

class IcaSymbol
{
public:
    IcaSymbol(std::string& name, IcaType& type)
        : m_name(name)
        , m_type(type)
    {}
    
    ~IcaSymbol();

    //Getter-Setters
    std::string getName() const { return m_name; }
    IcaType& getType() { return m_type; }

    //Visitors
    virtual void accept(IClassVisitor &visitor)  { visitor.Visit(*this); }
private:
    IcaSymbol();
    std::string m_name; //we need to add more details regarding the type
    IcaType m_type;
};

class IcaSymbolTable
{
public:
    //Getter-Setters
    IcErr add(IcaSymbol& sym);
    std::vector<IcaSymbol*>& getSymbols() { return m_symbols; }

    //Visitors
    virtual void accept(IClassVisitor &visitor)  { visitor.Visit(*this); }

private:
    std::vector<IcaSymbol*> m_symbols;
};

class IcaControlFlowStatement : public IcaStatement
{
public:
    IcaControlFlowStatement()
        : m_currentInsertStatement(NULL)
    {}
    
    virtual IcErr addStatement(IcaStatement& s) = 0;
    // return true if it accepted the request of ending a code block,
    // false if no code blocks were found to end
    virtual bool endCodeBlock();
    virtual IcaStatement* getCurrentStatement();
protected:
    IcaControlFlowStatement *m_currentInsertStatement;
};

//each branch in an if else
class IcaBranch 
{
public:
    IcaBranch(IcaExpression& condition)
        : m_condition(condition)
        , m_currentInsertStatement(NULL)
    {}

    std::vector<IcaStatement*>& getStatements() { return m_statementList; }
    IcErr addStatement(IcaStatement& s);
    bool endCodeBlock();
    virtual IcaStatement* getCurrentStatement();
    IcaExpression& getCondition() { return m_condition; }
private:
    IcaExpression& m_condition;
    std::vector<IcaStatement*> m_statementList;
    IcaControlFlowStatement* m_currentInsertStatement;
    IcaBranch();
    
};

//our if-else handler
class IcaBranchStatement : public IcaControlFlowStatement 
{
public:
    //an if-else will be like if (cond) do something else do something.
    //if else-if else will be if() do something else jump to end. if(second condition) else jump to end
    
    IcaBranchStatement(IcaExpression& condition);
    
    virtual CompEA* codegen();
    virtual llvm::Value* genLLVM(GenLLVM*);

    virtual IcErr addStatement(IcaStatement& s);
    virtual IcErr addBranch(IcaExpression& e);
    virtual bool endCodeBlock();
    virtual IcaStatement* getCurrentStatement();
    std::vector<IcaBranch*>& getBranches(){ return m_branches; }

    //Visitors
    virtual void accept(IClassVisitor &visitor)  { visitor.Visit(*this); }
private:
    std::vector<IcaBranch*> m_branches;
    IcaBranch* m_currentBranch;
    IcaBranchStatement();
};



class IcaWhileStatement : public IcaControlFlowStatement 
{
public:
    IcaWhileStatement(IcaExpression& condition)
        : m_condition(condition)
    {}

    virtual CompEA* codegen() { return nullptr; }
    virtual llvm::Value* genLLVM(GenLLVM*);

    virtual IcErr addStatement(IcaStatement& s);
    std::vector<IcaStatement*>& getStatements() { return m_statementList; }
    
    //Visitors
    virtual void accept(IClassVisitor &visitor)  { visitor.Visit(*this); }

    IcaExpression& getCondition() { return m_condition; }
private:
    IcaExpression& m_condition;
    std::vector<IcaStatement*> m_statementList;
    IcaWhileStatement();
};

class IcaFunction
{
public:
    IcaFunction(IcaFunctionProtoType& protoType, std::vector<IcaSymbol*>& argSymbolList)
        : m_protoType(protoType)
        , m_argSymbolList(argSymbolList)
        , m_symbolTable(*new IcaSymbolTable()), m_currentInsertStatement(NULL)
    {}

    ~IcaFunction();

    //Getter-Setters

    std::string getName() const { return m_protoType.getName(); }
    std::vector<IcaStatement*>& getStatements() { return m_statementList; }
    IcaFunctionProtoType& getProtoType() const { return m_protoType; }
    std::vector<IcaSymbol*>& getArgSymbolList() { return m_argSymbolList; }
    std::vector<IcaSymbol*>& getSymbols() { return m_symbolTable.getSymbols(); }
    IcaSymbolTable& getSymbolTable() { return m_symbolTable; }
    IcaStatement* getCurrentStatement();

    IcErr addStatement(IcaStatement& s);
    IcErr addSymbol(IcaSymbol& sym);

    bool endCodeBlock();

    virtual CompEA* codegen();
    virtual llvm::Value* genLLVM(GenLLVM*);

    //overloaded operators
    friend std::ostream& operator<<(std::ostream& stream, const IcaFunction& f);

    //Visitors
    virtual void accept(IClassVisitor &visitor)  { visitor.Visit(*this); }
private:
    std::vector<IcaStatement*> m_statementList;
    IcaFunctionProtoType& m_protoType;
    std::vector<IcaSymbol*> m_argSymbolList;
    IcaSymbolTable& m_symbolTable; // we should have a table for the function locals. this will get precedence over the global one
    IcaControlFlowStatement* m_currentInsertStatement;

    //prevent unintended c++ synthesis
    IcaFunction();
};

class IcarusModule
{
public:
    IcarusModule(const std::string& name);
    ~IcarusModule();
    //Getter-Setters

    std::string getName() const { return m_name; }
    std::vector<IcaFunction*>& getFunctions() { return m_functionList; }
    std::vector<IcaSymbol*>& getSymbols() { return m_symbolTable.getSymbols(); }
    IcaFunctionProtoType* getProtoType(const std::string name, std::vector<IcaType*>& dataTypes);
    IcaFunctionProtoType* getProtoType(const std::string name);//works till we allow overloading
    IcaFunction* getFunction(const std::string name); //need to add support for datatypes too

    IcErr addFunction(IcaFunction& f);
    IcErr addSymbol(IcaSymbol& s);
    IcErr addProtoType(IcaFunctionProtoType& fp);
    IcErr insertStatement(IcaFunction& f, IcaStatement& s);

    virtual CompEA* codegen();
    virtual llvm::Value* genLLVM(GenLLVM*);

    //overloaded operators
    friend std::ostream& operator<<(std::ostream& stream, const IcarusModule& m);

    //Visitors
    virtual void accept(IClassVisitor &visitor)  { visitor.Visit(*this); }
    
private:
    std::string m_name;
    std::vector<IcaFunction*> m_functionList;
    std::vector<IcaFunctionProtoType*> m_funcProtoList;
    IcaSymbolTable& m_symbolTable;
    IcarusModule();
};

#endif //codegen.h
