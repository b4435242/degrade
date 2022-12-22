#include <iostream>
#include <sstream>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <fstream>
#include <vector>

#include "../AngelixCommon.h"
#include "../suspicious/SMTLIB2.h"

std::vector<std::vector<int>> positions;

enum VarTypes { ALL, INT, INT_AND_PTR };


bool suitableVarDecl(VarDecl* vd, VarTypes collectedTypes) {
  std::cout << vd->getName().str() << " " << vd->getType().getAsString()<<std::endl;

  return (collectedTypes == ALL ||
          (collectedTypes == INT &&
           (vd->getType().getTypePtr()->isIntegerType() ||
            vd->getType().getTypePtr()->isCharType())) ||
          (collectedTypes == INT_AND_PTR &&
           (vd->getType().getTypePtr()->isIntegerType() ||
            vd->getType().getTypePtr()->isCharType() ||
            vd->getType().getTypePtr()->isPointerType())));
}


bool isBooleanExpr(const Expr* expr) {
  if (isa<BinaryOperator>(expr)) {
    const BinaryOperator* op = cast<BinaryOperator>(expr);
    std::string opStr = BinaryOperator::getOpcodeStr(op->getOpcode()).lower();
    return opStr == "==" || opStr == "!=" || opStr == "<=" || opStr == ">=" || opStr == ">" || opStr == "<" || opStr == "||" || opStr == "&&";
  }
  return false;
}


class CollectVariables : public StmtVisitor<CollectVariables> {
  std::unordered_set<VarDecl*> *VSet;
  std::unordered_set<MemberExpr*> *MSet;
  std::unordered_set<ArraySubscriptExpr*> *ASet;
  VarTypes Types;

public:
  CollectVariables(std::unordered_set<VarDecl*> *vset,
                   std::unordered_set<MemberExpr*> *mset,
                   std::unordered_set<ArraySubscriptExpr*> *aset, VarTypes t): VSet(vset), MSet(mset), ASet(aset), Types(t) {}

  void Collect(Expr *E) {
    if (E)
      Visit(E);
  }

  void Visit(Stmt* S) {
    StmtVisitor<CollectVariables>::Visit(S);
  }

  void VisitBinaryOperator(BinaryOperator *Node) {
    Collect(Node->getLHS());
    Collect(Node->getRHS());
  }

  void VisitUnaryOperator(UnaryOperator *Node) {
    Collect(Node->getSubExpr());
  }

  void VisitImplicitCastExpr(ImplicitCastExpr *Node) {
    Collect(Node->getSubExpr());
  }

  void VisitParenExpr(ParenExpr *Node) {
    Collect(Node->getSubExpr());
  }

  void VisitIntegerLiteral(IntegerLiteral *Node) {
  }

  void VisitCharacterLiteral(CharacterLiteral *Node) {
  }

  void VisitMemberExpr(MemberExpr *Node) {
    if (MSet) {
      MSet->insert(Node); // TODO: check memeber type?
    }
  }

  void VisitDeclRefExpr(DeclRefExpr *Node) {
    if (VSet && isa<VarDecl>(Node->getDecl())) {
      VarDecl* vd;
      if ((vd = cast<VarDecl>(Node->getDecl())) != NULL) {
        if (suitableVarDecl(vd, Types)) {
          VSet->insert(vd);
        }
      }
    }
  }

  void VisitArraySubscriptExpr(ArraySubscriptExpr *Node) {
    if (ASet) {
      ASet->insert(Node);
    }
  }


};


std::unordered_set<VarDecl*> collectVarsFromExpr(const Stmt* stmt, VarTypes t) {
  std::unordered_set<VarDecl*> set;
  CollectVariables T(&set, NULL, NULL, t);
  T.Visit(const_cast<Stmt*>(stmt));
  return set;
}


std::unordered_set<MemberExpr*> collectMemberExprFromExpr(const Stmt* stmt) {
  std::unordered_set<MemberExpr*> set;
  CollectVariables T(NULL, &set, NULL, ALL);
  T.Visit(const_cast<Stmt*>(stmt));
  return set;
}

std::unordered_set<ArraySubscriptExpr*> collectArraySubscriptExprFromExpr(const Stmt* stmt) {
  std::unordered_set<ArraySubscriptExpr*> set;
  CollectVariables T(NULL, NULL, &set, ALL);
  T.Visit(const_cast<Stmt*>(stmt));
  return set;
}

std::pair< std::unordered_set<VarDecl*>, std::unordered_set<MemberExpr*> > collectVarsFromScope(const DynTypedNode node, ASTContext* context, unsigned line, Rewriter &Rewrite) {
  VarTypes collectedTypes = INT_AND_PTR;
  /*if (getenv("ANGELIX_POINTER_VARIABLES")) {
    collectedTypes = INT_AND_PTR;
  } else {
    collectedTypes = INT;
  }*/
  
  //node.dump(llvm::outs(), const_cast<const ASTContext&>(*context));
  const FunctionDecl* fd;
  if ((fd = node.get<FunctionDecl>()) != NULL) {
    std::unordered_set<VarDecl*> var_set;
    std::unordered_set<MemberExpr*> member_set;
    //if (getenv("ANGELIX_FUNCTION_PARAMETERS")) {
      for (auto it = fd->param_begin(); it != fd->param_end(); ++it) {
        auto vd = cast<VarDecl>(*it);
		//if (suitableVarDecl(vd, collectedTypes)) {
          var_set.insert(vd);
        //}
      }
    //}

    //if (getenv("ANGELIX_GLOBAL_VARIABLES")) {
      clang::DynTypedNodeList parents = context->getParents(node);
      if (parents.size() > 0) {
        const DynTypedNode parent = *(parents.begin()); // TODO: for now only first
        const TranslationUnitDecl* tu;
        if ((tu = parent.get<TranslationUnitDecl>()) != NULL) {
          for (auto it = tu->decls_begin(); it != tu->decls_end(); ++it) {
            if (isa<VarDecl>(*it)) {
              VarDecl* vd = cast<VarDecl>(*it);
              unsigned beginLine = getDeclExpandedLine(vd, context->getSourceManager());
              if (line > beginLine /*&& suitableVarDecl(vd, collectedTypes)*/) {
                var_set.insert(vd);
              }
            }
          }
   }
      }
    //}
    std::pair< std::unordered_set<VarDecl*>, std::unordered_set<MemberExpr*> > result(var_set, member_set);
    return result;

  } else {

    std::unordered_set<VarDecl*> var_set;
    std::unordered_set<MemberExpr*> member_set;
    const CompoundStmt* cstmt;
    if ((cstmt = node.get<CompoundStmt>()) != NULL) {
      for (auto it = cstmt->body_begin(); it != cstmt->body_end(); ++it) {

        if (isa<BinaryOperator>(*it)) {
          BinaryOperator* op = cast<BinaryOperator>(*it);
          SourceRange expandedLoc = getExpandedLoc(op, context->getSourceManager());
          unsigned beginLine = context->getSourceManager().getExpansionLineNumber(expandedLoc.getBegin());
          if (line > beginLine &&
              BinaryOperator::getOpcodeStr(op->getOpcode()).lower() == "=" &&
              isa<DeclRefExpr>(op->getLHS())) {
            DeclRefExpr* dref = cast<DeclRefExpr>(op->getLHS());
            VarDecl* vd;
            if ((vd = cast<VarDecl>(dref->getDecl())) != NULL /*&& suitableVarDecl(vd, collectedTypes)*/) {
              var_set.insert(vd);
            }
          }
        }

        if (isa<DeclStmt>(*it)) {
          DeclStmt* dstmt = cast<DeclStmt>(*it);
          SourceRange expandedLoc = getExpandedLoc(dstmt, context->getSourceManager());
          unsigned beginLine = context->getSourceManager().getExpansionLineNumber(expandedLoc.getBegin());
          if (dstmt->isSingleDecl()) {
            Decl* d = dstmt->getSingleDecl();
            if (isa<VarDecl>(d)) {
              VarDecl* vd = cast<VarDecl>(d);
              if (line > beginLine && vd->hasInit() /*&& suitableVarDecl(vd, collectedTypes)*/) {
                var_set.insert(vd);
              } else {
                //if (getenv("ANGELIX_INIT_UNINIT_VARS")) { //local
                  if (line > beginLine /*&& suitableVarDecl(vd, collectedTypes)*/) {
                    std::ostringstream stringStream;
                    stringStream << vd->getType().getAsString() << " "
                                 << vd->getNameAsString()
                                 << " = 0;";
                    std::string replacement = stringStream.str();
                    Rewrite.ReplaceText(expandedLoc, replacement);
                    var_set.insert(vd);
                  }
                //}
              }
            }
          }
        }

        //if (getenv("ANGELIX_USED_VARIABLES")) {
          Stmt* stmt = cast<Stmt>(*it);
          SourceRange expandedLoc = getExpandedLoc(stmt, context->getSourceManager());
          unsigned beginLine = context->getSourceManager().getExpansionLineNumber(expandedLoc.getBegin());
          if (line > beginLine) {
            std::unordered_set<VarDecl*> varsFromExpr = collectVarsFromExpr(*it, collectedTypes);
            var_set.insert(varsFromExpr.begin(), varsFromExpr.end());

            //TODO: should be generalized for other cases:
            if (isa<IfStmt>(*it)) {
              IfStmt* ifStmt = cast<IfStmt>(*it);
              Stmt* thenStmt = ifStmt->getThen();
              if (isa<CallExpr>(*thenStmt)) {
                CallExpr* callExpr = cast<CallExpr>(thenStmt);
                for (auto a = callExpr->arg_begin(); a != callExpr->arg_end(); ++a) {
                  auto e = cast<Expr>(*a);
                  std::unordered_set<MemberExpr*> membersFromArg = collectMemberExprFromExpr(e);
                  member_set.insert(membersFromArg.begin(), membersFromArg.end());
                }
              }
            }
          }
        //}

      }
    }

    clang::DynTypedNodeList parents = context->getParents(node);
    if (parents.size() > 0) {
      const DynTypedNode parent = *(parents.begin()); // TODO: for now only first
      std::pair< std::unordered_set<VarDecl*>, std::unordered_set<MemberExpr*> > parent_vars = collectVarsFromScope(parent, context, line, Rewrite);
      var_set.insert(parent_vars.first.cbegin(), parent_vars.first.cend());
      member_set.insert(parent_vars.second.cbegin(), parent_vars.second.cend());
    }
    std::pair< std::unordered_set<VarDecl*>, std::unordered_set<MemberExpr*> > result(var_set, member_set);
    return result;
  }
}

void parse_position(int argc, const char** argv, int offset){
	int n = (argc-offset)/4;
	int count = offset;
	
	positions.resize(n, std::vector<int>(4));
	for(int i=0; i<n; i++)
		for(int j=0; j<4; j++)
			positions[i][j] = atoi(argv[count++]);	
	
}

bool isRecord(int beginLine, int beginCol, int endLine, int endCol) {
  for(auto &pos: positions)
	if (pos[0]==beginLine && pos[1]==beginCol && pos[2]==endLine && pos[3]==endCol)
	return true;
  
  return false;
}


class ExpressionHandler : public MatchFinder::MatchCallback {
public:
  ExpressionHandler(Rewriter &Rewrite, std::string t) : Rewrite(Rewrite), type(t) {}

  virtual void run(const MatchFinder::MatchResult &Result) override{
    if (const Expr *expr = Result.Nodes.getNodeAs<clang::Expr>("repairable")) {
      SourceManager &srcMgr = Rewrite.getSourceMgr();

      SourceRange expandedLoc = getExpandedLoc(expr, srcMgr);

      unsigned beginLine = srcMgr.getExpansionLineNumber(expandedLoc.getBegin());
      unsigned beginColumn = srcMgr.getExpansionColumnNumber(expandedLoc.getBegin());
      unsigned endLine = srcMgr.getExpansionLineNumber(expandedLoc.getEnd());
      unsigned endColumn = srcMgr.getExpansionColumnNumber(expandedLoc.getEnd());

      if (! isRecord(beginLine, beginColumn, endLine, endColumn)) {
        return;
      }

      std::cout << beginLine << " " << beginColumn << " " << endLine << " " << endColumn << "\n"
                << toString(expr) << "\n";

      std::ostringstream exprId;
      exprId << beginLine << "-" << beginColumn << "-" << endLine << "-" << endColumn;
      std::string extractedDir(getenv("ANGELIX_EXTRACTED"));
      std::ofstream fs(extractedDir + "/" + exprId.str() + ".smt2");
      fs << "(assert " << toSMTLIB2(expr) << ")\n";

      const DynTypedNode node = DynTypedNode::create(*expr);
      std::pair< std::unordered_set<VarDecl*>, std::unordered_set<MemberExpr*> > varsFromScope = collectVarsFromScope(node, Result.Context, beginLine, Rewrite);
      std::unordered_set<VarDecl*> varsFromExpr = collectVarsFromExpr(expr, ALL);
      std::unordered_set<MemberExpr*> memberFromExpr = collectMemberExprFromExpr(expr);
      std::unordered_set<ArraySubscriptExpr*> arraySubscriptFromExpr = collectArraySubscriptExprFromExpr(expr);
      std::unordered_set<VarDecl*> vars;
      vars.insert(varsFromScope.first.begin(), varsFromScope.first.end());
      vars.insert(varsFromExpr.begin(), varsFromExpr.end());
      std::unordered_set<MemberExpr*> members;
      members.insert(varsFromScope.second.begin(), varsFromScope.second.end());
      members.insert(memberFromExpr.begin(), memberFromExpr.end());
      std::ostringstream exprStream;
      std::ostringstream nameStream;
      bool first = true;
      for (auto it = vars.begin(); it != vars.end(); ++it) {
        if (first) {
          first = false;
        } else {
          exprStream << ", ";
          nameStream << ", ";
        }
        VarDecl* var = *it;
        exprStream << var->getName().str();
        nameStream << "\"" << var->getName().str() << "\"";
      }
      for (auto it = members.begin(); it != members.end(); ++it) {
        if (first) {
          first = false;
        } else {
          exprStream << ", ";
          nameStream << ", ";
        }
        MemberExpr* me = *it;
        exprStream << toString(me);
        nameStream << "\"" << toString(me) << "\"";
      }
      for (auto it = arraySubscriptFromExpr.begin(); it != arraySubscriptFromExpr.end(); ++it) {
        if (first) {
          first = false;
        } else {
          exprStream << ", ";
          nameStream << ", ";
        }
        ArraySubscriptExpr* ae = *it;
        exprStream << toString(ae->getLHS()) << "[" << toString(ae->getRHS()) << "]";
        nameStream << "\"" <<  toString(ae->getLHS()) << "_LBRSQR_" << toString(ae->getRHS()) << "_RBRSQR_" << "\"";
      }

      int size = vars.size() + members.size() + arraySubscriptFromExpr.size();

      std::ostringstream stringStream;
      stringStream << "ANGELIX_CHOOSE("
                   << type << ", "
                   << toString(expr) << ", "
                   << beginLine << ", "
                   << beginColumn << ", "
                   << endLine << ", "
                   << endColumn << ", "
                   << "((char*[]){" << nameStream.str() << "}), "
                   << "((int[]){" << exprStream.str() << "}), "
                   << size
                   << ")";
      std::string replacement = stringStream.str();

      Rewrite.ReplaceText(expandedLoc, replacement);
    }
  }

private:
  Rewriter &Rewrite;
  std::string type;

};




class StatementHandler : public MatchFinder::MatchCallback {
public:
  StatementHandler(Rewriter &Rewrite) : Rewrite(Rewrite) {}

  virtual void run(const MatchFinder::MatchResult &Result) override{
    if (const Stmt *stmt = Result.Nodes.getNodeAs<clang::Stmt>("repairable")) {
      SourceManager &srcMgr = Rewrite.getSourceMgr();

      SourceRange expandedLoc = getExpandedLoc(stmt, srcMgr);

      unsigned beginLine = srcMgr.getExpansionLineNumber(expandedLoc.getBegin());
      unsigned beginColumn = srcMgr.getExpansionColumnNumber(expandedLoc.getBegin());
      unsigned endLine = srcMgr.getExpansionLineNumber(expandedLoc.getEnd());
      unsigned endColumn = srcMgr.getExpansionColumnNumber(expandedLoc.getEnd());

      if (! isRecord(beginLine, beginColumn, endLine, endColumn)) {
        return;
      }

      /*std::cout << beginLine << " " << beginColumn << " " << endLine << " " << endColumn << "\n"
                << toString(stmt) << "\n";

      std::ostringstream stmtId;
      stmtId << beginLine << "-" << beginColumn << "-" << endLine << "-" << endColumn;
      std::string extractedDir(getenv("ANGELIX_EXTRACTED"));
      std::ofstream fs(extractedDir + "/" + stmtId.str() + ".smt2");
      fs << "(assert (not (= 1 0)))\n";*/

      const DynTypedNode node = DynTypedNode::create(*stmt);
      std::pair< std::unordered_set<VarDecl*>, std::unordered_set<MemberExpr*> > varsFromScope = collectVarsFromScope(node, Result.Context, beginLine, Rewrite);
      std::unordered_set<VarDecl*> vars;
      vars.insert(varsFromScope.first.begin(), varsFromScope.first.end());
      //FIXME: why no members here?
      std::ostringstream exprStream;
      std::ostringstream nameStream;
      std::ostringstream typeStream;
	  std::ostringstream sizeStream;
	  bool first = true;
      for (auto it = vars.begin(); it != vars.end(); ++it) {
        if (first) {
          first = false;
        } else {
          exprStream << ", ";
          nameStream << ", ";
		  typeStream << ", ";
		  sizeStream << ", ";
        }
        VarDecl* var = *it;
        exprStream << "&" << var->getName().str();
        nameStream << "\"" << var->getName().str() << "\"";
      	typeStream << "\"" << var->getType().getAsString() << "\"" ;
	  	sizeStream << var->getASTContext().getTypeInfo(var->getType()).Width/8;
	  }



      int size = vars.size();
      std::string indent(beginColumn-1, ' ');

      std::ostringstream stringStream;
      stringStream << toString(stmt)
                   << ";\n"
				   << indent
				   << "DG_RECORD("
                   << beginLine << ", "
                   << beginColumn << ", "
                   << endLine << ", "
                   << endColumn << ", "
                   << "((char*[]){" << nameStream.str() << "}), "
                   << "((char*[]){" << typeStream.str() << "}), "
				   << "((void*[]){" << exprStream.str() << "}), "
				   << "((int[]){" << sizeStream.str() << "}), "
                   << size
                   << ")";
      std::string replacement = stringStream.str();

      Rewrite.ReplaceText(expandedLoc, replacement);
    }
  }

private:
  Rewriter &Rewrite;

};


class MyASTConsumer : public ASTConsumer {
public:
  MyASTConsumer(Rewriter &R) : HandlerForIntegerExpressions(R, "int"),
                               HandlerForBooleanExpressions(R, "bool"),
                               HandlerForStatements(R) {	
	Matcher.addMatcher(RepairableAssignment, &HandlerForIntegerExpressions);
	Matcher.addMatcher(InterestingRepairableCondition, &HandlerForBooleanExpressions);
	Matcher.addMatcher(InterestingStatement, &HandlerForStatements);
  }

  void HandleTranslationUnit(ASTContext &Context) override {
    Matcher.matchAST(Context);
  }

private:
	ExpressionHandler HandlerForIntegerExpressions;
	ExpressionHandler HandlerForBooleanExpressions;
	StatementHandler HandlerForStatements;

  MatchFinder Matcher;
};


class InstrumentSuspiciousAction : public ASTFrontendAction {
public:
  InstrumentSuspiciousAction() {}

  void EndSourceFileAction() override {
    FileID ID = TheRewriter.getSourceMgr().getMainFileID();
    if (INPLACE_MODIFICATION) {
      //overwriteMainChangedFile(TheRewriter);
      TheRewriter.overwriteChangedFiles();
    } else {
      TheRewriter.getEditBuffer(ID).write(llvm::outs());
    }
  }

  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI, StringRef file) override {
    TheRewriter.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
    return std::make_unique<MyASTConsumer>(TheRewriter);
  }

private:
  Rewriter TheRewriter;
};


// Apply a custom category to all command-line options so that they are the only ones displayed.
static llvm::cl::OptionCategory MyToolCategory("angelix options");


int main(int argc, const char **argv) {
	// CommonOptionsParser constructor will parse arguments and create a
	// CompilationDatabase.  In case of error it will terminate the program.
	parse_position(argc, argv, 2);
	argc = 2;
	// clang 16
	//auto ExpectedParser = CommonOptionsParser::create(argc, argv, MyToolCategory);
	//CommonOptionsParser &OptionsParser = ExpectedParser.get();

	//clang 11
	CommonOptionsParser OptionsParser(argc, argv, MyToolCategory);

	// We hand the CompilationDatabase we created and the sources to run over into the tool constructor.
	ClangTool Tool(OptionsParser.getCompilations(), OptionsParser.getSourcePathList());

	return Tool.run(newFrontendActionFactory<InstrumentSuspiciousAction>().get());
}
