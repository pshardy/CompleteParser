#include "CompleteParser.h"

template <class T> class IsEpsilon : public unary_function<T, bool>
{
public:
	bool operator() (T& val)
	{
		return val == TOKEN_EPSILON;
	}
};

class StackMemCompare
{
public:
    StackMemCompare(void* compare) : compare(compare) {}
    bool operator() (const StackMemory *mem) const {return mem->mem == compare;}
private:
    void* compare;
};

CompleteParser::CompleteParser()
{
	m_inputBuffer				= 0;
	m_tokenStart = m_tokenEnd	= 0;
	m_grammarStage				= GRMR_NONTERMINALS;
	m_startOfRule				= false;
	m_evaluatingLoop			= false;
	m_currentRule				= 0;
	m_currentRuleList			= 0;
	m_variables					= 0;
	m_openBrackets				= 0;
	m_scoping					= SCOPING_STATIC;
	m_consoleMode				= false;

	ClearNodes();
}

CompleteParser::~CompleteParser()
{

}

bool CompleteParser::Initialize(Input* input)
{
	if(!input) return false;

	m_inputBuffer	= input;
	m_variables		= new Variables;
	m_nodes.type	= BASE_NODE_TYPE;

	string boolStr("PRIM_BOOL");
	string stringStr("PRIM_STRING");
	string intStr("PRIM_INT");
	string realStr("PRIM_REAL");
	m_variables->AddPrimitive(boolStr);
	m_variables->AddPrimitive(stringStr);
	m_variables->AddPrimitive(intStr);
	m_variables->AddPrimitive(realStr);

	return true;
}

int CompleteParser::GetOpenBrackets()
{
	return m_openBrackets;
}

int CompleteParser::GetGrammarStage()
{
	return m_grammarStage;
}

void CompleteParser::SetGrammarStage(int stage)
{
	m_grammarStage = stage;
}

__declspec(dllexport) bool CompleteParser::Update()
{
	char c;

	// Loop through the entire line finding all words and matching all tokens.
	while(!m_inputBuffer->IsEOL() && m_grammarStage != GRMR_FINISHED)
	{
		if(m_inputBuffer->GetCurrentChar(c))
		{
			// Error check the token output. If true then processing should be aborted.
			if(HandleError(HandleToken(GetTokenType(c))))
				return false;
		}
	}

	if(m_grammarStage == GRMR_FINISHED)
		return false;

	if(m_inputBuffer->IsEOF() || m_errors.IsCritical() || CompleteProgram())
	{
		EndParsing();
		return false;
	}

	return true;
}

void CompleteParser::EndParsing()
{
	if(m_currentRule || m_grammarStage < GRMR_FINISHED)
	{
		HandleError(TOKEN_ERR_SYNTAX);
		return;
	}
	
	if(!m_errors.IsCritical())
	{
		CalculateFirstAndFollowSets();
		if(!m_errors.HasErrors())
		{
			if(PRINT_RULES)
				PrintRules();
			if(PRINT_OUTPUT)
			{
				PrintFirstSets();
				PrintFollowSets();
			}
		}
	}
}

bool CompleteParser::HandleError(int errorCode)
{
	if(errorCode)
	{
		m_errors.AddError(errorCode);

		if(m_errors.IsCritical())
			return true;
	}

	return false;
}

int CompleteParser::HandleToken(int token)
{
	int returnCode = TOKEN_ERR_NONE;

	switch(token)
	{
		// A syntax error has occurred.
	//default:
//		returnCode = TOKEN_ERR_SYNTAX;
	//	break;
		// Check end of entry.
	case TOKEN_HASH:
		// Check if epsilon is being entered (NULL input)
		if(m_currentRule && m_currentRule->empty())
		{
			string eps(TOKEN_EPSILON); // Declare here for portability.
			HandleTokenID(eps);
		}
		if(m_grammarStage < GRMR_RULES)
			m_grammarStage++;
		if(m_grammarStage == GRMR_RULES)
		{
			if(m_terminals.empty() || m_nonTerminals.empty())
				return TOKEN_ERR_SYNTAX;
			// Load in constant terminals.
			if(!IsTokenTerminal(string(TOKEN_EOF)))
				m_terminals.push_back(TOKEN_EOF);
			if(!IsTokenTerminal(string(TOKEN_EPSILON)))
				m_terminals.push_back(TOKEN_EPSILON);
			m_startOfRule = true;
		}
		m_currentRule = 0;
		break;
		// Check end of file.
	case TOKEN_DOUBLEHASH:
		m_grammarStage = GRMR_FINISHED;
		EndParsing();
		break;
		// Add ID to terminals, non-terminals, or rules.
	default:
		{
			string cpy = m_inputBuffer->CopyBuffer(m_tokenStart, m_tokenEnd, token == PRIM_STRING); // Declare here for portability.
			RemoveCString(cpy);
			returnCode = HandleTokenID(cpy);
			break;
		}
		// End the key definition of a rule.
	case TOKEN_ARROW:
		if(!m_startOfRule)
			HandleError(TOKEN_ERR_SYNTAX);
		m_startOfRule = false;
		break;
	}

	return returnCode;
}

/*
	The map searching here is logarithmic using the std algorithms,
	however it could be further optimized to bookmark where an entry should
	be inserted to during the initial existance search. So when the entry
	is verified as not existing it would then be inserted directly into the
	bookmark.
*/

int CompleteParser::HandleTokenID(string& token)
{
	int returnCode = TOKEN_ERR_NONE;

	if(token.empty())
		return returnCode;

	switch(m_grammarStage)
	{
	default:
		return HandleTokenIDSyntax(token);
	case GRMR_NONTERMINALS:
		// Only add to the list if the token does not already exist.
		if(!IsTokenNonTerminal(token))
			m_nonTerminals.push_back(token);
		break;
	case GRMR_TERMINALS:
		// Only add to the list if the token does not already exist.
		if(!IsTokenTerminal(token))
			m_terminals.push_back(token);
		break;
	case GRMR_RULES:
		// A new rule key is going to be defined.
		if(m_startOfRule)
		{
			// The current rule key has not been closed out by an arrow.
			if(m_currentRule) return TOKEN_ERR_SYNTAX;

			// Check to make sure the non terminal has been declared.
			if(!IsTokenNonTerminal(token) && !IsTokenTerminal(token))
				returnCode = TOKEN_ERR_NOT_DECLARED;

			// Check to make sure a terminal is not being defined.
			if(IsTokenTerminal(token))
				returnCode = TOKEN_ERR_NON_MODIFIABLE;

			map<string, vector< list <string> > >::iterator it;
			// If the non-terminal does not yet exist as a rule key.
			if((it = m_rules.find(token)) == m_rules.end())
			{
				// Create a new map entry and link the current token to it.
				m_rules[token].push_back(list<string>());
				m_currentRule = &m_rules[token][m_currentRuleList = 0];
			}
			// There is already an existing key with an existing set of rules. Add another set of rules.
			else
			{
				it->second.push_back(list<string>());
				m_currentRule = &it->second[(m_currentRuleList = (it->second.size() - 1))];
			}
		}
		// Rules are being added to an existing rule key.
		else
		{
			// Check the token against terminals and non terminals.
			if(!m_currentRule)
				return TOKEN_ERR_SYNTAX;

			if(!IsTokenNonTerminal(token) && !IsTokenTerminal(token))
				returnCode = TOKEN_ERR_NOT_DECLARED;

			// Add the token to the current ruleset.
			m_currentRule->push_back(token);

		}
		break;
	}

	return returnCode;
}

// This is called when a new token is being identified.
int CompleteParser::GetTokenType(char c)
{
	int tType = TOKEN_UNKNOWN;

	m_tokenStart = m_inputBuffer->GetCurrentIndex() - 1;

	switch(GetIDType(c))
	{
		// The first char must be a letter. (letter (letter + digit)*)
	case LETTER:
		{
			char nextC;
			// Continue the rest of the input to gather the ID.
			while(m_inputBuffer->GetCurrentChar(nextC))
			{
				// Only a letter or digit extends this token, and a space completes it.
				if(GetIDType((int)nextC) == OTHER)
				{
					if(nextC != *TOKENS[RBRACE] && nextC != *TOKENS[SEMICOLON] && nextC != *TOKENS[COMMA])
					{
						//HandleError(TOKEN_ERR_SYNTAX);
					}
					m_inputBuffer->DecrementIndex();
					break;
				}
			}
			
			tType = TOKEN_ID;

			break;
		}
	case DIGIT:
		{
handle_digit:
			// A negative operator could be either part of a sign of digit or an operation.
			if(c == '-' && m_currentLine.size())
			{
				string op = m_currentLine.back();
				if(!(op == "=" || op == ":" || OperationToTokenType(op)))
					break;
			}

			tType = PRIM_INT;
			char nextC;
			// Continue the rest of the input to gather the number.
			while(m_inputBuffer->GetCurrentChar(nextC))
			{
				if(nextC == '.')
				{
					tType = PRIM_REAL;
				}
				else
				{
					// Only a digit extends this token.
					if(GetIDType((int)nextC) != DIGIT)
					{
						m_inputBuffer->DecrementIndex();
						break;
					}
				}
			}

			break;
		}
		// If this is not an ID being processed.
	case OTHER:
		{
			if(m_grammarStage == PROGRAM_INPUT && c == TOKENS[QUOTE][0])
			{
				char nextC = ' ';
				// Continue the rest of the input to gather the ID.
				while(nextC != TOKENS[QUOTE][0])
				{
					m_inputBuffer->GetCurrentChar(nextC);
					if(m_inputBuffer->IsEOF())
						break;
				}

				tType = PRIM_STRING;
			}
			else if(c == TOKENS[TOKEN_HASH][0])
			{
				tType = TOKEN_HASH;
				char nextC;
				// Check if the next char completes the double hash.
				if(m_inputBuffer->PeekChar(nextC))
				{
					if(nextC == TOKENS[TOKEN_HASH][0])
					{
						tType = TOKEN_DOUBLEHASH;
						// Check if the next char is white space.
						if(m_inputBuffer->PeekChar(nextC, 1))
						{
							if(!isspace(nextC) && !m_inputBuffer->IsEndOfLine(nextC))
							{
								HandleError(TOKEN_ERR_SYNTAX);
							}
							else // The next char after double hash is white space, advance the current char.
								m_inputBuffer->GetCurrentChar(nextC);
						}
					}
					// The next character does not complete or extend this token.
					else if(!isspace(nextC) && !m_inputBuffer->IsEndOfLine(nextC))
						HandleError(TOKEN_ERR_SYNTAX);
				}
			}
			else if(c == TOKENS[TOKEN_ARROW][0])
			{
				tType = TOKEN_UNKNOWN;
				char nextC;
				if(m_inputBuffer->PeekChar(nextC))
				{
					if(nextC == TOKENS[TOKEN_ARROW][1])
					{
						tType = TOKEN_ARROW;
						if(m_inputBuffer->PeekChar(nextC, 1))
						{
							if(!isspace(nextC) && !m_inputBuffer->IsEndOfLine(nextC))
								HandleError(TOKEN_ERR_SYNTAX);
							else // The next char after the arrow is white space, advance the current char.
								m_inputBuffer->GetCurrentChar(nextC);
						}
					}
					else if(!isspace(nextC) && !m_inputBuffer->IsEndOfLine(nextC))
					{
						// May be a negative number.
						if(isdigit(nextC))
							goto handle_digit;
						else
							HandleError(TOKEN_ERR_SYNTAX);
					}
				}
			}
			
			else if(c == TOKENS[GTEQ][0])
			{
				tType = GREATER;
				char nextC;
				if(m_inputBuffer->PeekChar(nextC))
				{
					if(nextC == TOKENS[GTEQ][1])
					{
						tType = GTEQ;
						if(m_inputBuffer->PeekChar(nextC, 1))
						{
							if(!isspace(nextC) && !m_inputBuffer->IsEndOfLine(nextC))
								HandleError(TOKEN_ERR_SYNTAX);
							else // The next char after the arrow is white space, advance the current char.
								m_inputBuffer->GetCurrentChar(nextC);
						}
					}
					else if(!isspace(nextC) && !m_inputBuffer->IsEndOfLine(nextC))
						HandleError(TOKEN_ERR_SYNTAX);
				}
			}
			else if(c == TOKENS[LTEQ][0])
			{
				tType = LESS;
				char nextC;
				if(m_inputBuffer->PeekChar(nextC))
				{
					if(nextC == TOKENS[LTEQ][1])
					{
						tType = LTEQ;
						if(m_inputBuffer->PeekChar(nextC, 1))
						{
							if(!isspace(nextC) && !m_inputBuffer->IsEndOfLine(nextC))
								HandleError(TOKEN_ERR_SYNTAX);
							else // The next char after the arrow is white space, advance the current char.
								m_inputBuffer->GetCurrentChar(nextC);
						}
					}
					else if(nextC == TOKENS[NOTEQUAL][1])
					{
						tType = NOTEQUAL;
						if(m_inputBuffer->PeekChar(nextC, 1))
						{
							if(!isspace(nextC) && !m_inputBuffer->IsEndOfLine(nextC))
								HandleError(TOKEN_ERR_SYNTAX);
							else // The next char after the arrow is white space, advance the current char.
								m_inputBuffer->GetCurrentChar(nextC);
						}
					}
					else if(!isspace(nextC) && !m_inputBuffer->IsEndOfLine(nextC))
						HandleError(TOKEN_ERR_SYNTAX);
				}
			}
			break;
		}
	}

	m_tokenEnd = m_inputBuffer->GetLastGoodIndex() + 1;

	return tType;
}

bool CompleteParser::IsTokenTerminal(string& token)
{
	return find(m_terminals.begin(), m_terminals.end(), token) != m_terminals.end();
}

bool CompleteParser::IsTokenNonTerminal(string& token)
{
	return find(m_nonTerminals.begin(), m_nonTerminals.end(), token) != m_nonTerminals.end();
}

bool CompleteParser::IsTokenRuleKey(string& token)
{
	return m_rules.find(token) != m_rules.end();
}

void CompleteParser::RemoveCString(string& str)
{
	if(str.length() > 0)
	{
		if(str[str.length() - 1] == '\0')
			str.erase(str.begin() + str.length() - 1);
	}
}

bool CompleteParser::IsTokenInRule(string& token)
{
	// The epsilon and EOF are added in apart from the normal token list and do not have to be in the rules.
	if(token == TOKEN_EOF || token == TOKEN_EPSILON)
		return true;

	for(map<string, vector<list <string> > >::iterator rules = m_rules.begin(); rules != m_rules.end(); rules++)
	{
		for(unsigned int i = 0; i < rules->second.size(); i++)
		{
			for(list<string>::iterator rule = rules->second[i].begin(); rule != rules->second[i].end(); rule++)
			{
				if(*rule == token)
					return true;
			}
		}
	}

	return false;
}

int CompleteParser::GetIDType(int c)
{
	return (isalpha(c) || c == '_') ? LETTER : isdigit(c) ? DIGIT : OTHER;
}

void CompleteParser::CalculateFirstAndFollowSets()
{
	if(m_rules.empty())
		return;

	// Add EOF to start symbols.
	if(IsTokenRuleKey(*m_nonTerminals.begin()))
	{
		for(unsigned int i = 0; i < m_rules[*m_nonTerminals.begin()].size(); i++)
		{
			m_rules[*m_nonTerminals.begin()][i].push_back(TOKEN_EOF);
		}
	}
	else
		HandleError(TOKEN_ERR_NON_MODIFIABLE);

	m_firstSets.clear();
	m_followSets.clear();
	// Determine the first possible operation(s) of each non-terminal.
	for(list<string>::iterator nt = m_nonTerminals.begin(); nt != m_nonTerminals.end(); nt++)
	{
		list<StackMemory*> stack;
		FindStartTerminalsOfRules(*nt, m_firstSets[*nt], stack);
		stack.clear();
		FindFollowTerminalsOfRules(*nt, m_followSets[*nt], stack);
	}

	// Check to make sure every non-terminal is defined.

	for(list<string>::iterator it = m_nonTerminals.begin(); it != m_nonTerminals.end(); it++)
	{
		if(!IsTokenRuleKey(*it))
		{
			HandleError(TOKEN_ERR_NT_NOT_DEFINED);
			break;
		}
	}

	// Now check that the terminals are reachable.
	// TODO: Determine if a new section outside of terminals is needed, such as for primitives
	// and sign operations. For example, the negative (-2) sign is used for numbers, but may
	// not be valid as an expression operator (5 - 2).
	/*
	for(list<string>::iterator it = m_terminals.begin(); it != m_terminals.end(); it++)
	{
		if(!IsTokenInRule(*it))
		{
			HandleError(TOKEN_ERR_NON_REACHABLE);
			break;
		}
	}*/
}

#define TOKEN_IS_EPSILON	-1
#define TOKEN_IS_NON_TERM	0
#define TOKEN_IS_TERMINAL	1
#define TOKEN_IS_EOF		2
#define TOKEN_IS_REPEAT		3

int CompleteParser::FindStartTerminalsOfRules(string& token, list<string>& terminals, list<StackMemory*>& stackOverflow)
{
	// The token has been executed through the rules down to a terminal.
	if(token != TOKEN_EOF && IsTokenTerminal(token))
	{
		if(find(terminals.begin(), terminals.end(), token) == terminals.end())
		{
			terminals.push_back(token);
		}
		return (token == TOKEN_EPSILON) ? TOKEN_IS_EPSILON : TOKEN_IS_TERMINAL;
	}

	int terminalFound = TOKEN_IS_NON_TERM;

	bool epsilonVerified = false;
	int epsilonCount = 0;

	if(IsTokenRuleKey(token))
	{
		// For each rule set of this token. (If a key has multiple definitions)
		for(unsigned int i = 0; i < m_rules[token].size(); i++)
		{
			// For each rule of the duplicate keys.
			int ruleItIndex = 0;
			for(list<string>::iterator ruleIt = m_rules[token][i].begin(); ruleIt != m_rules[token][i].end(); ruleIt++)
			{
				// Recursive check against the rule token.
				list<StackMemory*>::iterator stackIt = find_if(stackOverflow.begin(), stackOverflow.end(), StackMemCompare(&*ruleIt));
				if(stackIt == stackOverflow.end())
				{
					StackMemory stackmem;
					stackmem.mem = &*ruleIt;
					stackOverflow.push_back(&stackmem);
					int result = FindStartTerminalsOfRules(*ruleIt, terminals, stackOverflow);
					stackOverflow.remove(&stackmem);
					if(result != TOKEN_IS_NON_TERM)
					{
						if(result != TOKEN_IS_EPSILON)
						{
							epsilonCount = 0;
							// Epsilon has the highest return value priority.
							if(terminalFound != TOKEN_IS_EPSILON)
								terminalFound = TOKEN_IS_TERMINAL;
							break;
						}
						else
						{
							// The terminal found was epsilon. Add to the count of epsilons for this sequence.
							terminalFound = TOKEN_IS_EPSILON;
							epsilonCount++;
						}
					}
					else
					{
						break;
					}
				}
				else
					break;

				ruleItIndex++;
			}

			// The espilon should only be added if it exists as a distinct single possibility.
			int size = m_rules[token][i].back() == TOKEN_EOF ? m_rules[token][i].size() - 1 : m_rules[token][i].size();
			if(epsilonCount == size)
			{
				epsilonVerified = true;
			}
			if(!epsilonVerified)
			{
				terminals.remove(TOKEN_EPSILON);
			}
		}
	}

	return terminalFound;
}

int CompleteParser::FindFollowTerminalsOfRules(string& token, list<string>& terminals, list<StackMemory*>& stackOverflow)
{
	// The token has been executed through the rules down to a terminal.
	if(IsTokenTerminal(token))
	{
		if(find(terminals.begin(), terminals.end(), token) == terminals.end())
		{
			// Epsilon does not show up on the follow sets.
			if(token != TOKEN_EPSILON)
				terminals.push_back(token);
		}
		return (token == TOKEN_EPSILON) ? TOKEN_IS_EPSILON : TOKEN_IS_TERMINAL;
	}

	int terminalFound = TOKEN_IS_NON_TERM;

	// Loop through every rule key.
	for(map<string, vector<list <string> > >::iterator ruleKey = m_rules.begin(); ruleKey != m_rules.end(); ruleKey++)
	{
		// Loop through every rule set for this rule key.
		for(unsigned int ruleSet = 0; ruleSet < ruleKey->second.size(); ruleSet++)
		{
			unsigned int ruleItIndex = 0;
			// Now check every rule in the set for this token.
			for(list<string>::iterator rule = ruleKey->second[ruleSet].begin(); rule != ruleKey->second[ruleSet].end(); rule++)
			{
				// The token was found as a rule.
				if(*rule == token)
				{
					int result = TOKEN_IS_EPSILON;
					while(result == TOKEN_IS_EPSILON)
					{
						// The rule is not at the end of the rule set.
						if(ruleItIndex < ruleKey->second[ruleSet].size() - 1)
						{
							list<StackMemory*> stack;
							// Determine what comes after the token.
							result = FindStartTerminalsOfRules(*(++rule), terminals, stack);
							// Make sure epsilon is not in the follow set.
							terminals.remove_if(IsEpsilon<string&>());
							ruleItIndex++;
						}
						else
							break;
					}

					terminalFound = result;

					// The rule is at the end of this set.
					if(terminalFound != TOKEN_IS_TERMINAL && ruleItIndex >= ruleKey->second[ruleSet].size() - 1)
					{
						// The rule is at the end of its set, so now look through all of the other
						// rules where this might be called.
						if(ruleKey->first != token)
						{
							// If two different rule sets end with each others non-terminal a stack overflow can occur.
							// This keeps track of sets checked.
							if(find_if(stackOverflow.begin(), stackOverflow.end(), StackMemCompare(&m_rules[ruleKey->first][ruleSet])) == stackOverflow.end())
							{
								StackMemory mem;
								mem.mem = &m_rules[ruleKey->first][ruleSet];
								stackOverflow.push_back(&mem);
								FindFollowTerminalsOfRules((string&)ruleKey->first, terminals, stackOverflow);
								stackOverflow.remove(&mem);
							}
						}
						break;
					}
				}
				ruleItIndex++;
			}
		}
	}

	// Make sure the EOF symbol is loaded for the start symbol.
	if(token == *m_nonTerminals.begin())
	{
		// Add the end of file token.
		AddEOFToSet(terminals);
		terminalFound = TOKEN_IS_EOF;
	}
	
	return terminalFound;
}

void CompleteParser::AddEOFToSet(list<string>& setIn)
{
	// Add the end of file token.
	if(find(setIn.begin(), setIn.end(), TOKEN_EOF) == setIn.end())
	{
		setIn.push_back(TOKEN_EOF);
	}
}

// FIRST(key) = { VAL1, VAL2, ... VALN }
void CompleteParser::PrintFirstSets()
{
	// Find the key by using the non terminals to keep the original input order.
	for(list<string>::iterator it = m_nonTerminals.begin(); it != m_nonTerminals.end(); it++)
	{
		list<string> sortedList = m_firstSets[*it];
		sortedList.sort();

		string output = "";
		output.append("FIRST(").append(*it).append(") = {");

		unsigned int count = 0;
		for(list<string>::iterator terminals = sortedList.begin(); terminals != sortedList.end(); terminals++)
		{
			output.append(" ").append(*terminals);
			// Add comma except on the end.
			if(count < sortedList.size() - 1)
				output.append(",");	// push_back not portable prior to c++11.
			count++;
		}

		// Check to make sure the set exists.
		if(output.size() == 0)
			continue;
			
		output.append(" }\n");
		printf(output.c_str());
	}
}

void CompleteParser::PrintFollowSets()
{
	// Find the key by using the non terminals to keep the original input order.
	for(list<string>::iterator it = m_nonTerminals.begin(); it != m_nonTerminals.end(); it++)
	{
		list<string> sortedList = m_followSets[*it];
		sortedList.sort();

		string output = "";
		output.append("FOLLOW(").append(*it).append(") = {");

		unsigned int count = 0;
		for(list<string>::iterator terminals = sortedList.begin(); terminals != sortedList.end(); terminals++)
		{
			output.append(" ").append(*terminals);
			// Add comma except on the end.
			if(count < (sortedList.size() - 1))
				output.append(",");	// push_back not portable prior to c++11.
			count++;
		}

		// Check to make sure the set exists.
		if(output.size() == 0)
			continue;

		output.append(" }\n");
		printf(output.c_str());
	}
}

void CompleteParser::PrintErrors()
{
	if(!PRINT_OUTPUT) return;
	m_errors.PrintErrors();
}

void CompleteParser::PrintRules()
{
	if(!PRINT_OUTPUT) return;
	for(map<string, vector< list <string> > >::iterator it = m_rules.begin(); it != m_rules.end(); it++)
	{
		printf("key %s", it->first.c_str());
		for(unsigned int i = 0; i < it->second.size(); i++)
		{
			printf("\nruleset %i\n", i);
			for(list<string>::iterator rules = it->second[i].begin(); rules != it->second[i].end(); rules++)
			{
				printf("%s ", rules->c_str());
			}
		}
		printf("\n");
	}
	printf("\n");
}

void CompleteParser::Shutdown()
{
	m_inputBuffer = 0;

	delete m_variables;
	m_variables = 0;
}

bool CompleteParser::IsLooping()
{
	return m_evaluatingLoop;
}

__declspec(dllexport) Variables* CompleteParser::GetVariables()
{
	return m_variables;
}

__declspec(dllexport) Node* CompleteParser::GetNodes()
{
	return &m_nodes;
}

__declspec(dllexport) string CompleteParser::GetTextOutput()
{
	return m_textOutput.str();
}