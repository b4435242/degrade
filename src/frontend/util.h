#ifndef DG_UTIL_H
#define DG_UTIL_H

#include <iostream>
#include <sstream>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <fstream>
#include <vector>

#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/ASTConsumers.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "llvm/Support/raw_ostream.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/AST/RecordLayout.h"


using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::tooling;
using namespace llvm;



#include "suspicious/SMTLIB2.h"

unsigned getDeclExpandedLine(const clang::Decl* decl, SourceManager &srcMgr);
bool insideMacro(const clang::Stmt* expr, SourceManager &srcMgr, const LangOptions &langOpts);
SourceRange getExpandedLoc(const clang::Stmt* expr, SourceManager &srcMgr);
std::string toString(const clang::Stmt* stmt);
bool overwriteMainChangedFile(Rewriter &TheRewriter);


// DG record/restore
//template<typename T>
//int getSize(T vd);

void getPtrInStruct(ASTContext* Context, std::unordered_set<VarDecl*> &vars, std::map<std::string, RecordDecl::field_iterator> &ptrs, std::map<std::string, std::string> &exprs, std::map<std::string, int>& sizes);

class StructVisitor : public RecursiveASTVisitor<StructVisitor> {
public:
	StructVisitor(ASTContext *Context) : Context(Context) {}
	bool VisitVarDecl(VarDecl* vd);
	bool VisitRecordDecl(RecordDecl* rd);

	template<typename T>
	void record_push(T decl);

	void record_pop();

	std::vector<std::string> get_nullsafe_expr_and_name(std::string name);
	std::string get_struct_name();
	template<typename T>
	RecordDecl* get_recordDecl(T decl);
private:
    ASTContext *Context;
	std::vector<std::string> name_v, sign_v;
public:
	std::map<std::string, RecordDecl::field_iterator> vars;
	std::map<std::string, std::string> exprs;
	std::map<std::string, int> sizes;
};


//



enum VarTypes { ALL, INT, INT_AND_PTR };

bool suitableVarDecl(VarDecl* vd, VarTypes collectedTypes);
bool isBooleanExpr(const Expr* expr);

std::unordered_set<VarDecl*> collectVarsFromExpr(const Stmt* stmt, VarTypes t);
std::unordered_set<MemberExpr*> collectMemberExprFromExpr(const Stmt* stmt);
std::unordered_set<ArraySubscriptExpr*> collectArraySubscriptExprFromExpr(const Stmt* stmt);
std::pair< std::unordered_set<VarDecl*>, std::unordered_set<MemberExpr*> > collectVarsFromScope(const DynTypedNode node, ASTContext* context, unsigned line, Rewriter &Rewrite, VarTypes collectedTypes);


class CollectVariables : public StmtVisitor<CollectVariables> {
  std::unordered_set<VarDecl*> *VSet;
  std::unordered_set<MemberExpr*> *MSet;
  std::unordered_set<ArraySubscriptExpr*> *ASet;
  VarTypes Types;

public:
  CollectVariables(std::unordered_set<VarDecl*> *vset,
                   std::unordered_set<MemberExpr*> *mset,
                   std::unordered_set<ArraySubscriptExpr*> *aset, VarTypes t): VSet(vset), MSet(mset), ASet(aset), Types(t) {}

  void Collect(Expr *E);
  void Visit(Stmt* S); 
  void VisitBinaryOperator(BinaryOperator *Node); 
  void VisitUnaryOperator(UnaryOperator *Node);
  void VisitImplicitCastExpr(ImplicitCastExpr *Node);
  void VisitParenExpr(ParenExpr *Node);
  void VisitIntegerLiteral(IntegerLiteral *Node);
  void VisitCharacterLiteral(CharacterLiteral *Node);

  void VisitMemberExpr(MemberExpr *Node);

  void VisitDeclRefExpr(DeclRefExpr *Node); 

  void VisitArraySubscriptExpr(ArraySubscriptExpr *Node); 


};


#endif
