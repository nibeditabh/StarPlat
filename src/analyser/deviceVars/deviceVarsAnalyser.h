#ifndef DEVICE_VARS_ANALYSER
#define DEVICE_VARS_ANALYSER

#include "../../ast/ASTNodeTypes.hpp"
#include "../analyserUtil.cpp"
#include <unordered_map>

class lattice
{  
public:
  enum PointType{
    NOT_INITIALIZED,
    CPU_GPU_SHARED,
    GPU_ONLY,
    CPU_ONLY,
  };

  enum AccessType{
    CPU_READ,
    GPU_READ,
    CPU_WRITE,
    GPU_WRITE,
  };

  enum LatticeType{
    CPU_Preferenced,
    GPU_Preferenced,
  };

private:
  unordered_map<TableEntry*, PointType> typeMap;

public:
  lattice(){}

  lattice(list<Identifier*> &initVars){
    for(Identifier* iden: initVars)
      addVariable(iden);
  }

  lattice(const lattice &l1){
    for(pair<TableEntry*, PointType> pr: l1.typeMap)
      typeMap[pr.first] = pr.second;
  }

  //TODO : Needs to be changed
  PointType meet(PointType p1, PointType p2){
    return max(p1, p2);
  }

  void operator ^= (lattice &l1)
  {
    for(pair<TableEntry*, PointType> pr: l1.typeMap)
    {
      if(typeMap.find(pr.first) != typeMap.end())
        typeMap[pr.first] = meet(typeMap[pr.first], pr.second);
    }
  }

  lattice operator^ (const lattice &l1)
  {
    lattice out;
    for(pair<TableEntry*, PointType> pr: l1.typeMap)
    {
      if(typeMap.find(pr.first) != typeMap.end())
        out.typeMap[pr.first] = meet(typeMap[pr.first], pr.second);
    }
    return out;
  }

  void meet(Identifier* iden, AccessType acType)
  {
    TableEntry* symbInfo = iden->getSymbolInfo();
    if(typeMap.find(symbInfo) != typeMap.end())
    {
      PointType cType = typeMap.at(symbInfo);
      switch (cType)
      {
      case CPU_GPU_SHARED:
        switch (acType)
        {
        case CPU_READ:
          typeMap[symbInfo] = CPU_GPU_SHARED;
          break;
        case CPU_WRITE:
          typeMap[symbInfo] = CPU_ONLY;
          break;
        case GPU_READ:
          typeMap[symbInfo] = CPU_GPU_SHARED;
          break;
        case GPU_WRITE:
          typeMap[symbInfo] = GPU_ONLY;
          break;
        }
        break;
      
      case GPU_ONLY:
        switch (acType)
        {
        case CPU_READ:
          typeMap[symbInfo] = CPU_GPU_SHARED;
          break;
        case CPU_WRITE:
          typeMap[symbInfo] = CPU_ONLY;
          break;
        case GPU_READ:
          typeMap[symbInfo] = GPU_ONLY;
          break;
        case GPU_WRITE:
          typeMap[symbInfo] = GPU_ONLY;
          break;
        }
        break;
      
      case CPU_ONLY:
        switch (acType)
        {
        case CPU_READ:
          typeMap[symbInfo] = CPU_ONLY;
          break;
        case CPU_WRITE:
          typeMap[symbInfo] = CPU_ONLY;
          break;
        case GPU_READ:
          typeMap[symbInfo] = CPU_GPU_SHARED;
          break;
        case GPU_WRITE:
          typeMap[symbInfo] = GPU_ONLY;
          break;
        }
        break;
      }
    }
  }

  void addVariable(Identifier* iden, PointType type = NOT_INITIALIZED)
  {
    TableEntry* symbInfo = iden->getSymbolInfo();
    typeMap.insert({symbInfo, type});
  }

  void removeVariable(Identifier* iden)
  {
    TableEntry* symbInfo = iden->getSymbolInfo();
    typeMap.erase(symbInfo);
  }

  void print()
  {
    for(pair<TableEntry*, PointType> pr: typeMap)
    {
      string type;
      if(pr.second == CPU_GPU_SHARED)
        type = "CPU_GPU_SHARED";
      else if(pr.second == GPU_ONLY)
        type = "GPU_ONLY";
      else
        type = "CPU_ONLY";

      cout<<string(pr.first->getId()->getIdentifier())<<": "<<type<<' ';
    }
    cout<<endl;
  }

  void setType(LatticeType type){
    this->type = type;
  }

  void setVarType(Identifier* iden, PointType type){
    TableEntry* symbInfo = iden->getSymbolInfo();
    this->typeMap[symbInfo] = type;
  }

  unordered_map<TableEntry*, PointType> getLattice(){
    return typeMap;
  }
};

class ASTNodeWrap
{
  public:
  list<ASTNode*> predecessor, successor;
  usedVariables usedVars;
  lattice inMap, outMap;
  ASTNode* currStmt;
  bool hasForAll;
};

class deviceVarsAnalyser
{
  private:
  unordered_map<ASTNode*, ASTNodeWrap*> latticeMap;
  usedVariables gpuUsedVars;
  int tempVar = 0;

  ASTNodeWrap* initWrapNode(ASTNode* node, list<Identifier*> &vars)
  {
    ASTNodeWrap* wrapNode = new ASTNodeWrap();
    wrapNode->inMap = lattice(vars);
    wrapNode->outMap = lattice(vars);
    wrapNode->currStmt = node;
    return wrapNode;
  }

  ASTNodeWrap* getWrapNode(ASTNode* node){
    return latticeMap.at(node);
  }

  char* getTempVar()
  {
    string var = "tempVar_" + to_string(tempVar++);
    return var.c_str();
  }

  public:
  deviceVarsAnalyser(){
  }

  void analyse();
  void analyseFunc(ASTNode* proc);

  lattice analyseStatement(statement* stmt, lattice&);
  lattice analyseBlock(blockStatement* blockStmt, lattice&);
  lattice analyseUnary(unary_stmt* blockStmt, lattice&);
  lattice analyseIfElse(ifStmt* stmt, lattice&);
  lattice analyseAssignment(assignment* stmt, lattice&);
  lattice analyseDeclaration(declaration*, lattice&);
  lattice analyseForAll(forallStmt*, lattice&);
  lattice analyseWhile(whileStmt*, lattice&);
  lattice analyseDoWhile(dowhileStmt*, lattice&);
  lattice analyseFor(forallStmt*, lattice&);

  bool initBlock(blockStatement* blockStmt, list<Identifier*> &);
  bool initStatement(statement* stmt, list<Identifier*> &);
  bool initUnary(unary_stmt* blockStmt, list<Identifier*> &);
  bool initIfElse(ifStmt* stmt, list<Identifier*> &);
  bool initAssignment(assignment* stmt, list<Identifier*> &);
  bool initDeclaration(declaration*, list<Identifier*> &);
  bool initForAll(forallStmt*, list<Identifier*> &);
  bool initWhile(whileStmt*, list<Identifier*> &);
  bool initDoWhile(dowhileStmt*, list<Identifier*> &);
  bool initFor(forallStmt*, list<Identifier*> &);

  void printStatement(statement* , int);
  void printBlock(blockStatement* , int);
  void printUnary(unary_stmt* , int);
  void printIfElse(ifStmt* , int);
  void printAssignment(assignment* , int);
  void printDeclaration(declaration*, int);
  void printForAll(forallStmt*, int);

  usedVariables getVarsStatement(statement* stmt);
  usedVariables getVarsBlock(blockStatement* stmt);
  usedVariables getVarsForAll(forallStmt* stmt);
  usedVariables getVarsUnary(unary_stmt* stmt);
  usedVariables getVarsDeclaration(declaration* stmt);
  usedVariables getVarsWhile(whileStmt* stmt);
  usedVariables getVarsDoWhile(dowhileStmt* stmt);
  usedVariables getVarsAssignment(assignment* stmt);
  usedVariables getVarsIf(ifStmt* stmt);
  usedVariables getVarsExpr(Expression* stmt);

  statement* transferVarsStatement(statement* stmt,blockStatement* parBlock);
  statement* transferVarsForAll(forallStmt* stmt,blockStatement* parBlock);
  statement* transferVarsFor(forallStmt* stmt,blockStatement* parBlock);
  statement* transferVarsUnary(unary_stmt* stmt,blockStatement* parBlock);
  statement* transferVarsBlock(blockStatement* stmt,blockStatement* parBlock);
  statement* transferVarsDeclaration(declaration* stmt,blockStatement* parBlock);
  statement* transferVarsWhile(whileStmt* stmt,blockStatement* parBlock);
  statement* transferVarsDoWhile(dowhileStmt* stmt,blockStatement* parBlock);
  statement* transferVarsAssignment(assignment* stmt,blockStatement* parBlock);
  statement* transferVarsIf(ifStmt* stmt,blockStatement* parBlock);
};

#endif
