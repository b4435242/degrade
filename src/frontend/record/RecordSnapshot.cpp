#include <iostream>
#include <sstream>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <fstream>
#include <vector>

#include "../AngelixCommon.h"
#include "../util.h" 

std::vector<int> position;


void parse_position(int argc, const char** argv, int offset){
	
	position.resize(4);
	for(int i=0; i<4; i++)
		position[i] = atoi(argv[offset+i]);	
	
}

bool is_position(int beginLine, int beginCol, int endLine, int endCol) {
  return position[0]==beginLine && position[1]==beginCol && position[2]==endLine && position[3]==endCol;
}

template<typename T>
int getSize(T vd) {
    const clang::Type *type = vd->getType().getTypePtr();
    while (type->isPointerType()){
        type = type->getPointeeType().getTypePtr();
    }
    
	/*if (type->isRecordType()) {
		int size = 0;
		const RecordType* rt = dyn_cast<RecordType>(vd->getType().getTypePtr());
		const RecordDecl* rd = rt->getDecl();
		for(auto it=rd->field_begin(); it!=rd->field_end(); it++){
			const clang::Type* t = it->getType().getTypePtr();
			if (t->isRecordType()){
				size += getSize<RecordDecl::field_iterator>(it);
			}
			size += vd->getASTContext().getTypeInfo(type).Width;
		}
		return size;
	}*/

	//return vd->getASTContext().getTypeInfo(type).Width;
	return vd->getASTContext().getTypeSize(type);
}


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

      //std::cout << beginLine << " " << beginColumn << " " << endLine << " " << endColumn << "\n"   << toString(stmt) << "\n";
      
      if (!is_position(beginLine, beginColumn, endLine, endColumn))
      	return;
      

      /*std::ostringstream stmtId;
      stmtId << beginLine << "-" << beginColumn << "-" << endLine << "-" << endColumn;
      std::string extractedDir(getenv("ANGELIX_EXTRACTED"));
      std::ofstream fs(extractedDir + "/" + stmtId.str() + ".smt2");
      fs << "(assert (not (= 1 0)))\n";*/

      const DynTypedNode node = DynTypedNode::create(*stmt);
	  VarTypes collectedTypes = ALL;
      std::pair< std::unordered_set<VarDecl*>, std::unordered_set<MemberExpr*> > varsFromScope = collectVarsFromScope(node, Result.Context, beginLine, Rewrite, collectedTypes);
      std::unordered_set<VarDecl*> vars;
      vars.insert(varsFromScope.first.begin(), varsFromScope.first.end());
      
	  std::map<std::string, RecordDecl::field_iterator> ptrs;
	  std::map<std::string, std::string> exprs;
	  std::map<std::string, int> sizes;
	  getPtrInStruct(Result.Context, vars, ptrs, exprs, sizes);

	  //FIXME: why no members here?
      std::ostringstream exprStream;
      std::ostringstream nameStream;
      std::ostringstream typeStream;
	  std::ostringstream sizeStream;
	  std::ostringstream stringStream;
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
		sizeStream << getSize<VarDecl*>(var)/8;
		/*VarDecl* var = *it;
		stringStream << "DG_RESTORE("
					<< beginLine << ", "
					<< beginColumn << ", "
					<< endLine << ", "
					<< endColumn << ", "
					<< "\"" << var->getName().str() << "\"" << ", "
					<< "\"" << var->getType().getAsString() << "\"" << ","
					<< "&" << var->getName().str() << ","
					<< getSize<VarDecl*>(var)/8 
					<< ");\n";*/
	    //sizeStream << "sizeof(" <<  getDeference<VarDecl*>(var, var->getName().str()) << ")";
    }
    int n_ptr_in_struct = 0;
	for (auto it = ptrs.begin(); it != ptrs.end(); ++it) {
        std::string name = it->first;
		std::cout <<"name " << name << std::endl;
		std::string expr = exprs[name];
        auto var = it->second;
        //int size = getSize<RecordDecl::field_iterator>(var)/8;
        int size = sizes[name];
		if (size==0) 
            continue;
        if (first) {
          first = false;
        } else {
          exprStream << ", ";
          nameStream << ", ";
		  typeStream << ", ";
		  sizeStream << ", ";
        }
		exprStream << expr;
        nameStream << "\"" << name << "\"";
      	typeStream << "\"" << var->getType().getAsString() << "\"" ;
		sizeStream << size;
		n_ptr_in_struct++;
    }



      int size = vars.size() + n_ptr_in_struct;
      std::string indent(beginColumn-1, ' ');

      //std::ostringstream stringStream;
      stringStream << indent
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
                   << ");\n";
	  
	  stringStream << toString(stmt);
      std::string replacement = stringStream.str();

      Rewrite.ReplaceText(expandedLoc, replacement);
    }
  }

private:
  Rewriter &Rewrite;

};


class MyASTConsumer : public ASTConsumer {
public:
  MyASTConsumer(Rewriter &R) : HandlerForStatements(R) {	
	Matcher.addMatcher(InterestingStatement, &HandlerForStatements);
  	//Matcher.addMatcher(stmt().bind("repairable"), &HandlerForStatements);
  }

  void HandleTranslationUnit(ASTContext &Context) override {
    Matcher.matchAST(Context);
    
    const clang::SourceManager &SM = Context.getSourceManager();
  }

private:
	StatementHandler HandlerForStatements;
	MatchFinder Matcher;
};


class MyFrontendAction : public ASTFrontendAction {
public:
  MyFrontendAction() {}

  void EndSourceFileAction() override {
    FileID ID = TheRewriter.getSourceMgr().getMainFileID();
	std::cout << TheRewriter.getSourceMgr().getFileEntryForID(ID)->getName().str()<<std::endl;
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
	return Tool.run(newFrontendActionFactory<MyFrontendAction>().get());
}
