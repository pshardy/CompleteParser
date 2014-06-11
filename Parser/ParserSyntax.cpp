#include "CompleteParser.h"

int CompleteParser::GetTokenID(string& token)
{
	int tokenID = m_variables->IsTokenInternalType(token);
	switch(tokenID)
	{
	case -1:
		{
			int num = m_variables->IsDigit(token);

			if(num) tokenID = num;
			else tokenID = ID;
			break;
		}
	}

	return tokenID;
}

int CompleteParser::HandleTokenIDSyntax(string& token)
{
	int returnCode = TOKEN_ERR_NONE;

	switch(m_grammarStage)
	{
	case PROGRAM_INPUT:
		{
			m_currentLine.push_back(token);

			// Determine the id of the token.
			int tokenID = GetTokenID(token);

			// Unknown, check against constant types.
			switch(tokenID)
			{
			case SEMICOLON:
				{
					returnCode = EvaluateLine(m_currentLine);
					m_currentLine.clear();
					break;
				}
			case RBRACE:
				{
					returnCode = EvaluateLine(m_currentLine);
					m_currentLine.clear();
					break;
				}
			}

			break;
		}

	}
	return returnCode;
}

// Currently only checks for open brackets.
void CompleteParser::VerifyNodes(Node& node)
{
	if(node.type == TOKENS[LBRAC])
	{
		m_openBrackets++;
	}
	else if(node.type == TOKENS[RBRAC])
	{
		m_openBrackets--;
	}

	for(int i = 0; i < node.nodes.size(); i++)
	{
		VerifyNodes(node.nodes[i]);
	}
}

int CompleteParser::EvaluateLine(list<string>& line)
{
find_open_node:
	bool found = false;

	// The possible rules the first token of this line may match.
	list<string> possibleRules;
	int tokenID = GetTokenID(*line.begin());

	// Find the deepest node that has not been closed and has not been completed.
	Node* openNode = GetFarthestOpenNode(m_nodes);
	// Find the deepest node within the farthest open node which is unknown if it has been completed.
	Node* expandableNode = 0;
	if(openNode) expandableNode = GetFarthestExpandableNode(*openNode);
	if(expandableNode) openNode = expandableNode;

	Node &baseNode = openNode ? *openNode : m_nodes;

	// If we are adding to an existing node this is the start index of the list in the specific ruleset.
	int sIndex = 0;

	// When working with an unknown rule, find the possible start sets which create the rule.
	if(baseNode.type == BASE_NODE_TYPE)
	{
		found = true;
	}

	possibleRules.push_back(baseNode.type);
	sIndex = baseNode.nodes.size();

	list<string> lineCpy = list<string>(line);
	for(list<string>::iterator it = possibleRules.begin(); it != possibleRules.end(); it++)
	{
		if(MatchLineToRule(line, lineCpy, *it, baseNode, sIndex))
		{
			CloseNodeChildren(m_nodes);

			// A match may have been found but the line could still have more to process.
			if(line.empty())
				found = true;
			break;
		}
		else
		{
			// Reset the line for the next possible rule.
			line = lineCpy;
		}
	}

	// The base node is not correct. It is either already complete or a syntax error has occurred. Mark it as complete
	// and re-check all nodes.
	if(!found)
	{
		baseNode.complete = NODE_IS_COMPLETE;
		goto find_open_node;
	}

	return TOKEN_ERR_NONE;
}

bool CompleteParser::MatchLineToRule(list<string>& line, list<string> lineCpy, string& nonToken, Node& node, int sIndex)
{
	if(line.empty() || !IsTokenNonTerminal(nonToken))
		return false;
	// The best node to use out of all rulesets for this non-token.
	Node bestNode;
	list<string> bestLine = list<string>(line);
	// Will be set to false on first non-match through a ruleset iteration.
	bool match = true;
	int highestMatch = 0;
	// Out of all viable rulesets, which one has the highest percentage match?
	float highestPerMatch = 0.0f;
	// Out of all viable rulesets, which one has the best match? (1.0 is not always best. The line may just be incomplete.)
	float bestPerMatch = 0.0f;
	// For each ruleset of this rule.
	for(unsigned int i = 0; i < m_rules[nonToken].size(); i++)
	{
		match = false;
		int currentMatch = 0;
		float matchPercent = 0.0f;
		// The node for this token. It may be disregarded.
		Node newNode;
		newNode.type = nonToken;

		// Reset the line to be executed.
		line = list<string>(lineCpy);

		// For each token in the ruleset.
		list<string>::iterator tokenIt = m_rules[nonToken][i].begin();
		// If epsilon is valid then this rule should be marked as complete.
		//match = (*tokenIt == TOKEN_EPSILON);

		if(sIndex >= m_rules[nonToken][i].size())
			continue;
		advance(tokenIt, sIndex);
		while(tokenIt != m_rules[nonToken][i].end())
		{
			int tokenID = GetTokenID(*line.begin());

			// Compare to terminal requirement.
			if(IsTokenTerminal(*tokenIt))
			{
				// A match to a terminal was found.
				if(GetTokenID(*tokenIt) == tokenID)
				{
					// Automatically add the terminal node to the new node of this token.
					Node newNodeT;
					newNodeT.type = *tokenIt;
					newNodeT.closed = true;
					newNodeT.complete = NODE_IS_COMPLETE;
					newNodeT.value = *line.begin();
					newNodeT.lineNumber = m_inputBuffer->GetLineNumber();
					if(sIndex)
						node.nodes.push_back(newNodeT);
					else
						newNode.nodes.push_back(newNodeT);

					// Move the line forward.
					line.pop_front();
					match = true;
				}
				else // No match, this ruleset is not correct.
				{
					//////////////////////////////////////////////////////////////////////////
					// NOTE: Epsilon does not handle correctly. The grammar currently
					// needs a separate ruleset defined if one of the rules can be ommited.
					//////////////////////////////////////////////////////////////////////////

					// If epsilon is valid then this rule should be marked as complete.
					// The next entry in the series can then be checked.
					match = false;//(*tokenIt == TOKEN_EPSILON);
					break;
				}
			}
			// A non-terminal requires a complete re-evaluation.
			else
			{
				match = MatchLineToRule(line, list<string>(line), *tokenIt, newNode);
				if(!match)
					break;
			}
			tokenIt++;
			currentMatch++;

			// The rule requires more...
			if(tokenIt != m_rules[nonToken][i].end())
			{
				// ...but the line was completely parsed.
				if(line.empty())
					break;
			}
			else // .. The rule has been completed. Update the parent node.
			{
				node.complete = NODE_IS_COMPLETE;
			}
		}

		// The match percent can be between 0.0 and 1.0. If a match was found and
		// the percent is less than 1 then there are missing tokens.
		matchPercent = (float)currentMatch / (float)m_rules[nonToken][i].size();
		// We want to take the combination of entries that matches the longest ruleset.
		if(match && currentMatch >= highestMatch)
		{
			if(matchPercent > highestPerMatch)
			{
				highestPerMatch = matchPercent;
			}

			// If the current match is the same as the last ruleset, but the percent is lower use this.
			// This indicates a longer ruleset which may need to be completed.
			if(currentMatch == highestMatch)
			{
				if(matchPercent < bestPerMatch)
					bestPerMatch = matchPercent;
			}
			else // The rule is definately longer, update the percent match.
			{
				bestPerMatch = matchPercent;
			}

			highestMatch = currentMatch;
			bestNode = newNode;
			bestNode.lineNumber = m_inputBuffer->GetLineNumber();
			bestLine = list<string>(line);
		}
	}

	// Now determine the best match to the given rulesets.
	if(highestMatch > 0 || line.empty())
	{
		line = bestLine;

		// Some rules may appear complete but can be added to later.
		bestNode.complete = (bestPerMatch == 1.0f ? NODE_IS_COMPLETE : highestPerMatch == 1.0f ? NODE_MAYBE_COMPLETE : NODE_NOT_COMPLETE);

		// This makes sure garbage entries aren't added if an expandable node was being checked but no suitable rule was found.
		if(sIndex)
		{
			// Prevent duplicate entries. This node is being expanded upon and the grandchildren should become the children.
			for(unsigned int i = 0; i < bestNode.nodes.size(); i++)
			{
				node.nodes.push_back(bestNode.nodes[i]);
			}
		}
		else
		{
			// Prevent garbage entries.
			if(IsTokenTerminal(bestNode.type) || bestNode.nodes.size() > 0)
				node.nodes.push_back(bestNode);
		}

		return true;
	}

	return match;
}

bool CompleteParser::FindFirstSets(list<string>& possibleRules, int tokenID)
{
	// For each first set.
	for(map<string, list<string> >::iterator it = m_firstSets.begin(); it != m_firstSets.end(); it++)
	{
		// For each token starting the first set.
		for(list<string>::iterator tokenIt = it->second.begin(); tokenIt != it->second.end(); tokenIt++)
		{
			// The token is a match
			if(GetTokenID(*tokenIt) == tokenID)
			{
				possibleRules.push_back(it->first);
				// Break out of second loop but continue first finding all possibilities.
				break;
			}
		}
	}

	return true;
}

Node* CompleteParser::GetFarthestOpenNode(Node& node)
{
	Node* returnNode = (node.complete != NODE_IS_COMPLETE && !node.closed) ? &node : 0;

	for(unsigned int i = 0; i < node.nodes.size(); i++)
	{
		// Only accept a child node if it is not closed.
		Node* childNode = GetFarthestOpenNode(node.nodes[i]);
		if(childNode) returnNode = childNode;
	}

	return returnNode;
}

Node* CompleteParser::GetFarthestExpandableNode(Node& node)
{
	Node* returnNode = (!node.closed && node.complete == NODE_MAYBE_COMPLETE) ? &node : 0;

	for(unsigned int i = 0; i < node.nodes.size(); i++)
	{
		// Only accept a child node if it is not closed.
		Node* childNode = GetFarthestExpandableNode(node.nodes[i]);
		if(childNode) returnNode = childNode;
	}

	return returnNode;
}

Node* CompleteParser::GetLowestRightNode(Node& node)
{
	if(node.nodes.empty())
		return &node;

	return GetLowestRightNode(node.nodes.back());
}

bool CompleteParser::CloseNodeChildren(Node& node)
{
	bool close = false;

	for(unsigned int i = 0; i < node.nodes.size(); i++)
	{
		// Terminals are already closed.
		if(!IsTokenNonTerminal(node.nodes[i].type))
			continue;

		list<string> followTypes;
		// If the token is a non-terminal, find the first set since it would be in the follow set.
		if(i + 1 < node.nodes.size())
		{
			if(IsTokenNonTerminal(node.nodes[i+1].type))
			{
				for(list<string>::iterator it = m_firstSets[node.nodes[i+1].type].begin();
					it != m_firstSets[node.nodes[i+1].type].end(); it++)
				{
					followTypes.push_back(*it);
				}
				//CloseNodeIfChildTerminal(node.nodes[i]);
			}
			else
			{
				followTypes.push_back(node.nodes[i+1].type);
			}
			for(list<string>::iterator it = followTypes.begin();
							it != followTypes.end(); it++)
			{
				if(IsTokenFollow(node.nodes[i].type, *it))
				{
					// Close the node and all child nodes.
					ForceCloseNode(node.nodes[i]);
					close = true;
					break;
				}
			}
		}
		// Check children to see if they should close.
		CloseNodeChildren(node.nodes[i]);
	}

	return close;
}

bool CompleteParser::CloseNodeIfChildTerminal(Node& node)
{
	if(!(m_variables->IsTokenInternalType(node.type) && node.nodes.size() <= 1))
		return false;

	for(unsigned int i = 0; i < node.nodes.size(); i++)
	{
		if(!CloseNodeIfChildTerminal(node.nodes[i]))
			return false;
	}

	node.closed = true;

	return true;
}

void CompleteParser::ForceCloseNode(Node& node)
{
	node.closed = true;
	for(unsigned int i = 0; i < node.nodes.size(); i++)
	{
		ForceCloseNode(node.nodes[i]);
	}
}

bool CompleteParser::IsTokenFollow(string& nonTerminal, string& token)
{
	if(!IsTokenNonTerminal(nonTerminal))
		return false;

	for(list<string>::iterator tokenIt = m_followSets[nonTerminal].begin(); tokenIt != m_followSets[nonTerminal].end(); tokenIt++)
	{
		if(*tokenIt == token)
		{
			return true;
		}
	}

	return false;
}