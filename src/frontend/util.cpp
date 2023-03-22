#include "util.h"

unsigned getDeclExpandedLine(const clang::Decl* decl, SourceManager &srcMgr) {
  SourceLocation startLoc = decl->getBeginLoc();
  if(startLoc.isMacroID()) {
    // Get the start/end expansion locations
    CharSourceRange expansionRange = srcMgr.getExpansionRange(startLoc);
    // We're just interested in the start location
    startLoc = expansionRange.getBegin();
  }

  return srcMgr.getExpansionLineNumber(startLoc);
}


bool insideMacro(const clang::Stmt* expr, SourceManager &srcMgr, const LangOptions &langOpts) {
  SourceLocation startLoc = expr->getBeginLoc();
  SourceLocation endLoc = expr->getEndLoc();

  if(startLoc.isMacroID() && !Lexer::isAtStartOfMacroExpansion(startLoc, srcMgr, langOpts))
    return true;

  if(endLoc.isMacroID() && !Lexer::isAtEndOfMacroExpansion(endLoc, srcMgr, langOpts))
    return true;

  return false;
}


SourceRange getExpandedLoc(const clang::Stmt* expr, SourceManager &srcMgr) {
  SourceLocation startLoc = expr->getBeginLoc();
  SourceLocation endLoc = expr->getEndLoc();

  if(startLoc.isMacroID()) {
    // Get the start/end expansion locations
    CharSourceRange expansionRange = srcMgr.getExpansionRange(startLoc);
    // We're just interested in the start location
    startLoc = expansionRange.getBegin();
  }
  if(endLoc.isMacroID()) {
    // Get the start/end expansion locations
    CharSourceRange expansionRange = srcMgr.getExpansionRange(startLoc);
    // We're just interested in the end location
    endLoc = expansionRange.getEnd();
  }

  SourceRange expandedLoc(startLoc, endLoc);

  return expandedLoc;
}


std::string toString(const clang::Stmt* stmt) {
  /* Special case for break and continue statement
     Reason: There were semicolon ; and newline found
     after break/continue statement was converted to string
  */
  if (dyn_cast<clang::BreakStmt>(stmt))
    return "break";

  if (dyn_cast<clang::ContinueStmt>(stmt))
    return "continue";

  clang::LangOptions LangOpts;
  clang::PrintingPolicy Policy(LangOpts);
  std::string str;
  llvm::raw_string_ostream rso(str);

  stmt->printPretty(rso, nullptr, Policy);

  std::string stmtStr = rso.str();
  return stmtStr;
}


bool overwriteMainChangedFile(Rewriter &TheRewriter) {
  bool AllWritten = true;
  FileID id = TheRewriter.getSourceMgr().getMainFileID();
  const FileEntry *Entry = TheRewriter.getSourceMgr().getFileEntryForID(id);
  std::error_code err_code;
  llvm::raw_fd_ostream out(Entry->getName(), err_code, llvm::sys::fs::OF_None);
  TheRewriter.getRewriteBufferFor(id)->write(out);
  out.close();
  return !AllWritten;
}



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

    if (getenv("ANGELIX_GLOBAL_VARIABLES")) {
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
    }
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
          //if (dstmt->isSingleDecl()) {
            //Decl* d = dstmt->getSingleDecl();
          for(auto it = dstmt->decl_begin(); it!=dstmt->decl_end(); it++){
		  	Decl *d = *it;
			if (isa<VarDecl>(d)) {
              VarDecl* vd = cast<VarDecl>(d);
              if (line > beginLine && vd->hasInit() && suitableVarDecl(vd, collectedTypes)) {
                var_set.insert(vd);
              } else {
                //if (getenv("ANGELIX_INIT_UNINIT_VARS")) {
                  if (line > beginLine && suitableVarDecl(vd, collectedTypes)) {
                    /*std::ostringstream stringStream;
                    stringStream << vd->getType().getAsString() << " "
                                 << vd->getNameAsString()
                                 << " = 0";
                    std::string replacement = stringStream.str();
                    Rewrite.ReplaceText(vd->getSourceRange(), replacement);*/
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

/*template<typename T>
int getSize(T vd) {
    const clang::Type *type = vd->getType().getTypePtr();
    while (type->isPointerType()){
        type = type->getPointeeType().getTypePtr();
    }
    if (type->isRecordType()){
        const RecordType *rtype = dyn_cast<RecordType>(type);
        if (!rtype || !rtype->getDecl()) // exclude if size is not determined
            return 0;
    }

    return vd->getASTContext().getTypeInfo(type).Width;
}*/


void getPtrInStruct(ASTContext* Context, std::unordered_set<VarDecl*> &vars, std::map<std::string, RecordDecl::field_iterator> &ptrs, std::map<std::string, std::string> &exprs, std::map<std::string, int> &sizes){
    StructVisitor visitor(Context);
    for(auto &var: vars){
        visitor.TraverseDecl(var);
    }
    ptrs = visitor.vars;
	exprs = visitor.exprs;
	sizes = visitor.sizes;
}


bool StructVisitor::VisitVarDecl(VarDecl* vd) {
    const clang::Type* t = vd->getType().getTypePtr();
	if (t->isStructureType() || (t->isPointerType()&&t->getPointeeType().getTypePtr()->isStructureType())){
        RecordDecl *rd = get_recordDecl<VarDecl*>(vd);
        record_push<VarDecl*>(vd);
        TraverseDecl(rd);
    	record_pop();
    }
	return false;
}

bool StructVisitor::VisitRecordDecl(RecordDecl* rd){
	//const VarDecl* var = dyn_cast<VarDecl*>(rd);
	//const RecordType *rtype = dyn_cast<RecordType>(var->getType().getTypePtr());
	const RecordType* rtype = dyn_cast<RecordType>(rd->getTypeForDecl());
	//const ASTRecordLayout& layout = rd->getASTContext().getASTRecordLayout(rd);

	if (!rd->getDefinition() || !rd->isCompleteDefinition())
		return false;
	//int size = 0;
	unsigned size = rd->getASTContext().getTypeSize(rtype);
	for(auto it=rd->field_begin(); it!=rd->field_end(); it++){

        const clang::Type* t = it->getType().getTypePtr();
      
		if (t->isPointerType()){ // only dump pointer type inside a struct
			const clang::Type* pointee = t->getPointeeType().getTypePtr();
            /*if (t->isRecordType()){
                const RecordType *rtype = dyn_cast<RecordType>(t);
                if (!rtype || !rtype->getDecl()){ // exclude if size is not determine
                	std::cout << it->getName().str() <<std::endl;
					continue;
				}
            }*/
			
            std::string name = it->getName().str();
            auto v = get_nullsafe_expr_and_name(name);
            std::string nullsafe_expr = v[0], full_name = v[1];
			vars[full_name] = it;
			exprs[full_name] = nullsafe_expr;
			if (!pointee->isRecordType()){
				std::cout << it->getType().getAsString() << std::endl;
				sizes[full_name] = it->getASTContext().getTypeInfo(pointee).Width/8;
        	}
		}
		
        if (t->isStructureType() || (t->isPointerType()&&t->getPointeeType().getTypePtr()->isStructureType())){
            RecordDecl *child_rd = get_recordDecl<RecordDecl::field_iterator>(it);
            if (child_rd==NULL || rd->getName().str() == child_rd->getName().str()) // exclude link-list case
                continue;
            record_push<RecordDecl::field_iterator>(it);
            TraverseDecl(child_rd);
            record_pop();
        }
		
		/*if (t->isStructureType()){
			std::string field_name = get_nullsafe_expr_and_name(it->getNameAsString())[1];
			size += sizes[field_name];
		} else {
			size += it->getASTContext().getTypeInfo(t).Width/8;
		}*/
    }
	std::string struct_name = get_struct_name();
	sizes[struct_name] = size;


    return false;
}


template<typename T>
void StructVisitor::record_push(T decl){
    std::string name = decl->getName().str();
    bool isptr = decl->getType().getTypePtr()->isPointerType();
    name_v.push_back(name);
    sign_v.push_back(isptr?"->":".");
}

void StructVisitor::record_pop(){
    name_v.pop_back();
    sign_v.pop_back();
}

std::string StructVisitor::get_struct_name(){
	std::string curr;
	int n = name_v.size();
	for(int i=0; i<n; i++){
		curr += name_v[i];
		if (i!=n-1)
			curr += sign_v[i];
	}
	return curr;
}

std::vector<std::string> StructVisitor::get_nullsafe_expr_and_name(std::string name){
	std::string curr, ans;
	name_v.push_back(name); // check if last ptr is null
	sign_v.push_back("->");

	int n = name_v.size();
	for(int i=0; i<n; i++){
		curr += name_v[i];
		if (sign_v[i]=="->"){ // check null for ptr
			if (!ans.empty())
				ans += "&&";
			ans += (curr+"!=NULL");
		}
		if (i!=n-1)
			curr += sign_v[i];
	}

	ans += ("?&(" + curr + "):NULL");

	name_v.pop_back();
	sign_v.pop_back();

	return {ans, curr};
}

template<typename T>
RecordDecl* StructVisitor::get_recordDecl(T decl){
	const clang::Type* t = decl->getType().getTypePtr();
	if (t->isRecordType()){
		return t->getAsRecordDecl();
	} else {
		return t->getPointeeType()->getAsRecordDecl();
	}
}


void CollectVariables::Collect(Expr *E) {
	if (E)
		Visit(E);
}

void CollectVariables::Visit(Stmt* S) {
	StmtVisitor<CollectVariables>::Visit(S);
}

void CollectVariables::VisitBinaryOperator(BinaryOperator *Node) {
	Collect(Node->getLHS());
	Collect(Node->getRHS());
}

void CollectVariables::VisitUnaryOperator(UnaryOperator *Node) {
	Collect(Node->getSubExpr());
}

void CollectVariables::VisitImplicitCastExpr(ImplicitCastExpr *Node) {
	Collect(Node->getSubExpr());
}

void CollectVariables::VisitParenExpr(ParenExpr *Node) {
	Collect(Node->getSubExpr());
}

void CollectVariables::VisitIntegerLiteral(IntegerLiteral *Node) {
}

void CollectVariables::VisitCharacterLiteral(CharacterLiteral *Node) {
}

void CollectVariables::VisitMemberExpr(MemberExpr *Node) {
	if (MSet) {
		MSet->insert(Node); // TODO: check memeber type?
	}
}

void CollectVariables::VisitDeclRefExpr(DeclRefExpr *Node) {
	if (VSet && isa<VarDecl>(Node->getDecl())) {
		VarDecl* vd;
		if ((vd = cast<VarDecl>(Node->getDecl())) != NULL) {
			if (suitableVarDecl(vd, Types)) {
  				VSet->insert(vd);
			}
		}
	}
}

void CollectVariables::VisitArraySubscriptExpr(ArraySubscriptExpr *Node) {
	if (ASet) {
		ASet->insert(Node);
	}	
}

