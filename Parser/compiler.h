#ifndef _COMPILER_H_
#define _COMPILER_H_

#include "Variables.h"

/*
 * compiler.h
 * 
 */
 
#define TRUE 1
#define FALSE 0

#define NOOPSTMT	100
#define PRINTSTMT	101
#define ASSIGNSTMT	102
#define IFSTMT		103		// This is used for all control statements (if, while, repeat)
#define GOTOSTMT	104
#define FUNCSTMT	105
#define _CONSOLE 0			// Disable for GUI. Requires Windows and .NET 4.0.

extern Variables* variablesPtr;

//---------------------------------------------------------
// Data structures:

struct gotoStatement
{
	struct statementNode * target;
};

struct assignmentStatement
{
	struct varAccess * lhs;
	struct varAccess * op1;
	struct varAccess * op2;
	int op;		// PLUS, MINUS, MULT, DIV --> lhs = op1 op op2;
				// 0                      --> lhs = op1;
};

// Currently a place holder for a more advanced structure.
struct functionStatement
{
	struct gotoStatement * goto_stmt;
	string funcName;							// The name of the function being called.
	struct varAccess* argument;
};

struct printStatement
{
	struct varAccess * id;
};

struct ifStatement
{
	struct varAccess * op1;
	struct varAccess * op2;
	int relop;	// GREATER, LESS, NOTEQUAL
	struct statementNode * true_branch;
	struct statementNode * false_branch;
};

struct statementNode
{
	int stmt_type;								// NOOPSTMT, PRINTSTMT, ASSIGNSTMT, IFSTMT, GOTOSTMT
	struct assignmentStatement	* assign_stmt;	// NOT NULL iff stmt_type == ASSIGNSTMT
	struct functionStatement	* func_stmt;	// NOT NULL iff	func_stmt == FUNCSTMT
	struct printStatement		* print_stmt;	// NOT NULL iff stmt_type == PRINTSTMT
	struct ifStatement			* if_stmt;		// NOT NULL iff stmt_type == IFSTMT
	struct gotoStatement		* goto_stmt;	// NOT NULL iff stmt_type == GOTOSTMT
	struct statementNode		* next;			// next statement in the list or NULL 
};

__declspec(dllexport) void print_debug(const char * format, ...);

void execute_program(statementNode*);

#endif /* _COMPILER_H_ */
