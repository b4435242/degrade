#include <iostream>
#include <sstream>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <fstream>
#include <vector>

#include "../AngelixCommon.h"
#include "SMTLIB2.h"

std::vector<int> vulnerableLocation;
std::vector<int> restoreLocation;

enum VarTypes { ALL, INT, INT_AND_PTR };


bool suitableVarDecl(VarDecl* vd, VarTypes collectedTypes) {
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

std::pair< std::unordered_set<VarDecl*>, std::unordered_set<MemberExpr*> > collectVarsFromScope(const DynTypedNode node, ASTContext* context, unsigned line, Rewriter &Rewrite, VarTypes collectedTypes) {
  //VarTypes collectedTypes = INT_AND_PTR;
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
        if (suitableVarDecl(vd, collectedTypes)) {
          var_set.insert(vd);
        }
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
              if (line > beginLine && suitableVarDecl(vd, collectedTypes)) {
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
            if ((vd = cast<VarDecl>(dref->getDecl())) != NULL && suitableVarDecl(vd, collectedTypes)) {
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
              if (line > beginLine && vd->hasInit() && suitableVarDecl(vd, collectedTypes)) {
                var_set.insert(vd);
              } else {
                //if (getenv("ANGELIX_INIT_UNINIT_VARS")) {
                  if (line > beginLine && suitableVarDecl(vd, collectedTypes)) {
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
      std::pair< std::unordered_set<VarDecl*>, std::unordered_set<MemberExpr*> > parent_vars = collectVarsFromScope(parent, context, line, Rewrite, collectedTypes);
      var_set.insert(parent_vars.first.cbegin(), parent_vars.first.cend());
      member_set.insert(parent_vars.second.cbegin(), parent_vars.second.cend());
    }
    std::pair< std::unordered_set<VarDecl*>, std::unordered_set<MemberExpr*> > result(var_set, member_set);
    return result;
  }
}

void parse_locations(const char** argv, int offset){
	int count = offset;
	
	for(int i=0; i<4; i++)
		vulnerableLocation.push_back(atoi(argv[count++]));
	for(int i=0; i<4; i++) 
		restoreLocation.push_back(atoi(argv[count++]));
	
}

bool isSuspicious(int beginLine, int beginCol, int endLine, int endCol) {
 	return (vulnerableLocation[0]==beginLine && 
		vulnerableLocation[1]==beginCol && 
		vulnerableLocation[2]==endLine && 
		vulnerableLocation[3]==endCol);
  
}


class FunctionFinderVisitor: public RecursiveASTVisitor<FunctionFinderVisitor> {
public:
	FunctionFinderVisitor(ASTContext *Context, Rewriter &R): Context(Context),Rewrite(R) {}

	bool VisitFunctionDecl(FunctionDecl *funcDecl) {
    	// Check if the given line and column fall within the source range of the function declaration.
    	if (!funcDecl->hasBody()) 
			return true;
		Stmt* funcBody = funcDecl->getBody();
		SourceManager &srcMgr = Rewrite.getSourceMgr();
		int beginLine = srcMgr.getExpansionLineNumber(funcBody->getBeginLoc());
		int endLine = srcMgr.getExpansionLineNumber(funcBody->getEndLoc());
		if (restoreLocation[0]>beginLine && restoreLocation[2]<endLine){
        	// The given line and column fall within the function declaration.
       		FunctionName = funcDecl->getNameInfo().getAsString();
        	FunctionCall = FunctionName + "(";

        	// Add the function arguments to the string.
        	for (int i = 0, e = funcDecl->getNumParams(); i != e; ++i) {
            	if (i > 0)
                	FunctionCall += ", ";
            	FunctionCall += "0";
        	}

        	FunctionCall += ")";

      		return true;
    	}
    	return true;
  	}

	std::string getFunctionName() const { return FunctionName; }
	std::string getFunctionCall() const { return FunctionCall; }


private:
	ASTContext *Context;
	Rewriter &Rewrite;
	std::string FunctionName;
	std::string FunctionCall;
};

class MainFunctionRewriter : public MatchFinder::MatchCallback {
public:
	MainFunctionRewriter(Rewriter &Rewrite) : Rewrite(Rewrite) {}

	virtual void run(const MatchFinder::MatchResult &Result) {
		// Get the main function declaration from the match result
		if (const FunctionDecl *FD = Result.Nodes.getNodeAs<FunctionDecl>("mainFunctionDecl")) {
			// Check if the function has a body (definition)
			if (FD->hasBody()) {
				// Replace the entire function body with a single statement
				std::ostringstream harnessStream;
				harnessStream << "{ " << restoreFunctionCall << "; }\n";
				Rewrite.ReplaceText(FD->getBody()->getSourceRange(), harnessStream.str());
			}
		}
	} 
	
	void setFunction(std::string& fc){
		restoreFunctionCall = fc;
	} 

private:
	Rewriter &Rewrite;
	std::string restoreFunctionCall;
};

class RestoreStmtVisitor : public clang::RecursiveASTVisitor<RestoreStmtVisitor> {
public:
	RestoreStmtVisitor(Rewriter &Rewrite, ASTContext* Context): Rewrite(Rewrite), Context(Context) {}

	bool VisitStmt(Stmt *S) {
    	// This method will be called for every statement in the function.
    	// You can add code here to process the statement.
    	// Check if the function and statement match the target function and location

		// Get the function declaration and statement from the match result
		SourceManager &srcMgr = Rewrite.getSourceMgr();
		int beginLine = srcMgr.getExpansionLineNumber(S->getBeginLoc());
		int beginCol = srcMgr.getExpansionColumnNumber(S->getBeginLoc());
		int endLine = srcMgr.getExpansionLineNumber(S->getEndLoc());
		int endCol = srcMgr.getExpansionColumnNumber(S->getEndLoc());
		if (endLine<restoreLocation[0] || (endLine==restoreLocation[0] && endCol<restoreLocation[1]) ) {
			// Delete the statement from the source code
			Rewrite.RemoveText(S->getSourceRange());
			return true;
		}
		else if (beginLine==restoreLocation[0] && beginCol==restoreLocation[1] && endLine==restoreLocation[2] && endCol==restoreLocation[3]){
		
			const DynTypedNode node = DynTypedNode::create(*S);	
			enum VarTypes collectedTypes = ALL;
			std::pair< std::unordered_set<VarDecl*>, std::unordered_set<MemberExpr*>> varsFromScope = collectVarsFromScope(node, Context, beginLine, Rewrite, collectedTypes);
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

			std::ostringstream stringStream;
			stringStream << "DG_RESTORE("
						 << beginLine << ", "
						 << beginCol << ", "
						 << endLine << ", "
						 << endCol << ", "
						 << "((char*[]){" << nameStream.str() << "}), "
                  		 << "((char*[]){" << typeStream.str() << "}), "
                   		 << "((void*[]){" << exprStream.str() << "}), "
                   		 << "((int[]){" << sizeStream.str() << "}), "
                   		 << size
						 << ")";
			Rewrite.ReplaceText(S->getSourceRange(), stringStream.str());
			return false;
		}
		return true;
	}

private:
	Rewriter &Rewrite;
	ASTContext* Context;
};

class RestoreFunctionRewriter : public MatchFinder::MatchCallback {
public:
	RestoreFunctionRewriter(Rewriter &R): Rewrite(R) {}
	
	virtual void run(const MatchFinder::MatchResult &Result) {
		RestoreStmtVisitor restoreStmtVisitor(Rewrite, Result.Context);	
    	if (const FunctionDecl *funcDecl = Result.Nodes.getNodeAs<FunctionDecl>("restoreFunctionDecl")) {
      		// Create a RestoreStmtVisitor to traverse the statements in the function.
    		restoreStmtVisitor.TraverseStmt(funcDecl->getBody());
    	}
	}

private:
	Rewriter &Rewrite;
	//RestoreStmtVisitor restoreStmtVisitor;
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

      if (! isSuspicious(beginLine, beginColumn, endLine, endColumn)) {
        return;
      }

      std::cout << beginLine << " " << beginColumn << " " << endLine << " " << endColumn << "\n"
                << toString(stmt) << "\n";

      std::ostringstream stmtId;
      stmtId << beginLine << "-" << beginColumn << "-" << endLine << "-" << endColumn;
      std::string extractedDir(getenv("ANGELIX_EXTRACTED"));
      std::ofstream fs(extractedDir + "/" + stmtId.str() + ".smt2");
      fs << "(assert (not (= 1 0)))\n";

      const DynTypedNode node = DynTypedNode::create(*stmt);
	  enum VarTypes collectedTypes = INT_AND_PTR;
      std::pair< std::unordered_set<VarDecl*>, std::unordered_set<MemberExpr*> > varsFromScope = collectVarsFromScope(node, Result.Context, beginLine, Rewrite, collectedTypes);
      std::unordered_set<VarDecl*> vars;
      vars.insert(varsFromScope.first.begin(), varsFromScope.first.end());
      //FIXME: why no members here?
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

      int size = vars.size();
      std::string indent(beginColumn-1, ' ');

      std::ostringstream stringStream;
      stringStream << "if ("
                   << "ANGELIX_CHOOSE("
                   << "bool" << ", "
                   << 1 << ", "
                   << beginLine << ", "
                   << beginColumn << ", "
                   << endLine << ", "
                   << endColumn << ", "
                   << "((char*[]){" << nameStream.str() << "}), "
                   << "((int[]){" << exprStream.str() << "}), "
                   << size
                   << ")"
                   << ") "
                   << toString(stmt)
                   << ";\n"
                   << indent
                   << "else "
                   << "return 1";
      std::string replacement = stringStream.str();

      Rewrite.ReplaceText(expandedLoc, replacement);
    }
  }

private:
  Rewriter &Rewrite;

};


class MyASTConsumer : public ASTConsumer {
public:
	MyASTConsumer(Rewriter &R) : HandlerForStatements(R), 
								 HandlerForMainFunction(R),
								 HandlerForRestoreFunction(R),
								 Visitor(NULL, R)	
	{							 		
		Finder.addMatcher(InterestingStatement, &HandlerForStatements);
		Finder.addMatcher(MainFunctionMatcher, &HandlerForMainFunction);
		
	}

	void HandleTranslationUnit(ASTContext &Context) override {
		
    	Visitor.TraverseDecl(Context.getTranslationUnitDecl());
		std::string restoreFunctionName = Visitor.getFunctionName();
		std::string restoreFunctionCall = Visitor.getFunctionCall();
		
		HandlerForMainFunction.setFunction(restoreFunctionCall);
		
		DeclarationMatcher restoreFunctionMatcher = functionDecl(hasName(restoreFunctionName)).bind("restoreFunctionDecl");
		Finder.addMatcher(restoreFunctionMatcher, &HandlerForRestoreFunction);


		Finder.matchAST(Context);
		
	}

private:
	StatementHandler HandlerForStatements;
	MainFunctionRewriter HandlerForMainFunction;	
	RestoreFunctionRewriter HandlerForRestoreFunction; 

	MatchFinder Finder;
	
	FunctionFinderVisitor Visitor;
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
	parse_locations(argv, 2);
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
