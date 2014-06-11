#include "CompleteParser.h"

void InitializeStatementNode(statementNode* node)
{
	node->assign_stmt	= 0;
	node->goto_stmt		= 0;
	node->if_stmt		= 0;
	node->next			= 0;
	node->print_stmt	= 0;
	node->stmt_type		= 0;
	node->func_stmt		= 0;
}

void CompleteParser::ShutdownProgram(statementNode* node)
{
	while(node)
	{
		if(node->assign_stmt)
			delete node->assign_stmt;
		if(node->goto_stmt)
			delete node->goto_stmt;
		if(node->if_stmt)
			delete node->if_stmt;
		if(node->print_stmt)
			delete node->print_stmt;
		if(node->func_stmt)
			delete node->func_stmt;
		statementNode* thisNode = node;
		node = node->next;
		delete thisNode;
	}
}

__declspec(dllexport) statementNode* CompleteParser::Compile()
{
	// Initialize Types.
	int i = -1;
	Node* node = FindNodeByName("type_decl_section", m_nodes, i);

	if(node)
		EvaluateNodes(*node);

	// Initialize Variables.
	node = FindNodeByName("var_decl_section", m_nodes, i);

	if(node)
		EvaluateNodes(*node);

	vector<statementNode*> nodeList;

	// The parse tree consists of arrays containing arrays. This will compress them into a single linked list.
	CompressNodes(m_nodes, nodeList);

	// Link the list.
	for(int i = 0; i < nodeList.size() - 1; i++)
	{
		nodeList[i]->next = nodeList[i + 1];
	}

	return nodeList.size() ? nodeList[0] : 0;
}


varAccess* CompleteParser::CompileExpression(Node& node, vector<statementNode*>& stmtList)
{
	if(node.nodes.empty())
	{
		return m_variables->GetOrCreateVarAccess(node.value);
	}

	// Another hard-coded solution...
	if(node.type == "array")
	{
		varAccess* temp = CompileExpression(node.nodes[2], stmtList);
		list<string> idList;
		BuildIDList(idList, node.nodes[0]);

		for(list<string>::iterator it = idList.begin(); it != idList.end(); it++)
		{
			m_variables->AddVariable(*it, TYPE_UNKNOWN);	
		}
		varAccess* returnVar = new varAccess;
		if(temp->index)
		{
			statementNode* stmt = new statementNode;
			InitializeStatementNode(stmt);
			stmt->stmt_type = ASSIGNSTMT;
			stmt->assign_stmt = new assignmentStatement;
			stmt->assign_stmt->op = 0;
			stmt->assign_stmt->op1 = temp;
			stmt->assign_stmt->op2 = 0;
			Variable* tempVar = m_variables->AddTempVariable();
			string name(tempVar->name);
			stmt->assign_stmt->lhs = m_variables->GetVarAccess(name);
			stmtList.push_back(stmt);

			returnVar->index = tempVar;
		}
		else
			returnVar->index = temp->var;
		returnVar->var = m_variables->GetVariable(*idList.begin());

		return returnVar;
	}

	vector<varAccess*> varList;

	for(int i = 0; i < node.nodes.size(); i++)
	{
		varAccess* a = CompileExpression(node.nodes[i], stmtList);

		if(a)
			varList.push_back(a);
	}

	if(varList.size() == 1)
		return varList[0];

	if(varList.size() == 2)
	{
		statementNode* stmt = new statementNode;
		InitializeStatementNode(stmt);
		stmt->stmt_type = ASSIGNSTMT;
		stmt->assign_stmt = new assignmentStatement;
		Node* opNode = FindOp(node.nodes[1]);
		if(opNode)
			stmt->assign_stmt->op = OperationToTokenType(opNode->type);
		else
			stmt->assign_stmt->op = 0;
		stmt->assign_stmt->op1 = varList[0];
		stmt->assign_stmt->op2 = varList[1];
		Variable* temp = m_variables->AddTempVariable();
		string name(temp->name);
		stmt->assign_stmt->lhs = m_variables->GetVarAccess(name);
		stmtList.push_back(stmt);

		return stmt->assign_stmt->lhs;
	}

	return 0;
}

void CompleteParser::CompileFunctionStmt(Node& node, statementNode* sNode, vector<statementNode*>& stmtList)
{
	sNode->stmt_type = FUNCSTMT;
	sNode->func_stmt = new functionStatement;
	// TODO Make this work for different types of functions.
	sNode->func_stmt->goto_stmt = 0;
	sNode->func_stmt->argument = 0;
	int index;
	Node* result = FindNodeByName("ID", node, index);

	if(result)
		sNode->func_stmt->funcName = result->value;


	// Assemble arguments.
	// TODO: more than one type of argument.
	list<string> ids;

	if(BuildIDList(ids, node.nodes[2]))
	{
		for(list<string>::iterator it = ids.begin(); it != ids.end(); it++)
		{
			Variable* var = m_variables->GetVariable(m_variables->GetVarIDNumber(*it));
			// Add the variable if it does not exist.
			if(!var)
			{
				m_variables->AddVariable(*it, TYPE_UNKNOWN);
				var = m_variables->GetVariable(m_variables->GetVarIDNumber(*it));
			}
			sNode->func_stmt->argument = m_variables->GetVarAccess(var->name);
		}
	}

	stmtList.push_back(sNode);
}

void CompleteParser::CompileAssignStmt(Node& node, statementNode* sNode, vector<statementNode*>& stmtList)
{
	sNode->stmt_type = ASSIGNSTMT;
	sNode->assign_stmt = new assignmentStatement;
	sNode->assign_stmt->op2 = 0;

	int i = -1;
	Node* assignmentNode = FindAssignmentOp(node, i);
	
	list<string> ids;
	
	// Build the left hand side.
	if(!assignmentNode || (i - 1 < 0 || !BuildIDList(ids, assignmentNode->nodes[i - 1]) || i + 1 >= assignmentNode->nodes.size()))
		return;

	// Make sure variable is declared.
	for(list<string>::iterator it = ids.begin(); it != ids.end(); it++)
	{
		m_variables->AddVariable(*it, TYPE_UNKNOWN);	
	}

	//sNode->assign_stmt->lhs = m_variables->GetVarAccess(node.nodes[0].value);

	// Generate a list of expressions using temporary variables.
	vector<statementNode*> newstmts;
	varAccess* tempVar = CompileExpression(node.nodes[i+1], newstmts);

	if(node.nodes[i-1].type == "array")
	{
		sNode->assign_stmt->lhs = CompileExpression(node.nodes[i-1], newstmts);
	}
	else
		sNode->assign_stmt->lhs = m_variables->GetOrCreateVarAccess(node.nodes[i-1].value);
	sNode->assign_stmt->op1 = tempVar;
	sNode->assign_stmt->op = 0;
	sNode->assign_stmt->op2 = 0;
	newstmts.push_back(sNode);

	// Move the new statements onto the back of the main statement list.
	// The move command is C++11 and may not compile on other systems.
	for(vector<statementNode*>::iterator it = newstmts.begin(); it != newstmts.end(); it++)
	{
		stmtList.push_back(*it);
	}

	newstmts.clear();
}

void CompleteParser::CompileWhileStmt(Node& node, statementNode* sNode, vector<statementNode*>& stmtList)
{
	statementNode* targetNode = stmtList.back();

	CompileIfStmt(node, sNode, stmtList);

	statementNode* gotoNode = new statementNode;
	InitializeStatementNode(gotoNode);
	gotoNode->stmt_type = GOTOSTMT;
	gotoNode->next = stmtList.back();
	gotoNode->goto_stmt = new gotoStatement;
	gotoNode->goto_stmt->target = targetNode;
	// Insert goto node second to last place so the false branch will move past the goto.
	vector<statementNode*>::iterator it = stmtList.end();
	stmtList.insert(--it, gotoNode);
}

void CompleteParser::CompileRepeatStmt(Node& node, statementNode* sNode, vector<statementNode*>& stmtList)
{
	InitializeStatementNode(sNode);
	sNode->stmt_type = NOOPSTMT;
	stmtList.push_back(sNode);

	// Find and compile the body nodes.
	int i = -1;
	Node* bodyNode = FindBodyNode(node, i);

	int lastI = stmtList.size();

	// Compile body nodes. 
	CompressNodes(bodyNode->nodes[i], stmtList);

	statementNode* ifNode = new statementNode;
	InitializeStatementNode(ifNode);
	ifNode->stmt_type = IFSTMT;
	ifNode->if_stmt = new ifStatement;

	// Compile an if statement by itself.
	CompileIfStmt(node, ifNode, stmtList, false);

	// Create a closing no-op node.
	statementNode* noop = new statementNode;
	InitializeStatementNode(noop);

	noop->stmt_type = NOOPSTMT;
	stmtList.push_back(noop);

	// if the condition completes the entire repeat node is finished.
	ifNode->if_stmt->true_branch = noop;

	// False will loop back to the beginning.
	ifNode->if_stmt->false_branch = sNode;
}

void CompleteParser::CompileIfStmt(Node& node, statementNode* sNode, vector<statementNode*>& stmtList,
						   bool includeBody)
{
	int conditionI = -1;
	Node* conditionNode = FindConditionNode(node, conditionI);

	if(!conditionNode)
		return;

	// Get the comparison operater which divides the statement.
	int i = -1;

	Node* assignmentNode = FindComparisonOp(*conditionNode, i);


	if(i - 1 < 0 || i + 1 >= assignmentNode->nodes.size())
		return;

	vector<statementNode*> newstmts;

	// Create a list of each expression on each side of the relop.
	statementNode* leftComp = new statementNode;
	InitializeStatementNode(leftComp);
	leftComp->stmt_type = ASSIGNSTMT;
	leftComp->assign_stmt = new assignmentStatement;
	string name1(m_variables->AddTempVariable()->name);
	leftComp->assign_stmt->lhs = m_variables->GetVarAccess(name1);
	leftComp->assign_stmt->op = 0;
	leftComp->assign_stmt->op1 = CompileExpression(conditionNode->nodes[i - 1], newstmts);
	leftComp->assign_stmt->op2 = 0;
	newstmts.push_back(leftComp);

	statementNode* rightComp = new statementNode;
	InitializeStatementNode(rightComp);
	rightComp->stmt_type = ASSIGNSTMT;
	rightComp->assign_stmt = new assignmentStatement;
	string name2(m_variables->AddTempVariable()->name);
	rightComp->assign_stmt->lhs = m_variables->GetVarAccess(name2);
	rightComp->assign_stmt->op = 0;
	rightComp->assign_stmt->op1 = CompileExpression(conditionNode->nodes[i + 1], newstmts);
	rightComp->assign_stmt->op2 = 0;
	newstmts.push_back(rightComp);

	// Move the expression statements to the main statement list.
	for(vector<statementNode*>::iterator it = newstmts.begin(); it != newstmts.end(); it++)
	{
		stmtList.push_back(*it);
	}
	newstmts.clear();

	sNode->if_stmt->op1 = leftComp->assign_stmt->lhs;
	sNode->if_stmt->op2 = rightComp->assign_stmt->lhs;

	Node* opNode = FindOp(conditionNode->nodes[i]);
	if(opNode)
		sNode->if_stmt->relop = OperationToTokenType(opNode->type);
	else
		sNode->if_stmt->relop = 0;

	stmtList.push_back(sNode);

	if(includeBody)
	{
		i = -1;
		Node* bodyNode = FindBodyNode(node, i);

		int lastI = stmtList.size();

		// Compile body nodes.
		CompressNodes(bodyNode->nodes[i], stmtList);

		// Link to true node (the if-stmt body node)
		sNode->if_stmt->true_branch = stmtList[lastI];

		// Link to false node (the last node created will be a no-op node)
		sNode->if_stmt->false_branch = stmtList.back();
	}
	else
	{
		sNode->if_stmt->true_branch = 0;
		sNode->if_stmt->false_branch = 0;
	}
}

int CompleteParser::OperationToTokenType(string& op)
{
	if(op == TOKENS[PLUS])
	{
		return PLUS;
	}
	else if(op == TOKENS[MINUS])
	{
		return MINUS;
	}
	else if(op == TOKENS[MULT])
	{
		return MULT;
	}
	else if(op == TOKENS[DIV])
	{
		return DIV;
	}
	else if(op == TOKENS[GREATER])
	{
		return GREATER;
	}
	else if(op == TOKENS[GTEQ])
	{
		return GTEQ;
	}
	else if(op == TOKENS[LTEQ])
	{
		return LTEQ;
	}
	else if(op == TOKENS[NOTEQUAL])
	{
		return NOTEQUAL;
	}
	else if(op == TOKENS[EQUAL])
	{
		return EQUAL;
	}
	else if(op == TOKENS[LESS])
	{
		return LESS;
	}
	else if(op == TOKENS[NOTEQUAL])
	{
		return NOTEQUAL;
	}

	return 0;
}

statementNode* CompleteParser::CompressNodes(Node& node, vector<statementNode*>& nodes)
{
	struct statementNode* sNode = new statementNode;
	sNode->assign_stmt	= 0;
	sNode->goto_stmt	= 0;
	sNode->if_stmt		= 0;
	sNode->next			= 0;
	sNode->print_stmt	= 0;

	bool skipChildNodes = false;
	if(node.type == "assign_stmt")
	{
		CompileAssignStmt(node, sNode, nodes);

		return sNode;
	}
	else if(node.type == "function_stmt")
	{
		CompileFunctionStmt(node, sNode, nodes);

		return sNode;
	}
	else if(node.type == "if_stmt" || node.type == "while_stmt")
	{
		sNode->if_stmt = new ifStatement;
		sNode->if_stmt->op1 = 0;
		sNode->if_stmt->op2 = 0;
		sNode->if_stmt->relop = 0;
		sNode->if_stmt->true_branch = 0;
		sNode->if_stmt->false_branch = 0;
		sNode->stmt_type = IFSTMT;
		if(node.type == "if_stmt")
			CompileIfStmt(node, sNode, nodes);
		else if(node.type == "while_stmt")
			CompileWhileStmt(node, sNode, nodes);
		return sNode;
	}
	else if(node.type == "repeat_stmt")
	{
		CompileRepeatStmt(node, sNode, nodes);
		return sNode;
	}
	else if(node.type == "print_stmt")
	{
		sNode->print_stmt = new printStatement;
		sNode->stmt_type = PRINTSTMT;
		list<string> ids;

		// Print array... hard-coded again! Deadlines yay!
		if(node.nodes[1].type == "array")
		{
			vector<statementNode*> stmtNodes;
			varAccess* access = CompileExpression(node.nodes[1], stmtNodes);
			for(int i = 0; i < stmtNodes.size(); i++)
			{
				nodes.push_back(stmtNodes[i]);
			}
			sNode->print_stmt->id = access;
		}
		// Print regular variables.
		else
		{
			if(BuildIDList(ids, node.nodes[1]))
			{
				for(list<string>::iterator it = ids.begin(); it != ids.end(); it++)
				{
					Variable* var = m_variables->GetVariable(m_variables->GetVarIDNumber(*it));
					// Add the variable if it does not exist.
					if(!var)
					{
						m_variables->AddVariable(*it, TYPE_UNKNOWN);
						var = m_variables->GetVariable(m_variables->GetVarIDNumber(*it));
					}
					sNode->print_stmt->id = m_variables->GetVarAccess(var->name);
				}
			}
		}
	}
	else
	{
		sNode->stmt_type = NOOPSTMT;
	}

add_node:

	nodes.push_back(sNode);

	if(!skipChildNodes)
	{
		for(int i = 0; i < node.nodes.size(); i++)
		{
			CompressNodes(node.nodes[i], nodes);
		}
	}
	return sNode;
}