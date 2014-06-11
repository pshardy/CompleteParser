#include "CompleteParser.h"

void CompleteParser::RunProgram()
{
	m_openBrackets = 0;
	VerifyNodes(m_nodes);

	m_evaluatingLoop = false;

	EvaluateNodes(m_nodes);
}

void CompleteParser::SetScoping(int level)
{
	m_scoping = level;
}

void CompleteParser::EvaluateOpenNodes()
{
	if(m_currentNode.size())
	{
		m_evaluatingLoop = false;
		EvaluateNodes(*m_currentNode.front());
		if(!IsLooping())
		{
			m_currentNode.pop_front();
			EvaluateOpenNodes();
			m_variables->VerifyDataTypes();
		}
	}
}

bool CompleteParser::DoneRunning()
{
	return !m_evaluatingLoop && m_currentNode.empty();
}

bool CompleteParser::CompleteProgram()
{
	int i;
	Node* node = FindNodeByName("body", m_nodes, i, 3);
	if(node && i >= 0)
	{
		return node->complete;
	}

	return false;
}

int CompleteParser::EvaluateNodes(Node& node)
{
	// Save the state of nodes that have not been run and are on hold until a loop finishes.
	if(m_evaluatingLoop)
	{
		m_currentNode.push_back(&node);
		return TOKEN_ERR_NONE;
	}

	if(node.type == "type_decl")
		return AssignTypes(node);
	else if(node.type == "var_decl")
		return AssignVariables(node);
	else if(node.type == "assign_stmt")
		return SetVariables(node);
	else if(node.type == "while_stmt")
		return WhileStatement(node);
	else if(node.type == "if_stmt")
		return IfStatement(node);
	else if(node.type == "print_stmt")
		return PrintStatement(node);
	else if(node.type == TOKENS[LBRACE])
	{
		if(m_scoping != SCOPING_OFF)
			m_variables->AddScope();
	}
	else if(node.type == TOKENS[RBRACE])
	{
		if(m_scoping != SCOPING_OFF)
			m_variables->RemoveScope();
	}

	for(unsigned int i = 0; i < node.nodes.size(); i++)
	{
		int errCode = EvaluateNodes(node.nodes[i]);
		if(errCode)
			return errCode;
	}

	return TOKEN_ERR_NONE;
}

__declspec(dllexport) void CompleteParser::ClearNodes()
{
	m_nodes.nodes.clear();
	m_nodes.complete = NODE_NOT_COMPLETE;
	m_nodes.closed = false;
	m_currentNode.clear();
	m_evaluatingLoop = false;
	m_currentLine.clear();
	m_textOutput.str(string());
}

int CompleteParser::GetTypeFromTokenStr(string& tokenStr)
{
	string tokenCpy = tokenStr;
	// Check PRIM_INT and PRIM_REAL.
	int id = m_variables->IsDigit(tokenCpy);
	switch(id)
	{
	case PRIM_INT:
		tokenCpy = "PRIM_INT";
		break;
	case PRIM_REAL:
		tokenCpy = "PRIM_REAL";
		break;
	default:
		tokenCpy = "PRIM_STRING";
	}

	// Check type defs.
	id = m_variables->GetTypeIDNumber(tokenCpy);
	if(id == TYPE_UNKNOWN)
	{
		// Check variable types.
		int varID = m_variables->GetVarIDNumber(tokenCpy);
		if(varID != TYPE_UNKNOWN)
		{
			id = m_variables->GetVariable(varID)->typeID;
		}
	}

	return id;
}

string CompleteParser::EvaluateExpression(string& expression, int& typeOut)
{
	stringstream evaluatedExpression;

	stringstream ss(expression);

	string tokensStr[3];
	string tokens[3];

	int count = 0;
	while(ss && count < 3)
	{
		ss >> tokensStr[count++];
	}

	// Load variable values to the tokens instead.
	Variable* var;
	
	for(int i = 0; i < count; i++)
	{
		var = m_variables->GetVariable(m_variables->GetVarIDNumber(tokensStr[i]));
		if(var)
			tokens[i] = var->value[0];
		else
		{
			// A variable is being referenced but has not yet been declared.
			if(GetTokenID(tokensStr[i]) == ID)
			{
				// Add a variable and assign it an internal type.
				m_variables->AddVariable(tokensStr[i], TYPE_UNKNOWN);
				var = m_variables->GetVariable(m_variables->GetVarIDNumber(tokensStr[i]));
			}
			tokens[i] = tokensStr[i];
		}
	}

	// A parenthesis equation may come through. The middle token is the desired token.
	if(tokens[0] == TOKENS[LPAREN] && count >= 1)
	{
		evaluatedExpression << tokens[1];
	}
	else
	{
		// Unless a single number is being passed the count should always be factor, OPERATOR, factor.
		if(count >= 3)
		{
			float token1 = 0.0f, token2 = 0.0f;
			float floatResult = 0.0f;

			int digitType1 = m_variables->IsDigit(tokens[0]);
			int digitType2 = m_variables->IsDigit(tokens[2]);

			// Get float types.
			if(digitType1 == PRIM_REAL)
			{
				token1 = atof(tokens[0].c_str());
			}
			else if(digitType1 == PRIM_INT)
			{
				token1 = atoi(tokens[0].c_str());
			}

			// Get integer types.
			if(digitType2 == PRIM_REAL)
			{
				token2 = atof(tokens[2].c_str());
			}
			else if(digitType2 == PRIM_INT)
			{
				token2 = atoi(tokens[2].c_str());
			}

			// Determine the mathematical operation to apply.
			if(tokens[1] == TOKENS[PLUS])
			{
				floatResult = token1 + token2;
			}
			else if(tokens[1] == TOKENS[MINUS])
			{
				floatResult = token1 - token2;
			}
			else if(tokens[1] == TOKENS[MULT])
			{
				floatResult = token1 * token2;
			}
			else if(tokens[1] == TOKENS[DIV])
			{
				floatResult = token1 / token2;
			}

			stringstream result;
			result << ((digitType1 == PRIM_INT && digitType2 == PRIM_INT) ? (int)floatResult : floatResult);
			string resultStr = result.str();

			// Make sure a decimal point is added to a float. Floats take precedence.
			if(m_variables->IsDigit(resultStr) == PRIM_INT && (digitType1 == PRIM_REAL || digitType2 == PRIM_REAL))
			{
				result << ".0";
			}

			evaluatedExpression << result.str();
		}
		else
		{
			evaluatedExpression << tokens[0];
		}
	}

	// Determine the best type to use. Primitives types are set up to here. Variable types will take over.
	for(int i = 0; i < count; i++)
	{
		if(GetTokenID(tokensStr[i]) == ID)
		{
			var = m_variables->GetVariable(m_variables->GetVarIDNumber(tokensStr[i]));
			if(var)
			{
				int tId = var->typeID;
				if(tId != TYPE_UNKNOWN)
				{
					typeOut = tId;
					break;
				}
			}
		}
	}

	string returnVal = evaluatedExpression.str();

	// No variable types were referenced or constants were entered. Calculate the type from the expression value itself.
	// Constants are recalculated in case a higher priority one is present.
	if(typeOut == TYPE_UNKNOWN || m_variables->TypeIsPrimitive(typeOut))
		typeOut = GetTypeFromTokenStr(returnVal);

	return returnVal;
}

__declspec(dllexport) string CompleteParser::CreateExpression(Node& node)
{
	if(node.nodes.empty()) return node.value;

	string result;

	for(int i = 0; i < node.nodes.size(); i++)
	{
		result.append(CreateExpression(node.nodes[i]));

		if(i < node.nodes.size() - 1)
			result.append(" ");
	}
		
	return result;
}

string CompleteParser::CreateExpression(Node& node, int& typeOut)
{
	if(node.nodes.empty()) return node.value;

	string result;

	for(int i = 0; i < node.nodes.size(); i++)
	{
		result.append(CreateExpression(node.nodes[i], typeOut));
		if(i < node.nodes.size() - 1)
			result.append(" ");
	}
		
	return EvaluateExpression(result, typeOut);
}

int CompleteParser::PrintStatement(Node& node)
{
	list<string> ids;
	// Build the list to be printed.
	if(node.nodes.size() < 2)
		return 1; // TODO: P_ERROR CODE

	// Standard print.
	if(BuildIDList(ids, node.nodes[1]))
	{
		for(list<string>::iterator it = ids.begin(); it != ids.end(); it++)
		{
			Variable* var = m_variables->GetVariable(m_variables->GetVarIDNumber(*it));
			if(!var)
			{
				m_variables->AddVariable(*it, TYPE_UNKNOWN);
				var = m_variables->GetVariable(m_variables->GetVarIDNumber(*it));
			}
			if(var)
			{
				// For gui.
				m_textOutput << var->name << ": " << var->value[0] << "\n";
			}
		}
	}
	// Debug print
	else
	{
		int i;
		Node* debugNode = FindNodeByName((char*)TOKENS[TOKEN_DEBUG], node, i);
		if(debugNode)
		{
			m_variables->PrintTypes(m_textOutput);
		}
	}

	return TOKEN_ERR_NONE;
}

int CompleteParser::WhileStatement(Node& node)
{
	int conditionI = -1;
	Node* conditionNode = FindConditionNode(node, conditionI);

	if(!conditionNode || conditionI - 1 < 0 || conditionI >= conditionNode->nodes.size())
		return 1; // TODO: P_ERROR CODE

	if(EvaluateCondition(conditionNode->nodes[conditionI]))
	{
		int i = -1;
		Node* bodyNode = FindBodyNode(*conditionNode, i);
		if(!bodyNode || i - 1 < 0 || i >= bodyNode->nodes.size())
			return 1; // NO BODY

		int errCode = EvaluateNodes(bodyNode->nodes[i]);

		// Add the current node being processed to the queue.
		if(find(m_currentNode.begin(), m_currentNode.end(), &node) == m_currentNode.end())
		{
			m_currentNode.push_back(&node);
		}

		m_evaluatingLoop = true;

		if(errCode)
			return errCode;

		return TOKEN_ERR_NONE;
	}

	m_evaluatingLoop = false;

	return TOKEN_ERR_NONE;
}

int CompleteParser::IfStatement(Node& node)
{
	int conditionI = -1;
	Node* conditionNode = FindConditionNode(node, conditionI);

	if(!conditionNode || conditionI - 1 < 0 || conditionI >= conditionNode->nodes.size())
		return 1; // TODO: P_ERROR CODE

	// IF
	if(EvaluateCondition(conditionNode->nodes[conditionI]))
	{
		goto evaluate_body;
	}
	else
	{
		// ELSE IF
		int i = -1;
		Node* elseIfNode = FindNodeByName("else_stmt", node, i, 1);
		if(elseIfNode && i >= 0)
		{
			elseIfNode = FindNodeByName("if_stmt", elseIfNode->nodes[i], i, 1);
			if(elseIfNode && i >= 0)
				return IfStatement(elseIfNode->nodes[i]);
		}

		// ELSE
		Node* elseNode = FindNodeByName("else_stmt", node, i, 2);
		if(elseNode && i >= 0)
		{
			conditionNode = &elseNode->nodes[i];
			goto evaluate_body;
		}
	}

	return TOKEN_ERR_NONE;

evaluate_body:
	int i = -1;
	Node* bodyNode = FindBodyNode(*conditionNode, i);
	if(!bodyNode || i - 1 < 0 || i >= bodyNode->nodes.size())
		return 1; // NO BODY

	return EvaluateNodes(bodyNode->nodes[i]);
}

bool CompleteParser::EvaluateCondition(Node& node)
{
	// Get the comparison operater which divides the statement.
	int i = -1;
	int typeID = -1;

	Node* assignmentNode = FindComparisonOp(node, i);

	// There is only one node responsible for the condition. Evaluate it and return the result.
	if(!assignmentNode)
	{
		string expression = CreateExpression(node, typeID);
		return (bool)atof(expression.c_str());
	}

	if(i - 1 < 0 || i + 1 >= assignmentNode->nodes.size())
		return false;  // TODO: P_ERROR CODE

	// Take apart the condition and compare each side.
	float expression1 = atof(CreateExpression(assignmentNode->nodes[i - 1], typeID).c_str());
	typeID = -1;
	float expression2 = atof(CreateExpression(assignmentNode->nodes[i + 1], typeID).c_str());
	typeID = -1;
	string op = CreateExpression(assignmentNode->nodes[i], typeID);

	if(op == ">")
		return expression1 > expression2;
	else if(op == "<")
		return expression1 < expression2;
	else if(op == "<=")
		return expression1 <= expression2;
	else if(op == ">=")
		return expression1 >= expression2;
	else if(op == "<>")
		return expression1 != expression2;
	else if(op == "=")
		return expression1 == expression2;

	return false;
}

int CompleteParser::SetVariables(Node& node)
{
	list<string> ids;

	// Get the assignment operater which divides the statement.
	int i = -1;
	Node* assignmentNode = FindAssignmentOp(node, i);

	// Build the left hand side.
	if(!assignmentNode || (i - 1 < 0 || !BuildIDList(ids, assignmentNode->nodes[i - 1]) || i + 1 >= assignmentNode->nodes.size()))
		return 1; // TODO: P_ERROR CODE

	// Build the right hand side.
	int typeID = -1;
	string expression = CreateExpression(assignmentNode->nodes[i + 1], typeID);

	// Set variables.
	for(list<string>::iterator it = ids.begin(); it != ids.end(); it++)
	{
		if(!m_variables->SetVar(*it, expression, typeID))
			cout << "P_ERROR: Could not set variable.\n";
	}

	return TOKEN_ERR_NONE;
}

void CompleteParser::InitializeVariables(Node& node)
{
	list<string> ids;

	// Get the assignment operater which divides the statement.
	int i = -1;
	Node* assignmentNode = FindAssignmentOp(node, i);

	BuildIDList(ids, node.nodes[0]);

	// Variable declare only.
	if(i == -1)
	{
		// Assign variables.
		for(list<string>::iterator it = ids.begin(); it != ids.end(); it++)
		{
			m_variables->AddVariable(*it, TYPE_UNKNOWN);
		}

		return;
	}

	// Arrays.
	int size = atoi(node.nodes[4].value.c_str());

	if(size <= 0)
		size = 1;

	// Assign variables.
	for(list<string>::iterator it = ids.begin(); it != ids.end(); it++)
	{
		m_variables->AddVariable(*it, TYPE_UNKNOWN, size);
	}

}

int CompleteParser::AssignTypes(Node& node)
{
	list<string> ids;

	// Get the assignment operater which divides the statement.
	int i = -1;
	Node* assignmentNode = FindAssignmentOp(node, i);

	// Build the left hand side.
	if(!assignmentNode || (i - 1 < 0 || !BuildIDList(ids, assignmentNode->nodes[i - 1]) || i + 1 >= assignmentNode->nodes.size()))
		return 1; // TODO: P_ERROR CODE

	// Build the right hand side.
	Node* typeNode = GetLowestRightNode(assignmentNode->nodes[i + 1]);

	// Assign type to typedef.
	for(list<string>::iterator it = ids.begin(); it != ids.end(); it++)
	{
		if(!m_variables->AddType(*it, typeNode->value))
			cout << "P_ERROR: Type redeclared.\n";
	}

	return TOKEN_ERR_NONE;
}

int CompleteParser::AssignVariables(Node& node)
{
	list<string> ids;

	// Get the assignment operater which divides the statement.
	int i = -1;
	Node* assignmentNode = FindAssignmentOp(node, i);

	// Build the left hand side.
	if(!assignmentNode || (i - 1 < 0 || !BuildIDList(ids, assignmentNode->nodes[0]) || i + 1 >= assignmentNode->nodes.size()))
		return 1; // TODO: P_ERROR CODE

	// Build the right hand side.
	Node* typeNode = GetLowestRightNode(assignmentNode->nodes[i + 1]);

	// See if an array is being declared.
	Node* arrayNode = FindArray(node, i);

	int size = 1;
	if(arrayNode)
	{
		int type;
		// This is for an array of constant size. Evaluation can be done by the compiler.
		string str = CreateExpression(arrayNode->nodes[i + 1], type);
		size = atoi(str.c_str());
	}

	// Assign variables their type.
	for(list<string>::iterator it = ids.begin(); it != ids.end(); it++)
	{
		if(!m_variables->AddVariable(*it, typeNode->value, size))
			cout << "P_ERROR: Variable redeclared.\n";
	}

	return TOKEN_ERR_NONE;
}

Node* CompleteParser::FindBodyNode(Node& node, int& nodeIndex)
{
	nodeIndex = -1;
	if(node.type == "body" && node.complete)
		return &node;

	for(int i = 0; i < node.nodes.size(); i++)
	{
		if(FindBodyNode(node.nodes[i], nodeIndex))
		{
			nodeIndex = i;
			return &node;
		}
	}

	return 0;
}

Node* CompleteParser::FindConditionNode(Node& node, int& nodeIndex)
{
	return FindNodeByName("condition", node, nodeIndex, 2);
}

Node* CompleteParser::FindNodeByName(char* name, Node& node, int& nodeIndex, int searchLevels)
{
	string nameStr = name;
	return FindNodeByName(nameStr, node, nodeIndex, searchLevels);
}

Node* CompleteParser::FindNodeByName(string& name, Node& node, int& nodeIndex, int searchLevels)
{
	nodeIndex = -1;

	// Recursive search stops when level is 0.
	if(searchLevels == 0)
		return 0;

	if(node.type == name)
	{
		return &node;
	}

	for(int i = 0; i < node.nodes.size(); i++)
	{
		Node* returnNode = FindNodeByName(name, node.nodes[i], nodeIndex, searchLevels - 1);
		if(returnNode)
		{
			if(nodeIndex == -1)
				nodeIndex = i;
			return returnNode;
		}
	}

	return 0;
}

Node* CompleteParser::FindOp(Node& node)
{
	if(node.type == ">" || node.type == ">=" || node.type == "<=" || node.type == "<" ||
		node.type == "<>" || node.type == "=" || node.type == "*" || node.type == "+" ||
		node.type == "-" || node.type == "/")
		return &node;

	for(int i = 0; i < node.nodes.size(); i++)
	{
		Node* found = FindOp(node.nodes[i]);
		if(found)
			return found;
	}

	return 0;
}

Node* CompleteParser::FindComparisonOp(Node& node, int& nodeIndex)
{
	nodeIndex = -1;
	if(node.type == ">" || node.type == ">=" || node.type == "<=" || node.type == "<" ||
		node.type == "<>" || node.type == "=")
		return &node;

	for(int i = 0; i < node.nodes.size(); i++)
	{
		if(FindComparisonOp(node.nodes[i], nodeIndex))
		{
			nodeIndex = i;
			return &node;
		}
	}

	return 0;
}

Node* CompleteParser::FindArray(Node& node, int& nodeIndex)
{
	nodeIndex = -1;
	if(node.type == "[")
		return &node;

	for(int i = 0; i < node.nodes.size(); i++)
	{
		// Return the parent node to the array brackets and the index value.
		Node* foundNode = FindArray(node.nodes[i], nodeIndex);
		if(foundNode)
		{
			if(nodeIndex == -1)
			{
				// Make sure there aren't parent nodes breaking down to just one node.
				if(node.nodes.size() > 1)
					nodeIndex = i;
				return &node;
			}
			return foundNode;
		}
	}

	return 0;
}

Node* CompleteParser::FindAssignmentOp(Node& node, int& nodeIndex)
{
	nodeIndex = -1;
	if(node.type == "=" || node.type == ":")
		return &node;

	for(int i = 0; i < node.nodes.size(); i++)
	{
		// Return the parent node.
		Node* foundNode = FindAssignmentOp(node.nodes[i], nodeIndex);
		if(foundNode)
		{
			if(nodeIndex == -1)
			{
				// Make sure there aren't parent nodes breaking down to just one node.
				if(node.nodes.size() > 1)
					nodeIndex = i;
				return &node;
			}
			return foundNode;
		}
	}

	return 0;
}

bool CompleteParser::BuildIDList(list<string>& ids, Node& node)
{
	if(node.type == "ID")
	{
		ids.push_back(node.value);
		return true;
	}

	if(node.nodes.empty())
		return false;

	bool found = false;

	for(unsigned int i = 0; i < node.nodes.size(); i++)
	{
		if(node.nodes[i].type == "ID")
		{
			ids.push_back(node.nodes[i].value);
			found = true;
		}
		else
		{
			if(BuildIDList(ids, node.nodes[i]))
				found = true;
		}
	}

	return found;
}