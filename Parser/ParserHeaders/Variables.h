////////////////////////////////////////////////////////////////////////////////
// Filename: Variables.h
////////////////////////////////////////////////////////////////////////////////
#ifndef _VARIABLES_H_
#define _VARIABLES_H_

#include <map>
#include <list>
#include <vector>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <string.h>

using namespace std;

#define TYPE_UNKNOWN		-1


#define VAR					1
#define IF					2
#define WHILE				3 
#define REPEAT				4
#define UNTIL 				5
#define PRINT 				6
#define ARRAY 				7
#define PLUS 				8
#define MINUS 				9
#define DIV 				10
#define MULT 				11
#define EQUAL 				12
#define COLON 				13
#define COMMA 				14
#define SEMICOLON 			15
#define LBRAC 				16
#define RBRAC 				17
#define LPAREN 				18
#define RPAREN 				19
#define LBRACE 				20
#define RBRACE 				21
#define NOTEQUAL 			22
#define GREATER 			23
#define LESS 				24
#define ID 					25
#define NUM 				26
#define P_ERROR 			27
#define PRIM_INT			28
#define PRIM_REAL			29
#define PRIM_STRING			30
#define PRIM_BOOL			31
#define ELSE				32
#define GTEQ				33
#define LTEQ				34
#define TOKEN_ID			35
#define TOKEN_HASH			36
#define TOKEN_DOUBLEHASH	37
#define TOKEN_ARROW			38
#define TOKEN_DEBUG			39
#define QUOTE				40
#define ACTION				41
#define NOOP				42


static const char *TOKENS[] = 
{	"",
	"VAR",
	"IF",
	"WHILE",
	"REPEAT",
	"UNTIL",
	"print",
	"ARRAY",
	"+",
	"-",
	"/",
	"*",
	"=",
	":",
	",",
	";",
	"[",
	"]",
	"(",
	")",
	"{",
	"}",
	"<>",
	">",
	"<",
	"ID",
	"NUM",
	"P_ERROR",
	"PRIM_INT",
	"PRIM_REAL",
	"PRIM_STRING",
	"PRIM_BOOL",
	"ELSE",
	">=",
	"<=",
	"ID",
	"#",
	"##",
	"->",
	"debug",
	"\"",
	"ACTION",
	"NOOP"
};

const int RESERVED_COUNT = sizeof(TOKENS)/sizeof(TOKENS[0]);

struct Variable
{
	void Set(string val, int index)
	{
		while(index >= value.size())
			value.push_back("0");
		value[index] = val;
	};
	string Get(int index)
	{
		if(index >= value.size())
			Set("0", index);

		return value[index];
	};
	vector<string> value;
	string name;
	int typeID;
};

struct varAccess
{
	struct Variable*	var;
	struct Variable*	index;
};

struct Scope
{
	list<int> variables;
	list<int> types;
};

class Variables
{
public:
	Variables();
	~Variables();

	bool AddPrimitive(string& type);
	bool AddType(string& type,						// Adds a type and assigns an ID.
		bool internalOnly = false);					// Internal Only will auto designate a name.
	bool AddType(string& typeDef, string& type);	// Create a typedef for a given type.

	void InitializeVariable(Variable& var);			// Initialize a variable value depending on the type.

	void VerifyDataTypes();							// Checks all known variables and their data types converting them if needed.
													// Useful for when a variable of an unknown type is used and then set to a primitive later.

	void AddVariableToScope(int varID);				// Adds a variable ID to the back most scope.

	void AddTypeToScope(int typeID);				// Adds a type ID to the back most scope.

	void AddScope();								// Adds a scope to the back.

	void RemoveScope();								// Removes a scope from the back.

	__declspec(dllexport) bool AddVariable(string& varName,				// Add a variable with a type that may not be determined.
		string& type, int size = 1);

	__declspec(dllexport) bool AddVariable(string& varName, int typeID,
		int size = 1);								// Add a variable with a pre-calculated type. TYPE_UNKNOWN creates a pure internal type.

	__declspec(dllexport) bool AddVarNode(Variable&, int varID, int index);

	__declspec(dllexport) varAccess* GetOrCreateVarAccess(string& name);
	__declspec(dllexport) struct varAccess* GetVarAccess(string& name);
	//struct varNode* GetVarNode(string& name);

	struct Variable* AddTempVariable();

	__declspec(dllexport) bool SetVar(string& varName, string& value,		// Set a variable to a value. Will create the variable if required.
		int type, int index = 0);	

	int IsDigit(string& str);						// Determines if the string is PRIM_INT, PRIM_REAL, or neither.

	int IsTokenInternalType(string& tokenStr);		// Match against TOKENS.

	bool CheckTypes(string& type1, string& type2);	// Checks if two types are compatable.

	bool CheckTypes(int id1, int id2);

	bool VarIsDeclared(int varID);					// Checks m_variables for the given var ID.

	bool TypeIsPrimitive(int typeID);
	bool TypeIsDeclared(int typeID);
	bool TypeIsTypeDef(int typeID);

	int GetVarIDNumber(string& var);						// Get the ID number of a variable.
	__declspec(dllexport) int GetTypeIDNumber(string& type);// Get the ID number of a type.
	__declspec(dllexport) int GetLowestType(int typeID);	// Return the lowest type ID for a given type.

	bool ConvertValue(string& value, int typeID);
	__declspec(dllexport) string GetTypeString(int typeID);	// Return the friendly name of the type ID.

	Variable* GetVariable(string& var);
	Variable* GetVariable(int varID);				// Return pointer to variable by ID.
	map<int, Variable>* GetVariables();				// Returns the current map of variables.

	void PrintVariables();
	void PrintTypes(stringstream&);
	__declspec(dllexport) void Clear();				// Clear all variables and types.
private:
	map<int, string>		m_types;				// The loaded data types.
	map<int, string>		m_primitives;			// The primitive types.
	map<int, int>			m_typeDefs;				// The ID of one type referencing another type.
	map<int, Variable>		m_variables;			// The loaded variables.
	//map<int, 
	//	varNodeCPP>			m_varNodes;				// For compiling purposes.
	list<Scope>				m_scopes;				// Variables loaded for each scope.
	int						m_internalType;			// The current internal id being assigned to types.
	int						m_internalVariable;		// The current internal id being assigned to variables.
	int						m_tempVariableCount;	// The system assigns temporary variables for expressions.
	int						m_constantCount;		// Number of constants declared.
};

#endif