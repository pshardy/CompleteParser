#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>
#include "compiler.h"

Variables* variablesPtr;

#define TOKEN_DEBUG 1	 // 1 => Turn ON debugging, 0 => Turn OFF debugging

__declspec(dllexport) void print_debug(const char * format, ...)
{
	va_list args;
	if (TOKEN_DEBUG)
	{
		va_start (args, format);
		vfprintf (stdout, format, args);
		va_end (args);
	}
}

char *reserved[] = 
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
		"P_ERROR"
	 };

//---------------------------------------------------------
// Execute
void execute_program(struct statementNode* program)
{
	struct statementNode * pc = program;
	float op1, op2, result;
	int index, index1, index2;

	while (pc != NULL)
	{
		switch (pc->stmt_type)
		{
			case NOOPSTMT:
				pc = pc->next;
				break;

			case PRINTSTMT:
				if (pc->print_stmt == NULL)
				{
					print_debug("Error: pc points to a print statement but pc->print_stmt is null.\n");
					exit(1);
				}
				if (pc->print_stmt->id == NULL)
				{
					print_debug("Error: print_stmt->id is null.\n");
					exit(1);
				}
				// Get the index of the array.
				index = pc->print_stmt->id->index ? atoi(pc->print_stmt->id->index->Get(0).c_str()) : 0;
				cout << pc->print_stmt->id->var->Get(index) << "\n";
				pc = pc->next;
				break;

			case ASSIGNSTMT:
				{
					if (pc->assign_stmt == NULL)
					{
						print_debug("Error: pc points to an assignment statement but pc->assign_stmt is null.\n");
						exit(1);
					}
					if (pc->assign_stmt->op1 == NULL)
					{
						print_debug("Error: assign_stmt->op1 is null.\n");
						exit(1);
					}
					if (pc->assign_stmt->op == PLUS || pc->assign_stmt->op == MINUS
						|| pc->assign_stmt->op == MULT || pc->assign_stmt->op == DIV)
					{
						if (pc->assign_stmt->op2 == NULL)
						{
							print_debug("Error: right-hand-side of assignment is an expression but assign_stmt->op2 is null.\n");
							exit(1);
						}
					}
					if (pc->assign_stmt->lhs == NULL)
					{
						print_debug("Error: assign_stmt->lhs is null.\n");
						exit(1);
					}
					// Get the index of the array.
					index1 = pc->assign_stmt->op1->index ? atoi(pc->assign_stmt->op1->index->Get(0).c_str()) : 0;

					op1 = atof(pc->assign_stmt->op1->var->Get(index1).c_str());

					int id	= variablesPtr->GetLowestType(pc->assign_stmt->lhs->var->typeID);
					int id1 = variablesPtr->GetLowestType(pc->assign_stmt->op1->var->typeID);
					int id2 = 0;

					int typeToUse = id;

					if(pc->assign_stmt->op2 != 0)
					{
						index2 = pc->assign_stmt->op2->index ? atoi(pc->assign_stmt->op2->index->Get(0).c_str()) : 0;
						op2 = atof(pc->assign_stmt->op2->var->Get(index2).c_str());
						id2 = variablesPtr->GetLowestType(pc->assign_stmt->op2->var->typeID);
					}

					int idType = id;

					stringstream ss;
					if(variablesPtr->GetTypeString(id) == string(TOKENS[PRIM_STRING]) || 
						variablesPtr->GetTypeString(id1) == string(TOKENS[PRIM_STRING]) || variablesPtr->GetTypeString(id2) == string(TOKENS[PRIM_STRING]))
					{
						string resultStr;
						resultStr = pc->assign_stmt->op1->var->Get(index1);
						switch (pc->assign_stmt->op)
						{
						case PLUS:
							resultStr.append(pc->assign_stmt->op2->var->Get(index2));
							break;
						}

						typeToUse = variablesPtr->GetTypeIDNumber(string(TOKENS[PRIM_STRING]));

						ss << resultStr;
					}
					else
					{
						switch (pc->assign_stmt->op)
						{
							case PLUS:
								result = op1 + op2;
								break;
							case MINUS:
								result = op1 - op2;
								break;
							case MULT:
								result = op1 * op2;
								break;
							case DIV:
								result = op1 / op2;
								break;
							case 0:
								result = op1;
								break;
							default:
								print_debug("Error: invalid value for assign_stmt->op (%d).\n", pc->assign_stmt->op);
								exit(1);
								break;
						}
						ss << result;
					}
					index = pc->assign_stmt->lhs->index ? atoi(pc->assign_stmt->lhs->index->Get(0).c_str()) : 0;
					variablesPtr->SetVar(pc->assign_stmt->lhs->var->name, ss.str(), typeToUse, index);
					//pc->assign_stmt->lhs->var->Set(ss.str(), index);
					pc = pc->next;
					break;
				}
			case IFSTMT:
				if (pc->if_stmt == NULL)
				{
					print_debug("Error: pc points to an if statement but pc->if_stmt is null.\n");
					exit(1);
				}
				if (pc->if_stmt->true_branch == NULL)
				{
					print_debug("Error: if_stmt->true_branch is null.\n");
					exit(1);
				}
				if (pc->if_stmt->false_branch == NULL)
				{
					print_debug("Error: if_stmt->false_branch is null.\n");
					exit(1);
				}
				if (pc->if_stmt->op1 == NULL)
				{
					print_debug("Error: if_stmt->op1 is null.\n");
					exit(1);
				}
				if (pc->if_stmt->op2 == NULL)
				{
					print_debug("Error: if_stmt->op2 is null.\n");
					exit(1);
				}
				// Get the index of the array.
				index1 = pc->if_stmt->op1->index ? atoi(pc->if_stmt->op1->index->Get(0).c_str()) : 0;
				index2 = pc->if_stmt->op2->index ? atoi(pc->if_stmt->op2->index->Get(0).c_str()) : 0;
				op1 = atoi(pc->if_stmt->op1->var->Get(index1).c_str());
				if(pc->if_stmt->op2 != 0)
					op2 = atoi(pc->if_stmt->op2->var->Get(index2).c_str());
				switch (pc->if_stmt->relop)
				{
					case GREATER:
						if (op1 > op2)
							pc = pc->if_stmt->true_branch;
						else
							pc = pc->if_stmt->false_branch;
						break;
					case LESS:
						if (op1 < op2)
							pc = pc->if_stmt->true_branch;
						else
							pc = pc->if_stmt->false_branch;
						break;
					case NOTEQUAL:
						if (op1 != op2)
							pc = pc->if_stmt->true_branch;
						else
							pc = pc->if_stmt->false_branch;
						break;
					case GTEQ:
						if (op1 >= op2)
							pc = pc->if_stmt->true_branch;
						else
							pc = pc->if_stmt->false_branch;
						break;
					case LTEQ:
						if (op1 <= op2)
							pc = pc->if_stmt->true_branch;
						else
							pc = pc->if_stmt->false_branch;
						break;
					case EQUAL:
						if (op1 == op2)
							pc = pc->if_stmt->true_branch;
						else
							pc = pc->if_stmt->false_branch;
						break;
					default:
						print_debug("Error: invalid value for if_stmt->relop (%d).\n", pc->if_stmt->relop);
						exit(1);
						break;
				}
				break;

			case GOTOSTMT:
				if (pc->goto_stmt == NULL)
				{
					print_debug("Error: pc points to a goto statement but pc->goto_stmt is null.\n");
					exit(1);
				}
				if (pc->goto_stmt->target == NULL)
				{
					print_debug("Error: goto_stmt->target is null.\n");
					exit(1);
				}
				pc = pc->goto_stmt->target;
				break;

			default:
				print_debug("Error: invalid value for stmt_type (%d).\n", pc->stmt_type);
				exit(1);
				break;
		}
	}
}

//---------------------------------------------------------
// Entry point

/*
int main()
{
	struct statementNode * program = parse_program_and_generate_intermediate_representation();
	execute_program(program);
	getchar();
	return 0;
}*/

