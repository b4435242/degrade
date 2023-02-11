#include <iostream>
#include <sstream>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <fstream>
#include <vector>

#include "../AngelixCommon.h"
#include "../util.h"

std::string functionCall;

class MainFunctionRewriter : public RecursiveASTVisitor<MainFunctionRewriter> {
public:
	MainFunctionRewriter(Rewriter &R) : Rewrite(R) {}

	/*virtual void run(const MatchFinder::MatchResult &Result) {
		// Get the main function declaration from the match result
		if (const FunctionDecl *FD = Result.Nodes.getNodeAs<FunctionDecl>("mainFunctionDecl")) {
			// Check if the function has a body (definition)
			if (FD->hasBody()) {
				// Replace the entire function body with a single statement
				std::ostringstream harnessStream;
				harnessStream << "{ " << functionCall << "; }\n";
				Rewrite.ReplaceText(FD->getBody()->getSourceRange(), harnessStream.str());
			}
		}
	}*/
	bool VisitFunctionDecl(FunctionDecl *funcDecl) {
		if (!funcDecl->hasBody())
			return true;
		if (!(funcDecl->getNameAsString()=="main"))	
			return true;
		
		std::ostringstream harnessStream;
		harnessStream << "{ " << functionCall << "; }\n";
		Rewrite.ReplaceText(funcDecl->getBody()->getSourceRange(), harnessStream.str());

		return false;
	}

private:
	Rewriter &Rewrite;
};

class MyASTConsumer : public ASTConsumer {
public:
	MyASTConsumer(Rewriter &R) : HandlerForMainFunction(R),
								 Rewrite(R)
	{							 		
		//Finder.addMatcher(MainFunctionMatcher, &HandlerForMainFunction);
	}

	void HandleTranslationUnit(ASTContext &Context) override {
		//Finder.matchAST(Context);
		HandlerForMainFunction.TraverseDecl(Context.getTranslationUnitDecl());
	}

private:
	MainFunctionRewriter HandlerForMainFunction;	
	MatchFinder Finder;
	Rewriter &Rewrite;
};


class MyFrontendAction : public ASTFrontendAction {
public:
  MyFrontendAction() {}

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
	functionCall = argv[2];
	argc = 2;
	// clang 16
	//auto ExpectedParser = CommonOptionsParser::create(argc, argv, MyToolCategory);
	//CommonOptionsParser &OptionsParser = ExpectedParser.get();

	//clang 11
	CommonOptionsParser OptionsParser(argc, argv, MyToolCategory);

	// We hand the CompilationDatabase we created and the sources to run over into the tool constructor.
	ClangTool Tool(OptionsParser.getCompilations(), OptionsParser.getSourcePathList());

	return Tool.run(newFrontendActionFactory<MyFrontendAction>().get());
}
