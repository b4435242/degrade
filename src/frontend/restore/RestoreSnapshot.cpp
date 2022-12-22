#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "llvm/Support/CommandLine.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/ASTConsumers.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "llvm/Support/raw_ostream.h"

#include <sstream>

using namespace clang::driver;
using namespace clang::ast_matchers;
using namespace clang::tooling;
using namespace clang;

class FunctionFinderVisitor
    : public RecursiveASTVisitor<FunctionFinderVisitor> {
 public:
  FunctionFinderVisitor(ASTContext *Context, int Line, int Column)
      : Context(Context), Line(Line), Column(Column) {}

  bool VisitFunctionDecl(FunctionDecl *Declaration) {
    // Check if the given line and column fall within the source range of the
    // function declaration.
    SourceLocation Start = Declaration->getBeginLoc();
    SourceLocation End = Declaration->getEndLoc();
    if (Context->getSourceManager().isBeforeInTranslationUnit(Start, End) &&
    		Context->getSourceManager().isBeforeInTranslationUnit(
            Start, SourceLocation::getFromRawEncoding(Line)) &&
        Context->getSourceManager().isBeforeInTranslationUnit(
            SourceLocation::getFromRawEncoding(Line), End)) {
    	// The given line and column fall within the function declaration.
    	FunctionName = Declaration->getNameAsString();

		std::string FunctionCall = FunctionName + "(";

		// Add the function arguments to the string.
		for (int i = 0, e = Declaration->getNumParams(); i != e; ++i) {
  			if (i > 0)
    			FunctionCall += ", ";
  			FunctionCall += "0";
		}

		FunctionCall += ")"; 				

      return true;
    }
    return true;
  }

  std::string GetFunctionName() const { return FunctionName; }
  std::string GetFunctionCall() const { return FunctionCall; }


 private:
  ASTContext *Context;
  int Line;
  int Column;
  std::string FunctionName;
  std::string FunctionCall;
};

class MainFunctionRewriter : public MatchFinder::MatchCallback {
public:
  MainFunctionRewriter(Rewriter &Rewrite, std::string FunctionCall) : Rewrite(Rewrite), FakeFunctionCall(FunctionCall) {}

  virtual void run(const MatchFinder::MatchResult &Result) {
    // Get the main function declaration from the match result
    if (const FunctionDecl *FD = Result.Nodes.getNodeAs<FunctionDecl>("mainFunctionDecl")) {
      // Check if the function has a body (definition)
      if (FD->hasBody()) {
        // Replace the entire function body with a single statement
        Rewrite.ReplaceText(FD->getSourceRange(), FakeFunctionCall);
      }
    }
  }

private:
  Rewriter &Rewrite;
  std::string FakeFunctionCall;
};

class StmtDeleter : public MatchFinder::MatchCallback {
public:
  StmtDeleter(Rewriter &Rewrite, std::vector<int> TargetLocation)
      : Rewrite(Rewrite), TargetLocation(TargetLocation) {}

  virtual void run(const MatchFinder::MatchResult &Result) {
    // Get the function declaration and statement from the match result
    SourceManager &srcMgr = Rewrite.getSourceMgr();


	if (const FunctionDecl *FD = Result.Nodes.getNodeAs<FunctionDecl>("targetFunctionDecl")) {
      if (const Stmt *S = Result.Nodes.getNodeAs<Stmt>("stmt")) {
        // Check if the function and statement match the target function and location
        int beginLine = srcMgr.getExpansionLineNumber(S->getBeginLoc());
		int beginCol = srcMgr.getExpansionColumnNumber(S->getBeginLoc());
		int endLine = srcMgr.getExpansionLineNumber(S->getEndLoc());
		int endCol = srcMgr.getExpansionColumnNumber(S->getEndLoc());
		if (beginLine<TargetLocation[0] || (beginLine==TargetLocation[0] && beginCol<TargetLocation[1]) ) {
          // Delete the statement from the source code
          Rewrite.RemoveText(S->getSourceRange());
        }
		if (beginLine==TargetLocation[0] && beginCol==TargetLocation[1]){
			std::ostringstream stringStream;
			stringStream << "DG_RESTORE("
						 << beginLine << ", "
						 << beginCol << ", "
						 << endLine << ", "
						 << endCol
						 << ");\n";
			Rewrite.ReplaceText(S->getSourceRange(), stringStream.str());
		}
      }
    }
  }

private:
  Rewriter &Rewrite;
  std::vector<int> TargetLocation;
};

std::vector<std::string> get_targetFunction(ClangTool &Tool, int Line, int Column){
	// Run the tool and get the AST.
  ASTContext *Context = nullptr;
  FunctionFinderVisitor Visitor(Context, Line, Column);
  Tool.run(newFrontendActionFactory(&Visitor).get());

  // Get the function name from the visitor.
  std::vector<std::string> v(2);
  v[0] = Visitor.GetFunctionName();
  v[1] = Visitor.GetFunctionCall();
  return v;
}






static llvm::cl::OptionCategory MyToolCategory("dg options");

int main(int argc, const char **argv) {
  argc = 2;
  CommonOptionsParser OptionsParser(argc, argv, MyToolCategory);
  ClangTool Tool(OptionsParser.getCompilations(), OptionsParser.getSourcePathList());

	// arg
	int beginLine = atoi(argv[2]);
	int beginCol = atoi(argv[3]);
	int endLine = atoi(argv[4]);
	int endCol = atoi(argv[5]);
	std::vector<int> TargetLocation({beginLine, beginCol, endLine, endCol});
	std::vector<std::string> target = get_targetFunction(Tool, beginLine, beginCol); 
	std::string FunctionName = target[0], FunctionCall = target[1];


  MatchFinder Finder;
  
  // Set up the AST matcher for the main function declaration
  DeclarationMatcher MainFunctionMatcher = functionDecl(hasName("main")).bind("mainFunctionDecl");
  MainFunctionRewriter mainRewriter(Tool.getRewriter(), FunctionCall);
  Finder.addMatcher(MainFunctionMatcher, &mainRewriter);


  // Set up the AST matcher for function declarations and statements
  DeclarationMatcher FunctionMatcher = functionDecl(hasName(FunctionName)).bind("targetFunctionDecl");
  StatementMatcher StmtMatcher = stmt().bind("stmt");
  Finder.addMatcher(FunctionMatcher, &StmtMatcher);


  StmtDeleter Deleter(Tool.getRewriter(), TargetLocation);
  Finder.addMatcher(StmtMatcher, &Deleter);

  // Run the tool and collect the main function declaration
  return Tool.run(newFrontendActionFactory(&Finder).get());
}

