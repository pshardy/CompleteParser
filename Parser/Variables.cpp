#include "Variables.h"

Variables::Variables()
{
	m_internalType		= 0;
	m_internalVariable	= 0;
	m_tempVariableCount	= 0;
	m_constantCount		= 0;
}

Variables::~Variables()
{

}

__declspec(dllexport) void Variables::Clear()
{
	m_scopes.clear();
	m_types.clear();
	m_typeDefs.clear();
	m_variables.clear();
	m_tempVariableCount = 0;
	m_internalType = m_primitives.size();
	m_types = m_primitives;
	m_internalVariable = 0;
	m_constantCount = 0;
}

bool Variables::AddPrimitive(string& type)
{
	int existingID = GetTypeIDNumber(type);

	if(TypeIsPrimitive(existingID))
		return false;

	m_primitives[m_internalType] = type;

	AddType(type);

	return true;
}

bool Variables::CheckTypes(string& type1, string& type2)
{
	return CheckTypes(GetTypeIDNumber(type1), GetTypeIDNumber(type2));
}

bool Variables::CheckTypes(int id1, int id2)
{
	if(id1 == TYPE_UNKNOWN || id2 == TYPE_UNKNOWN)
		return true;

	int lowestID1 = GetLowestType(id1);
	int lowestID2 = GetLowestType(id2);

	// One type is a primitive and the other is not. This can be true.
	if(TypeIsPrimitive(lowestID1) && !TypeIsPrimitive(lowestID2) || TypeIsPrimitive(lowestID2) && !TypeIsPrimitive(lowestID1))
		return true;

	// Both base types must be equal for a comparison to be true.
	return lowestID1 == lowestID2;
}

void Variables::VerifyDataTypes()
{
	for(map<int, Variable>::iterator it = m_variables.begin(); it != m_variables.end(); it++)
	{
		for(int i = 0; i < it->second.value.size(); i++)
			ConvertValue(it->second.value[i], GetLowestType(it->second.typeID));
	}
}

bool Variables::ConvertValue(string& value, int typeID)
{
	if(!TypeIsDeclared(typeID))
		return false;

	string& type = m_types[typeID];
	std::size_t index = value.find('.');
	if(type == "PRIM_INT")
	{
		if(index != std::string::npos)
		{
			value.erase(index, value.length() - index);
			return true;
		}
	}
	else if(type == "PRIM_REAL")
	{
		if(index == std::string::npos)
		{
			value.append(".0");
			return true;
		}
		// There was no number following the decimal.
		else if(index == value.size() - 1)
		{
			value.append("0");
		}
	}

	return false;
}

__declspec(dllexport) bool Variables::SetVar(string& varName, string& value, int typeID, int index)
{
	// Will add the variable if it is not yet declared.
	AddVariable(varName, typeID);

	// The ID of the existing or new variable.
	int varID = GetVarIDNumber(varName);

	// TODO: Type checking is disabled... but PRIM_INT/PRIM_REAL will be converted.
	//if(CheckTypes(m_variables[varID].typeID, typeID))

	int lowestID1 = GetLowestType(m_variables[varID].typeID);
	int lowestID2 = GetLowestType(typeID);
	// The type was previously unknown. Set it to the known value.
	if(!TypeIsPrimitive(lowestID1) && lowestID1 != lowestID2 && !TypeIsTypeDef(lowestID1))
	{
		m_typeDefs[lowestID1] = lowestID2;
	}

	// Cast most compatible types.
	ConvertValue(value, GetLowestType(m_variables[varID].typeID));

	if(GetTypeIDNumber(value) == TYPE_UNKNOWN)
	{
		// Resize the array if necessary.
		while(m_variables[varID].value.size() <= index)
			m_variables[varID].value.push_back("0");

		m_variables[varID].value[index] = value;
		return true;
	}

	return false;
}

Variable* Variables::GetVariable(string& varName)
{
	return GetVariable(GetVarIDNumber(varName));
}

Variable* Variables::GetVariable(int varID)
{
	if(!VarIsDeclared(varID))
		return 0;

	return &m_variables[varID];
}

__declspec(dllexport) varAccess* Variables::GetOrCreateVarAccess(string& token)
{
	if(IsTokenInternalType(token) != -1)
		return 0;

	Variable* var;
	if(token[0] != TOKENS[QUOTE][0] && !IsDigit(token))
	{
		AddVariable(token, TYPE_UNKNOWN);
		var = GetVariable(token);
	}
	else // Constant value.
	{
		var = new Variable();
		var->name = "CONSTANT_";
		stringstream ss;
		ss << m_constantCount++;
		var->name.append(ss.str());
		string type;
		switch(IsDigit(token))
		{
		case PRIM_INT:
			type = "PRIM_INT";
			break;
		case PRIM_REAL:
			type = "PRIM_REAL";
			break;
		default:
			type = "PRIM_STRING";
			break;
		}
		var->typeID = GetTypeIDNumber(type);
		var->value.push_back(token);
		if(type == "PRIM_STRING")
		{
			var->value.back().pop_back();
			var->value.back().erase(var->value.back().begin());
		}
	}

	varAccess* access = new varAccess;
	access->index = 0;
	access->var = var;

	return access;
}


int Variables::IsTokenInternalType(string& token)
{
	for(int i = 0; i < RESERVED_COUNT; i++)
	{
		if(strcmp(token.c_str(), TOKENS[i]) == 0)
			return i;
	}

	return -1;
}

int Variables::IsDigit(string& token)
{
	bool numberFound = false;

	if(token.empty())
		goto end;

	if(isdigit(token[0]) || token[0] == '-')
	{
		// PRIM_REAL
		for(unsigned int c = 0; c < token.length(); c++)
		{
			if(token[c] == '.')
				return PRIM_REAL;
			else if(isdigit(token[c]))
				numberFound = true;
		}

		// PRIM_INT
		if(numberFound)
			return PRIM_INT;
	}

end:

	return 0;
}

map<int, Variable>* Variables::GetVariables()
{
	return &m_variables;
}

void Variables::InitializeVariable(Variable& variable)
{
	int lowestID = GetLowestType(variable.typeID);

	// Universal option.
	for(int i = 0; i < variable.value.size(); i++)
		variable.value[i] = "0";

	if(TypeIsPrimitive(lowestID))
	{
		// Numbers get initialized to 0.
		if(m_types[lowestID] == "PRIM_INT")
		{
			for(int i = 0; i < variable.value.size(); i++)
				variable.value[i] = "0";
		}
		else if(m_types[lowestID] == "PRIM_REAL")
		{
			for(int i = 0; i < variable.value.size(); i++)
				variable.value[i] = "0.0";
		}
	}
}

void Variables::AddScope()
{
	Scope scope;
	m_scopes.push_back(scope);
}

void Variables::RemoveScope()
{
	if(m_scopes.size() > 0)
	{
		// Remove variables declared in this scope.
		for(list<int>::iterator it = m_scopes.back().variables.begin(); it != m_scopes.back().variables.end(); it++)
		{
			m_variables.erase(*it);
		}

		// Remove types declared in this scope.
		for(list<int>::iterator it = m_scopes.back().types.begin(); it != m_scopes.back().types.end(); it++)
		{
			m_types.erase(*it);

			// Remove typedefs that map to a type declared in this scope.
			for(map<int, int>::iterator tDefs = m_typeDefs.begin(); tDefs != m_typeDefs.end(); tDefs++)
			{
				if(tDefs->second == *it)
					m_typeDefs.erase(tDefs->first);
			}

			// Remove typedefs declared in thsi scope.
			m_typeDefs.erase(*it);
		}

		m_scopes.pop_back();
	}
}

void Variables::AddVariableToScope(int varID)
{
	if(m_scopes.empty()) return;
	if(find(m_scopes.back().variables.begin(), m_scopes.back().variables.end(), varID) == m_scopes.back().variables.end())
		m_scopes.back().variables.push_back(varID);
}

void Variables::AddTypeToScope(int typeID)
{
	if(m_scopes.empty()) return;
	if(find(m_scopes.back().types.begin(), m_scopes.back().types.end(), typeID) == m_scopes.back().types.end())
		m_scopes.back().variables.push_back(typeID);
}

__declspec(dllexport) bool Variables::AddVariable(string& varName, int typeID, int size)
{
	int varID = GetVarIDNumber(varName);

	int typeIDCpy = typeID;

	// Variable already declared.
	if(VarIsDeclared(varID))
		return false;

	// Variable is really a type.
	if(TypeIsDeclared(GetTypeIDNumber(varName)))
		return false;

	// The type is not yet declared. Add an internal type.
	if(typeID == TYPE_UNKNOWN)
	{
		string typeStr = "";
		if(!AddType(typeStr, true))
			return false;

		// Get the type again since it has now been added.
		typeIDCpy = m_internalType - 1;
	}
	
	// Add the variable.
	Variable variable;
	variable.name	= varName;
	variable.typeID = typeIDCpy;
	while(variable.value.size() < size)
		variable.value.push_back("0");
	InitializeVariable(variable);

	AddVariableToScope(m_internalVariable);

	m_variables[m_internalVariable++] = variable;

	return true;
}

__declspec(dllexport) bool Variables::AddVariable(string& varName, string& type, int index)
{
	int varID = GetVarIDNumber(varName);

	// Variable already declared.
	if(VarIsDeclared(varID))
		return false;

	int typeID = GetTypeIDNumber(type);

	// The type does not exist. Check if the type is actually a variable so the type can then be extracted.
	if(typeID == TYPE_UNKNOWN)
	{
		typeID = GetVarIDNumber(type);

		// The type is not yet declared. Add an internal type.
		if(typeID == TYPE_UNKNOWN)
		{
			if(!AddType(type))
				return false;

			// Get the type again since it has now been added.
			typeID = GetTypeIDNumber(type);
		}
		// The type passed is really a variable. Retrieve the type of the variable.
		else
		{
			typeID = m_variables[typeID].typeID;
		}
	}

	// Add the variable.
	Variable variable;
	variable.name	= varName;
	variable.typeID = typeID;
	while(variable.value.size() < index)
		variable.value.push_back("0");
	InitializeVariable(variable);

	AddVariableToScope(m_internalVariable);

	m_variables[m_internalVariable++] = variable;

	return true;
}

__declspec(dllexport) struct varAccess* Variables::GetVarAccess(string& name)
{
	Variable* node = GetVariable(GetVarIDNumber(name));
	if(node)
	{
		varAccess* access = new varAccess;
		access->index = 0;
		access->var = node;
		return access;
	}

	return 0;
}

struct Variable* Variables::AddTempVariable()
{
	// Use '#' to prevent user from adding variable name.
	string tempName("temp#");
	stringstream ss;
	ss << tempName << m_tempVariableCount++;
	string finalStr = ss.str();
	AddVariable(finalStr, TYPE_UNKNOWN);
	return GetVariable(GetVarIDNumber(finalStr));
}

bool Variables::AddType(string& typeDef, string& type)
{
	// The types are compatible.
	if(!CheckTypes(typeDef, type))
	{
		return false;
	}

	int typeDefID = GetTypeIDNumber(typeDef);

	// The type def is already declared.
	if(TypeIsTypeDef(typeDefID))
	{
		return false;
	}

	if(!TypeIsDeclared(typeDefID))
	{
		AddType(typeDef);
		typeDefID = GetTypeIDNumber(typeDef);
	}

	int typeID = GetTypeIDNumber(type);
	if(!TypeIsDeclared(typeID))
	{
		AddType(type);
		typeID = GetTypeIDNumber(type);
	}

	AddTypeToScope(typeDefID);

	m_typeDefs[typeDefID] = typeID;

	return true;
}

bool Variables::AddType(string& type, bool internalOnly)
{
	int existingID = GetTypeIDNumber(type);

	if(TypeIsDeclared(existingID))
		return false;

	// A variable is being declared with an unknown type.
	if(internalOnly)
	{
		stringstream ss;
		ss << "INTERNAL_" << m_internalType;
		m_types[m_internalType++] = ss.str();
	}
	else
	{
		m_types[m_internalType++] = type;
	}

	return true;
}

int Variables::GetVarIDNumber(string& token)
{
	// Search all types.
	map<int, Variable>::iterator it = m_variables.begin();

	while(it != m_variables.end())
	{
		if(it->second.name.compare(token) == 0)
			return it->first;
		it++;
	}

	return TYPE_UNKNOWN;
}

__declspec(dllexport) int Variables::GetTypeIDNumber(string& token)
{
	// Search all types.
	map<int, string>::iterator it = m_types.begin();

	while(it != m_types.end())
	{
		if(it->second.compare(token) == 0)
			return it->first;
		it++;
	}

	// Search primitives.
	it = m_primitives.begin();

	while(it != m_primitives.end())
	{
		if(it->second.compare(token) == 0)
			return it->first;

		it++;
	}

	return TYPE_UNKNOWN;
}

__declspec(dllexport) string Variables::GetTypeString(int typeID)
{
	if(!TypeIsDeclared(typeID))
		return "";

	return m_types[typeID];
}

__declspec(dllexport) int Variables::GetLowestType(int typeID)
{
	if(!TypeIsTypeDef(typeID))
		return typeID;

	return GetLowestType(m_typeDefs[typeID]);
}

bool Variables::TypeIsTypeDef(int typeID)
{
	return (m_typeDefs.find(typeID) != m_typeDefs.end());
}

bool Variables::VarIsDeclared(int varID)
{
	return (m_variables.find(varID) != m_variables.end());
}

bool Variables::TypeIsDeclared(int typeID)
{
	return (m_types.find(typeID) != m_types.end());
}

bool Variables::TypeIsPrimitive(int typeID)
{
	return (m_primitives.find(typeID) != m_primitives.end());
}

void Variables::PrintVariables()
{
	map<int, Variable>::iterator it = m_variables.begin();

	cout << "VARIABLES\n";
	while(it != m_variables.end())
	{
		cout << "ID: " << it->first << " VAR: " << it->second.name.c_str() << " Value: " << it->second.value[0].c_str() << " TYPE: " << it->second.typeID << "\n";
		it++;
	}
}

void Variables::PrintTypes(stringstream& out)
{
	stringstream ss;

	map<int, string>::iterator it = m_types.begin();

	ss << "TYPES\n";
	while(it != m_types.end())
	{
		ss << "ID: " << it->first << " TYPE: " << it->second.c_str() << "\n";
		it++;
	}

	map<int, int>::iterator it2 = m_typeDefs.begin();

	ss << "TYPEDEFS\n";
	while(it2 != m_typeDefs.end())
	{
		ss << "ID: " << it2->first << " DEF: " << it2->second << "\n";
		it2++;
	}

	out << ss.str();
}