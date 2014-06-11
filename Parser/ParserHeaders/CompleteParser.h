////////////////////////////////////////////////////////////////////////////////
// Filename: CompleteParser.h
////////////////////////////////////////////////////////////////////////////////
#ifndef _COMPLETEPARSER_H_
#define _COMPLETEPARSER_H_

#include "compiler.h"
#include "Input.h"

// ID TYPE
#define DIGIT				0
#define LETTER				1
#define OTHER				2

// Grammar stages.
#define GRMR_NONTERMINALS	0
#define GRMR_TERMINALS		1
#define GRMR_RULES			2
#define GRMR_FINISHED		3
#define PROGRAM_INPUT		4

// Special output tokens.
#define TOKEN_EPSILON		"#"
#define TOKEN_EOF			"$"

// TOKEN_DEBUG
#define PRINT_OUTPUT		1
#define PRINT_RULES			0

// Scoping
#define SCOPING_OFF			0
#define SCOPING_STATIC		1


// Indices for tokens.
#define TOKEN_UNKNOWN		-1


// Error codes when parsing or analyzing.
// Subtract 1 to meet specification on output.
#define TOKEN_ERR_NONE				0
#define TOKEN_ERR_SYNTAX			1
#define TOKEN_ERR_NT_NOT_DEFINED	2
#define TOKEN_ERR_NOT_DECLARED		3
#define TOKEN_ERR_NON_MODIFIABLE	4
#define TOKEN_ERR_NON_REACHABLE		5

#define BASE_NODE_TYPE				"program"

using namespace std;

struct CompleteParserErrors
{
	CompleteParserErrors()
	{
		m_criticalError = false;
	};

	~CompleteParserErrors()
	{

	};

	bool AddError(int code)
	{
		if(find(m_errorCodes.begin(), m_errorCodes.end(), code) == m_errorCodes.end())
		{
			if(code == TOKEN_ERR_SYNTAX)
				m_criticalError = true;

			m_errorCodes.push_back(code);
			m_errorCodes.sort();
			return true;
		}
		return false;
	};

	void PrintErrors()
	{
		if(IsCritical())
			printf("P_ERROR CODE %i", TOKEN_ERR_SYNTAX - 1);
		else
			for(list<int>::iterator it = m_errorCodes.begin(); it != m_errorCodes.end(); it++)
			{
				printf("P_ERROR CODE %i\n", *it - 1);
			}
	};

	bool HasErrors()
	{
		return m_errorCodes.size() > 0;
	}

	bool IsCritical()
	{
		return m_criticalError;
	};

private:
	list<int>	m_errorCodes;				// List of unique error codes.
	bool		m_criticalError;			// Critical errors flag the program to abort processing.
};

struct StackMemory
{
	StackMemory()
	{
		mem = 0;
	};
	~StackMemory() {};

	void* mem;
};

void InitializeStatementNode(statementNode* node);

#define NODE_NOT_COMPLETE	0
#define NODE_IS_COMPLETE	1
#define NODE_MAYBE_COMPLETE	2

struct Node
{
	Node()
	{
		closed = false;
		complete = NODE_NOT_COMPLETE;
		lineNumber = 0;
	};

	~Node() {};
	vector<Node> nodes;						// Child nodes.
	string type;					 		// The token this node represents.
	string value;							// The value of the node.
	int lineNumber;							// The line number from the original code.
	int complete;							// 1 when rule matched completely. 2 when partial match found.
	bool closed;							// True when follow set matched.
};

////////////////////////////////////////////////////////////////////////////////
// Class name: CompleteParser
//
// Handles data structures and parsing algorithms.
////////////////////////////////////////////////////////////////////////////////
class CompleteParser
{
public:
	CompleteParser();
	~CompleteParser();

	bool Initialize(Input* inputPtr);
	void Shutdown();
	void ShutdownProgram(statementNode*);			// Free memory from compiled program.

	__declspec(dllexport) bool Update();
	void RunProgram();
	// Compiling
	__declspec(dllexport) statementNode* Compile();	// Compiles the nodes into an executable graph.

	bool DoneRunning();								// True once the program has completed.
	void EvaluateOpenNodes();
	__declspec(dllexport) void ClearNodes();		// Clear the syntax parse tree.
	void SetScoping(int level);						// Determine the scoping level based on user selection.
	int GetOpenBrackets();							// Returns m_openBrackets.
	int GetGrammarStage();							// Returns m_grammarStage.
	void SetGrammarStage(int);						// Set the stage the parser is expecting.

	void PrintErrors();								// Print any errors.
	void PrintRules();								// Print all keys, sets, and rules.
	void PrintFirstSets();							// Print all first entries.
	void PrintFollowSets();							// Print all follow entries.
	bool IsLooping();								// Determines if a loop is processing.
	__declspec(dllexport) Variables* GetVariables();// Return m_variables.
	__declspec(dllexport) Node* GetNodes();			// Return m_nodes;
	__declspec(dllexport) string GetTextOutput();	// Returns m_textOutput;
	__declspec(dllexport) string CreateExpression(Node& node);
private:
	// General
	int GetTypeFromTokenStr(string& tokenStr);		// Determine the ID number of a token. Handles variables, types, and constants.
	int GetTokenType(char c);						// Retrieve the type of the new token.
	int GetIDType(int c);							// Determine if the char is a letter, digit, or other.
	int HandleToken(int token);						// Logic behind token operations on a stage basis.
	
	int HandleTokenID(string& tokenStr);			// When an ID is entered. Returns error code.
	bool HandleError(int errorCode);				// Determine action on an error. Return code of true indicates serious error.
	bool IsTokenTerminal(string& tokenStr);			// Check if a token is a terminal.
	bool IsTokenNonTerminal(string& tokenStr);		// Check if a token is a non-terminal.
	
	// Data Processing
	statementNode* CompressNodes(Node& node,		// Compress nodes into a singular list in order of execution.
		vector<statementNode*>&);	
	void CompileAssignStmt(Node&, statementNode*,
		vector<statementNode*>&);
	void CompileIfStmt(Node&, statementNode*,
		vector<statementNode*>&,
		bool includeBody = true);
	void CompileWhileStmt(Node&, statementNode*,
		vector<statementNode*>&);
	void CompileRepeatStmt(Node&, statementNode*,
		vector<statementNode*>&);
	void CompileFunctionStmt(Node& node, statementNode*,
		vector<statementNode*>&);
	int OperationToTokenType(string& operation);
	//varAccess* GetOrCreateVarAccess(string& val);
	//varNode* GetOrCreateVarNode(string& val);		// Val can be constant or variable.
	varAccess* CompileExpression(Node& node,
		vector<statementNode*>& stmtList);

	void InitializeVariables(Node& node);

	bool CompleteProgram();							// Checks if the base node is complete.
	int EvaluateNodes(Node& node);					// Determine data types and variables.
	int AssignTypes(Node& node);					// Assign a type to a list.
	int AssignVariables(Node& node);				// Assign a variable to a list.
	int SetVariables(Node& node);					// Set a variable.
	int WhileStatement(Node& node);					// Handles a while statement.
	int IfStatement(Node& node);					// Handles an if statement.
	int PrintStatement(Node& node);					// Handles a print statement.
	bool EvaluateCondition(Node& node);				// Determines if a condition is true or not.
	
	string CreateExpression(Node& node,				// Return a string of a non-evaluated expression. The typeout is the best type ID to use
		int& typeIDOut);							// based on the terms of the expression.
	string EvaluateExpression(string& in,			// Evaluate an expression following order of operations.
		int& typeIDOut);				
	bool BuildIDList(list<string>&, Node& node);	// Build a list of string IDs from a node list. Type must be ID and they can be COMMA separated.
	Node* FindArray(Node& node, int& index);
	Node* FindAssignmentOp(Node& node, int& index);	// Find the highest most node whos children fall on both sides of an assignment operator.
	Node* FindComparisonOp(Node& node, int& index);	// Find the highest most node whos children fall on both sides of a comparison operator.
	Node* FindOp(Node& node);						// Returns the node consisting of an operator.
	Node* FindConditionNode(Node& node,				// Find the highest mode node whos type is 'condition'.
		int& index);
	Node* FindNodeByName(char* name, Node& node,	// Search for a node by its friendly name. The index sets to the vector index found of the node
		int& index, int searchLevels = -1);			// containing the search node. Search levels indicate number of recursive calls to make.
	Node* FindNodeByName(string& name, Node& node,	// Search for a node by its friendly name. The index sets to the vector index found of the node
		int& index, int searchLevels = -1);			// containing the search node. Search levels indicate number of recursive calls to make.
	Node* FindBodyNode(Node& node, int& index);		// Find the node of type body.

	// Syntax Handling
	int GetTokenID(string& tokenStr);
	int HandleTokenIDSyntax(string& tokenStr);		// Specific logic for syntax handling.
	int EvaluateLine(list<string>& line);			// Determine the nodes to use or create for the given line.
	bool FindFirstSets(list<string>& sets,			// Searches first sets to build a list of potential rules.
		int tokenID);
	bool IsTokenFollow(string& nonTerminal,			// Determine if the token is a follow set to the non-token rule.
		string& token);
	Node* GetFarthestOpenNode(Node& node);			// Find the furthest node that has not been closed.
	Node* GetFarthestExpandableNode(Node& node);	// Find the furthest node that can accept additional nodes.
	Node* GetLowestRightNode(Node& node);			// Return the lowest node in the parse tree.
	bool CloseNodeChildren(Node& node);				// Determine if the node's children should be closed.
	bool CloseNodeIfChildTerminal(Node& node);		// Close a node if all of its children are terminals.
	void ForceCloseNode(Node& node);				// Force close a node and all child nodes.
	bool MatchLineToRule(list<string>& line,		// Pops the front of the line for every word matching the token.
		list<string> lineCpy, string& nontoken,		// This constructs a complete parse tree to be used for evaluation.
		Node& node, int sIndex = 0);				// The sIndex is used when revisiting a node that was not completed.
	void VerifyNodes(Node& node);					// Verify proper syntax on nodes.

	// Grammar Handling
	bool IsTokenRuleKey(string& tokenStr);			// Check if a token is the key to a rule.
	bool IsTokenInRule(string& tokenStr);			// Check if a token is in the right hand side of a rule.
	int FindStartTerminalsOfRules(string& token,	// For a given token, find all the terminals
		list<string>& terminals,					// by traversing the rules. Return code based on token.
		list<StackMemory*>&);		
	int FindFollowTerminalsOfRules(string& token,	// For a given token, find all the terminals
		list<string>& terminals,					// by traversing the rules. Return code based on token.
		list<StackMemory*>&);		
	void CalculateFirstAndFollowSets();				// Set m_firstSets and m_followSets.
	void AddEOFToSet(list<string>&);				// Adds the eof token to a set.
	void RemoveCString(string&);					// Removes the null terminator if it exists.
	void EndParsing();								// Called when all parsing is complete.
private:
	map<string,										// The set of rules for the terminals and non-terminals.
		vector<list<string> > > m_rules;			// Each key can have a vector of a list of rules.
	list<string>*		m_currentRule;				// The current rule being assigned.
	list<string>		m_currentLine;				// The current line pending evaluation.
	map<string,
		list<string> >	m_firstSets;				// The first sets of the grammar rules.
	map<string,
		list<string> >	m_followSets;				// The follow sets of the grammar rules.
	Node			m_nodes;						// The nodes of the program based on the grammar.
	list<Node*>		m_currentNode;					// The current node being evaluated. Used for single threaded loops.
	Variables*		m_variables;					// The variables the program may use.
	CompleteParserErrors	m_errors;						// List of errors found during parsing or analyzing.
	Input*			m_inputBuffer;					// Buffer which is a pointer to the input object.
	list<string>	m_nonTerminals;					// Linked list of user defined non terminals.
	list<string>	m_terminals;					// Linked list of user defined terminals.
	stringstream	m_textOutput;					// Text output generated by the program.
	int				m_scoping;						// The scoping level of the program.
	int				m_openBrackets;					// Number of brackets currently not closed.
	int				m_tokenStart, m_tokenEnd;		// Start and end indices of the token.
	int				m_grammarStage;					// What stage the parser is on in defining the grammar.
	int				m_currentRuleList;				// Non-terminals may have different sets of rules to follow.
	bool			m_startOfRule;					// If the parser is expecting a new rule for a non-terminal.
	bool			m_evaluatingLoop;				// True when a loop is being evaluated.
	bool			m_consoleMode;					// If the console is active.
};

#endif