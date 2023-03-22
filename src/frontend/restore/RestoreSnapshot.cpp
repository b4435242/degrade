#include <iostream>
#include <sstream>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <fstream>
#include <vector>

#include "../AngelixCommon.h"
#include "../util.h"

std::vector<int> restoreLocation;




void parse_locations(const char** argv, int offset){
	int count = offset;
	
	for(int i=0; i<4; i++) 
		restoreLocation.push_back(atoi(argv[count++]));
	
}

template<typename T>
int getSize(T vd) {
    const clang::Type *type = vd->getType().getTypePtr();
    while (type->isPointerType()){
        type = type->getPointeeType().getTypePtr();
    }
    /*if (type->isRecordType()){
        const RecordType *rtype = dyn_cast<RecordType>(type);
        if (!rtype || !rtype->getDecl()) // exclude if size is not determined
            return 0;
    }*/

    return vd->getASTContext().getTypeInfo(type).Width;
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

		

		SourceManager &srcMgr = Rewrite.getSourceMgr();
		int beginLine = srcMgr.getExpansionLineNumber(S->getBeginLoc());
		int beginCol = srcMgr.getExpansionColumnNumber(S->getBeginLoc());
		int endLine = srcMgr.getExpansionLineNumber(S->getEndLoc());
		int endCol = srcMgr.getExpansionColumnNumber(S->getEndLoc());
	
		std::cout<<toString(S)<<std::endl;

		if (!isa<DeclStmt>(S) && S->getBeginLoc().isValid() && (endLine<restoreLocation[0] || (endLine==restoreLocation[0] && endCol<restoreLocation[1])) ) {
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
			std::map<std::string, RecordDecl::field_iterator> ptrs;
			std::map<std::string, std::string> exprs;
			std::map<std::string, int> sizes;
			getPtrInStruct(Context, vars, ptrs, exprs, sizes);
			
			std::ostringstream stringStream; 
      		//FIXME: why no members here?
			std::ostringstream exprStream;
			std::ostringstream nameStream;
			std::ostringstream typeStream;
      		std::ostringstream sizeStream;
			bool first = true;
			for (auto it = vars.begin(); it != vars.end(); ++it) {
        		
				VarDecl* var = *it;
				/*if (first) {
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
        		sizeStream << getSize<VarDecl*>(var)/8;
      		*/
				stringStream << "DG_RESTORE("
						 << beginLine << ", "
						 << beginCol << ", "
						 << endLine << ", "
						 << endCol << ", "
						 << "\"" << var->getName().str() << "\"" << ", " 
						 << "\"" << var->getType().getAsString() << "\"" << ", "
						 << "&" << var->getName().str() << ", "
						 << getSize<VarDecl*>(var)/8
						 << ");\n";
						 
			}
	
			int n_ptr_in_struct = 0;
			for (auto it = ptrs.begin(); it != ptrs.end(); ++it) {
        		std::string name = it->first;
        		std::string expr = exprs[name];
				auto var = it->second;

        		//int size = getSize<RecordDecl::field_iterator>(var)/8;
        		int size = sizes[name];
				if (size==0) 
            		continue;
        		stringStream << "DG_RESTORE("
						 << beginLine << ", "
						 << beginCol << ", "
						 << endLine << ", "
						 << endCol << ", "
						 << "\"" << name << "\"" << ", " 
						 << "\"" << var->getType().getAsString() << "\"" << ", "
						 << expr << ", "
						 << size
						 << ");\n";
				
    		}


			stringStream << toString(S);
      		int size = vars.size() + n_ptr_in_struct;
			/*std::ostringstream stringStream;
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
						 << ");\n";
			stringStream << toString(S);*/
			Rewrite.ReplaceText(S->getSourceRange(), stringStream.str());
			return false;
		}
		//return RecursiveASTVisitor<RestoreStmtVisitor>::TraverseStmt(S);
		return true;
	}

private:
	Rewriter &Rewrite;
	ASTContext* Context;
};

class FunctionVisitor : public clang::RecursiveASTVisitor<FunctionVisitor> {
public:
	FunctionVisitor(Rewriter &R, ASTContext* Context, std::string restoreFuncName): Rewrite(R), Context(Context), restoreFuncName(restoreFuncName) {}
	
	/*virtual void run(const MatchFinder::MatchResult &Result) {
		RestoreStmtVisitor restoreStmtVisitor(Rewrite, Result.Context);	
    	if (const FunctionDecl *funcDecl = Result.Nodes.getNodeAs<FunctionDecl>("restoreFunctionDecl")) {
      		// Create a RestoreStmtVisitor to traverse the statements in the function.
    		restoreStmtVisitor.TraverseStmt(funcDecl->getBody());
    	}
	}*/

	bool VisitFunctionDecl(FunctionDecl* fd){
		if (fd->getNameAsString()==restoreFuncName){
			RestoreStmtVisitor restoreStmtVisitor(Rewrite, Context);
			restoreStmtVisitor.TraverseStmt(fd->getBody());
			return false;
		}
		return true;
	}


private:
	Rewriter &Rewrite;
	std::string restoreFuncName;
	//RestoreStmtVisitor restoreStmtVisitor;
	ASTContext* Context;
};




class MyASTConsumer : public ASTConsumer {
public:
	MyASTConsumer(Rewriter &R) : HandlerForMainFunction(R),
								 //HandlerForRestoreFunction(R),
								 Visitor(NULL, R),
								 Rewrite(R)
	{							 		
		//Finder.addMatcher(MainFunctionMatcher, &HandlerForMainFunction);
		
	}

	void HandleTranslationUnit(ASTContext &Context) override {
		
    	Visitor.TraverseDecl(Context.getTranslationUnitDecl());
		std::string restoreFunctionName = Visitor.getFunctionName();
		std::string restoreFunctionCall = Visitor.getFunctionCall();
		
		HandlerForMainFunction.setFunction(restoreFunctionCall);
		
		//DeclarationMatcher restoreFunctionMatcher = functionDecl(hasName(restoreFunctionName)).bind("restoreFunctionDecl");
		//Finder.addMatcher(restoreFunctionMatcher, &HandlerForRestoreFunction);
		//Finder.matchAST(Context);

		FunctionVisitor visitor(Rewrite, &Context, restoreFunctionName);
		visitor.TraverseDecl(Context.getTranslationUnitDecl());

		
	}

private:
	MainFunctionRewriter HandlerForMainFunction;	
	//RestoreFunctionRewriter HandlerForRestoreFunction; 

	MatchFinder Finder;
	
	FunctionFinderVisitor Visitor;
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
	parse_locations(argv, 2);
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
