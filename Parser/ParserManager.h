////////////////////////////////////////////////////////////////////////////////
// Filename: ParserManager.h
////////////////////////////////////////////////////////////////////////////////
#ifndef _PARSERMANAGER_H_
#define _PARSERMANAGER_H_

#include "CompleteParser.h"

struct VarAccessOut
{
	//vector<string> value;
	int typeID;
	string name;
	
};

extern "C"
{
	#define BUFFER_SIZE	2000

	class ParserManager
	{
	public:
		__declspec(dllexport) ParserManager();
		__declspec(dllexport) ~ParserManager();

		__declspec(dllexport) bool Initialize(char* filePath);

		__declspec(dllexport) void Run();

		__declspec(dllexport) void Shutdown();

		__declspec(dllexport) Input* GetInput();
		__declspec(dllexport) CompleteParser* GetParser();

	private:
		Input*	m_input;
		CompleteParser* m_parser;
	};

	// Wrapper point for C# or other languages.
__declspec(dllexport) ParserManager* CreateParserManager();
__declspec(dllexport) void DeleteParserManager(ParserManager* manager);
__declspec(dllexport) bool InitializeParser(ParserManager* manager, char* filePath);
__declspec(dllexport) void ParseSyntax(ParserManager* manager, char* syntax);
__declspec(dllexport) void RunParser(ParserManager* manager);
__declspec(dllexport) bool CompileAndExecuteProgram(ParserManager* manager);
__declspec(dllexport) Variables* GetVariables(ParserManager* manager);
__declspec(dllexport) VarAccessOut* GetOrCreateVarAccess(ParserManager* manager, char* varName);
}
#endif