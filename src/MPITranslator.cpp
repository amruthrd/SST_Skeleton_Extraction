#include "rose.h"
#include "string.h"

using namespace SageInterface;
using namespace std;
/**
 *
 */
class MPITranslator: public SgSimpleProcessing {

public:
	void visit(SgNode * astNode);
	int insertHeaders();
	void insertFunctionSymbol(string funcName, string newFuncSymbol);
	void process_MPI_Init(SgFunctionCallExp* functioncall);
	void process_MPI_Comm_rank(SgFunctionCallExp* functioncall);
	void process_MPI_Finalize(SgFunctionCallExp* functioncall);
	void process_MPI_Broadcast(SgFunctionCallExp* functioncall);
	void process_MPI_Send(SgFunctionCallExp* functioncall);
	void process_MPI_Recv(SgFunctionCallExp* functioncall);
	SgFunctionCallExp * createNamedFunctioncall(std::string name,
			SgExprListExp * exprList, SgScopeStatement* scope);
public:
	int mpiCount;
	SgProject* project;
	SgGlobal *glb;
	Sg_File_Info *file;
	MPITranslator(SgProject* project) :
		project(project) {
		mpiCount = 0;
	}
	void setSgGlobal(SgProject* project){
			glb = getFirstGlobalScope(project);
	}



};



/**
 * This method is called for every node in the AST. This is mainly used during transformation phase to find out
 * MPI calls and call the corresponding process_MPI_XXX method for doing transformation.
 */
void MPITranslator::visit(SgNode* astNode) {
	if (astNode->variantT() == V_SgFunctionCallExp) {
		SgFunctionCallExp *functionCall = isSgFunctionCallExp(astNode);
		SgFunctionRefExp* function = isSgFunctionRefExp(
				functionCall->get_function());

		if (function != NULL) {
			SgName functionName =
					function->get_symbol()->get_declaration()->get_name();

			//for MPI_init
			if (functionName.getString() == "MPI_Init") {
				mpiCount++;
				process_MPI_Init(functionCall);
			}
			//for MPI_Comm_rank
			if (functionName.getString() == "MPI_Comm_rank") {
				mpiCount++;
				process_MPI_Comm_rank(functionCall);
			}
			//for MPI_finalize
			if (functionName.getString() == "MPI_Finalize") {
				mpiCount++;
				process_MPI_Finalize(functionCall);
			}
			//for MPI_Broadcast
			if (functionName.getString() == "MPI_Bcast") {
				mpiCount++;
				process_MPI_Broadcast(functionCall);
			}
			//for MPI_Send
			if (functionName.getString() == "MPI_Send") {
				mpiCount++;
				process_MPI_Send(functionCall);
			}
			//for MPI_Recv
			if (functionName.getString() == "MPI_Recv") {
				mpiCount++;
				process_MPI_Recv(functionCall);
			}

		}
	}

}

// TODO
/**
 * This method tranforms MPI_init(argc, argv) to init.
 */
void MPITranslator::process_MPI_Init(SgFunctionCallExp* functionCall) {

	SgFunctionRefExp* function = isSgFunctionRefExp(
			functionCall->get_function());
	if (function->variantT() == V_SgFunctionRefExp) {

		//fetch the arguments
		SgExprListExp * oldArgList = functionCall->get_args();
		cout<<"Old Arguments inside MPI_init call"<<oldArgList->unparseToString()<<endl;
		SgExpressionPtrList oldExprList= oldArgList->get_expressions();

		SgExpressionPtrList::iterator oldExprIt=oldExprList.begin();

		SgExprListExp * newArgList = new SgExprListExp(
				Sg_File_Info::generateDefaultFileInfoForTransformationNode());


		functionCall->set_args(newArgList);
		newArgList->set_parent(functionCall);
		oldExprIt++;
		newArgList->append_expression(*oldExprIt);
		oldExprIt--;
		newArgList->append_expression(*oldExprIt);
		delete (oldArgList);

		SgFunctionSymbol *func_symbol = glb->lookup_function_symbol(SgName("init"),
				function->get_type());

		ROSE_ASSERT(func_symbol != NULL);
		ROSE_ASSERT(func_symbol->get_type() != NULL);
		function->set_symbol(func_symbol);

		cout<<func_symbol->get_type();



		//replaceStatement(isSgStatement(functionCall->get_parent()), decl);

		cout << "new function symbol set in the Symbol table" << endl;

		//insertStatementBefore(isSgStatement(functionCall->get_parent()), decl);


		cout << "converting MPI_Init done" << endl;
	}

}

/**
 * This method tranforms MPI_Comm_rank to rank() with one more SST/macro mpi call inserted before the statement.
 */
void MPITranslator::process_MPI_Comm_rank(SgFunctionCallExp* functionCall) {

	SgFunctionRefExp* function = isSgFunctionRefExp(
			functionCall->get_function());
	if (function->variantT() == V_SgFunctionRefExp) {

		SgFunctionSymbol *func_symbol = glb->lookup_function_symbol("rank",
				function->get_type());
		ROSE_ASSERT(func_symbol != NULL);
		ROSE_ASSERT(func_symbol->get_type() != NULL);
		function->set_symbol(func_symbol);
		cout << "new function symbol set in the Symbol table" << endl;

		SgExprListExp * arg = new SgExprListExp(
				Sg_File_Info::generateDefaultFileInfoForTransformationNode());
		SgFunctionCallExp * rhs = createNamedFunctioncall(string("comm_world"),
				arg, isSgStatement(functionCall->get_parent())->get_scope());

		SgVariableDeclaration* decl = SageBuilder::buildVariableDeclaration(
				"world_", SageBuilder::buildVoidType(),
				SageBuilder::buildAssignInitializer(rhs));

		insertStatementBefore(isSgStatement(functionCall->get_parent()), decl);

		//fetch the arguments
		SgExprListExp * oldArgList = functionCall->get_args();
		SgExprListExp * newArgList = new SgExprListExp(
				Sg_File_Info::generateDefaultFileInfoForTransformationNode());
		functionCall->set_args(newArgList);
		newArgList->set_parent(functionCall);
		delete (oldArgList);

		cout << "converting MPI_Comm_rank done" << endl;
	}

}

/**
 * MPI_Finalize ---> finalize with no arguments
 */
void MPITranslator::process_MPI_Finalize(SgFunctionCallExp* functionCall) {

	SgFunctionRefExp* function = isSgFunctionRefExp(
			functionCall->get_function());
	if (function->variantT() == V_SgFunctionRefExp) {

		SgFunctionSymbol *func_symbol = glb->lookup_function_symbol("finalize",
				function->get_type());
		ROSE_ASSERT(func_symbol != NULL);
		ROSE_ASSERT(func_symbol->get_type() != NULL);
		function->set_symbol(func_symbol);
		cout << "new function symbol set in the Symbol table" << endl;

		//fetch the arguments
		SgExprListExp * oldArgList = functionCall->get_args();
		SgExprListExp * newArgList = new SgExprListExp(
				Sg_File_Info::generateDefaultFileInfoForTransformationNode());
		functionCall->set_args(newArgList);
		newArgList->set_parent(functionCall);
		delete (oldArgList);

		cout << "converting MPI_Finalize done" << endl;
	}
}

/**
 * MPI_Bcast ---> bcast with same arguments, arguments need to rearranged
 */
void MPITranslator::process_MPI_Broadcast(SgFunctionCallExp* functionCall) {

	SgFunctionRefExp* function = isSgFunctionRefExp(
			functionCall->get_function());
	if (function->variantT() == V_SgFunctionRefExp) {

		SgFunctionSymbol *func_symbol = glb->lookup_function_symbol("bcast",
				function->get_type());
		ROSE_ASSERT(func_symbol != NULL);
		ROSE_ASSERT(func_symbol->get_type() != NULL);
		function->set_symbol(func_symbol);
		cout << "new function symbol set in the Symbol table" << endl;

		cout << "converting MPI_Bcast done" << endl;
	}
}

/**
 * MPI_Send ---> send with same arguments, arguments need to rearranged
 */
void MPITranslator::process_MPI_Send(SgFunctionCallExp* functionCall) {

	SgFunctionRefExp* function = isSgFunctionRefExp(
			functionCall->get_function());
	if (function->variantT() == V_SgFunctionRefExp) {

		SgFunctionSymbol *func_symbol = glb->lookup_function_symbol("send",
				function->get_type());
		ROSE_ASSERT(func_symbol != NULL);
		ROSE_ASSERT(func_symbol->get_type() != NULL);
		function->set_symbol(func_symbol);
		cout << "new function symbol set in the Symbol table" << endl;

		cout << "converting MPI_Send done" << endl;
	}
}

/**
 * MPI_Recv ---> bcast with same arguments, arguments need to rearranged
 */
void MPITranslator::process_MPI_Recv(SgFunctionCallExp* functionCall) {

	SgFunctionRefExp* function = isSgFunctionRefExp(
			functionCall->get_function());
	if (function->variantT() == V_SgFunctionRefExp) {

		SgFunctionSymbol *func_symbol = glb->lookup_function_symbol("recv",
				function->get_type());
		ROSE_ASSERT(func_symbol != NULL);
		ROSE_ASSERT(func_symbol->get_type() != NULL);
		function->set_symbol(func_symbol);
		cout << "new function symbol set in the Symbol table" << endl;

		cout << "converting MPI_Recv done" << endl;
	}
}


/**
 * This is to create a fake SST/macro MPI call ---> world()
 */
SgFunctionCallExp * MPITranslator::createNamedFunctioncall(string name,
		SgExprListExp * exprList, SgScopeStatement* scope) {
	SgFunctionDeclaration * decl = new SgFunctionDeclaration(
			Sg_File_Info::generateDefaultFileInfoForCompilerGeneratedNode(),
			SgName(name), new SgFunctionType(new SgTypeInt()));
	decl->set_endOfConstruct(
			Sg_File_Info::generateDefaultFileInfoForCompilerGeneratedNode());
	decl->setForward();
	decl->set_scope(scope);

	SgFunctionRefExp * funcName = new SgFunctionRefExp(
			Sg_File_Info::generateDefaultFileInfoForCompilerGeneratedNode(),
			new SgFunctionSymbol(decl), new SgFunctionType(new SgTypeInt()));
	funcName->set_endOfConstruct(
			Sg_File_Info::generateDefaultFileInfoForCompilerGeneratedNode());

	SgFunctionCallExp * call = new SgFunctionCallExp(
			Sg_File_Info::generateDefaultFileInfoForTransformationNode(),
			funcName, exprList);
	call->set_endOfConstruct(
			Sg_File_Info::generateDefaultFileInfoForCompilerGeneratedNode());
	return call;
}

/**
 * This method is to insert a function symbol in the symbol table so as to replace
 * standard MPI name with that of SST/macro MPI name.
 */

//void MPITranslator::insertFunctionSymbol(string funcName, string newFuncSymbol) {
//	glb = getFirstGlobalScope(project);
//	SgFunctionDeclaration* func = findDeclarationStatement<
//			SgFunctionDeclaration> (project, funcName, glb, true);
//	ROSE_ASSERT (func != NULL);
//	cout << "Function Name is " << funcName << endl;
//	SgFunctionDeclaration* func_copy = isSgFunctionDeclaration(copyStatement(
//			func));
//	func_copy->set_name(newFuncSymbol);
//	appendStatement(func_copy, glb);
//
//	SgFunctionSymbol *func_symbol = glb->lookup_function_symbol(newFuncSymbol,
//			func_copy->get_type());
//	if (func_symbol == NULL) {
//		func_symbol = new SgFunctionSymbol(func_copy);
//		glb ->insert_symbol(newFuncSymbol, func_symbol);
//		cout << "symbol added for " << funcName << endl;
//	}
//
//}

/**
 * Inserting SST/macro headers. Some more headers need to be included
 */
int MPITranslator::insertHeaders() {
	Rose_STL_Container<SgNode*> globalScopeList = NodeQuery::querySubTree (project,V_SgGlobal);
	for (Rose_STL_Container<SgNode*>::iterator i = globalScopeList.begin(); i != globalScopeList.end(); i++)
	{
		SgGlobal* globalscope = isSgGlobal(*i);
		ROSE_ASSERT (globalscope != NULL);
		SageInterface::insertHeader("sstmac/sstmacconfig.h",PreprocessingInfo::after,false,globalscope);
		SageInterface::insertHeader("sstmac/process/mpistatus.h",PreprocessingInfo::after,false,globalscope);
		SageInterface::insertHeader("sstmac/process/mpitype.h",PreprocessingInfo::after,false,globalscope);
		SageInterface::insertHeader("sstmac/process/mpitag.h",PreprocessingInfo::after,false,globalscope);
		SageInterface::insertHeader("sstmac/util/errors.h",PreprocessingInfo::after,false,globalscope);
		SageInterface::insertHeader("sstmac/process/mpiapp.h",PreprocessingInfo::after,false,globalscope);
		SageInterface::insertHeader("sstmac/process/mpiapi.h",PreprocessingInfo::after,false,globalscope);
	}
	return 0; //assume always successful currently
}

int main(int argc, char * argv[]) {
	// Build ROSE AST
	SgProject* project = frontend(argc, argv);
	ROSE_ASSERT(project != NULL);

	generateDOT(*project);

	// Call traversal
	MPITranslator* mpiTranslator = new MPITranslator(project);

	// insert SST/macro headers
	mpiTranslator->insertHeaders();
	mpiTranslator->setSgGlobal(project);

	mpiTranslator->traverseInputFiles(project, preorder);

	// Print the number of MPI calls
	cout << "Number of MPI Calls Transformed " << mpiTranslator->mpiCount
			<< endl;

	cout << "\nStart:Code Transformed-----------------------------------------" << endl;
	cout << project->unparseToString();
	cout << "End:Code Transformed-------------------------------------------" << endl;

	//project->skipfinalCompileStep(true);

	//AstTests::runAllTests(project);

	// Generate Code and compile it with backend (vendor) compiler
	return backend(project);
}
