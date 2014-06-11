//////////////////////////////////////////////////////////////////////////////////////////////////
// Complete CompleteParser
// by Patrick Hardy
//
// Enter the desired grammar into a text file called grammar.txt.
// Syntax will be enforced to match the grammar. Data types
// and variables can be declared explicitly or implicitly.
// Static scoping is available only when running pure interpreted (non-pseudo compiled) code.
//
// There is a GUI designed in .NET which can be enabled under Global.h.
//
// CSE 340 Information:
// Grammar loading is based on Project 2 specs but slightly modified in terms of error handling.
// Type checking is based on Project 3 but more lenient. PRIM_INT and PRIM_REAL can be mixed; PRIM_REAL takes
// priority. However once a type has been identified down to a primitive it cannot be changed.
// PRIM_BOOL is true for anything not equal to 0. Constant STRINGs are not currently implemented but the
// framework has been setup for them. They will most likely take priority over PRIM_REAL.
//
// Project 4 / 4.5:
// The grammar has been changed for 4 and 4.5, but the code itself is mostly the same between them. Since data
// types and variables are setup to match Project 3, the toned down project 4 var-declare is not needed.
// The grammar is still set to match, and the parse-tree initially constructed understands the 
// var-declare section, but since variables can be declared implicitly, and since scoping is not used
// for any 'compiled' program, the var-declare section is completely ignored effectively making all
// variables used for project 4 implicit. Variables can be explicitly declared, along with typedefs,
// when the grammar is set to match Project 3.
//
// !!!Bonus Project Update!!!
// The var declare section will work for explicit declaration of variables and arrays. Standard variables
// still can be implicitly declared but the program does not rely on that like in the project 4 submission.
// Most of the code attempts to work with different types of grammar, but due to time-constraints many project
// 4.5 specs have been hard-coded to match the grammar (a simple search algorithm can mitigate this).
// The only changes to the provided C code was the inclusion of a varAccess struct to manage array access
// to the varNode struct. Also since an external C file isn't being used I changed it to use the C++ compiler
// to make things easier.
/////////////////////////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <stdarg.h>
#include "ParserManager.h"

#if !_CONSOLE
#include "GUICompleteParser.h"
using namespace System;
using namespace System::Windows::Forms;

#endif

struct statementNode* parse_program_and_generate_intermediate_representation()
{
	// --------------Initialize program--------------
	ParserManager* system = new ParserManager;

	if(!system->Initialize())
		return 0;

	// --------------Generate parse tree--------------
	system->Run();

	// --------------Generate intermediate representation--------------
	return system->m_parser->Compile();
}


// Disable for console
#if !_CONSOLE
[STAThread]
#endif
int main()
{
#if !_CONSOLE
    Application::EnableVisualStyles();
    Application::SetCompatibleTextRenderingDefault(false);
#endif

	// --------------Initialize program--------------
	ParserManager* system = new ParserManager;

	// Attach exit function to program exit.
	//atexit(&system->Shutdown);

	if(!system->Initialize())
		return 0;

	variablesPtr = system->m_parser->GetVariables();

#if !_CONSOLE
	// Start the managed .NET Windows Forms.
	GUICompleteParser::GUICompleteParser ^gui	= gcnew GUICompleteParser::GUICompleteParser();

	// Link system object to gui.
	gui->InitializeUserComponent(system);

	// Add an idle state to the gui so it can update per frame.
	Application::Idle += gcnew EventHandler(gui, &GUICompleteParser::GUICompleteParser::MemberIdle);

	// Launch the gui.
	Application::Run(gui);
#else
	// --------------Generate parse tree--------------
	system->Run();

	// --------------Generate intermediate representation--------------
	statementNode* program = system->m_parser->Compile();

	// --------------Execute the program using project compiler.c(pp) file--------------
	execute_program(program);

	// --------------Free all memory.--------------
	system->m_parser->ShutdownProgram(program);

	getchar();
#endif
	
	// --------------Shutdown program--------------
	system->Shutdown();
	system = 0;

	return 0;
}