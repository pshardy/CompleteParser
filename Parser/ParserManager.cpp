#include "ParserManager.h"
#include <stdexcept>

__declspec(dllexport) ParserManager* CreateParserManager()
{
	return new ParserManager();
}

__declspec(dllexport) void DeleteParserManager(ParserManager* manager)
{
	if(manager != NULL)
	{
		manager->Shutdown();
		delete manager;
		manager = 0;
	}
}

__declspec(dllexport) bool InitializeParser(ParserManager* manager, char* filePath)
{
	if(manager != NULL)
	{
		return manager->Initialize(filePath);
	}

	return false;
}

__declspec(dllexport) void ParseSyntax(ParserManager* manager, char* syntax)
{
	if(manager != NULL)
	{
		manager->GetParser()->GetVariables()->Clear();
		manager->GetParser()->ClearNodes();
		manager->GetInput()->SetBuffer(string(syntax));
		manager->GetParser()->Update();
	}
}

__declspec(dllexport) void RunParser(ParserManager* manager)
{
	if(manager != NULL)
	{
		manager->Run();
	}
}

__declspec(dllexport) bool CompileAndExecuteProgram(ParserManager* manager)
{
	statementNode* program = NULL;
	if(manager != NULL)
	{
		program = manager->GetParser()->Compile();
		if(program != NULL)
		{
			execute_program(program);
			return true;
		}
	}

	return false;
}

__declspec(dllexport) Variables* GetVariables(ParserManager* manager)
{
	if(manager != NULL)
	{
		return manager->GetParser()->GetVariables();
	}

	return NULL;
}

__declspec(dllexport) VarAccessOut* GetOrCreateVarAccess(ParserManager* manager, char* varName)
{
	VarAccessOut* out = NULL;

	Variables* variables = manager->GetParser()->GetVariables();

	if(variables != NULL)
	{
		varAccess* var = variables->GetOrCreateVarAccess(string(varName));
		if(var)
		{
			cout << "\nVAR NAME: " << var->var->name;
			out = new VarAccessOut;
			out->name = var->var->name;
			//out->value = var->var->value;
			out->typeID = var->var->typeID;
		}
	}
	return out;
}

__declspec(dllexport) ParserManager::ParserManager()
{
	m_input		= 0;
	m_parser	= 0;
}

__declspec(dllexport) ParserManager::~ParserManager()
{

}

__declspec(dllexport) bool ParserManager::Initialize(char* filePath)
{
	m_input = new Input;

	if(!m_input->Initialize(BUFFER_SIZE))
	{
		printf("Could not initialize input.");
		return false;
	}

	m_parser = new CompleteParser;

	if(!m_parser->Initialize(m_input))
	{
		printf("Could not initialize parser.");
		return false;
	}

	// Load grammar from file.
	if(!m_input->OpenFile(filePath))
		printf("Failed to open file %s.", filePath);

	// --------------Load Grammar--------------
	while(true)
	{
		m_input->ReadLine();
		if(!m_parser->Update())
			break;
	}

	m_input->CloseFile();

	m_parser->SetGrammarStage(PROGRAM_INPUT);

	return true;
}

__declspec(dllexport) void ParserManager::Shutdown()
{
	if(m_parser)
	{
		m_parser->Shutdown();
		delete m_parser;
		m_parser = 0;
	}

	if(m_input)
	{
		m_input->Shutdown();
		delete m_input;
		m_input = 0;
	}
}

__declspec(dllexport) void ParserManager::Run()
{
	// --------------Accept Program Input--------------
	while(true)
	{
		m_input->ReadLine();
		if(!m_parser->Update())
			break;
	}
}

__declspec(dllexport) Input* ParserManager::GetInput()
{
	return m_input;
}

__declspec(dllexport) CompleteParser* ParserManager::GetParser()
{
	return m_parser;
}