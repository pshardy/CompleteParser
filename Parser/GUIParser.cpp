#include "GUICompleteParser.h"
#include "Global.h"

void GUICompleteParser::GUICompleteParser::InitializeUserComponent(ParserManager* system)
{
	m_programFinished = false;
	m_systemPtr = system;
	this->runComboBox->SelectedItem = this->runComboBox->Items[0];
	this->scopingComboBox->SelectedItem = this->scopingComboBox->Items[0];
}

void GUICompleteParser::GUICompleteParser::CompileProgram()
{
	SendInputToCompleteParser();
	statementNode* program = m_systemPtr->m_parser->Compile();
	execute_program(program);
	m_systemPtr->m_parser->ShutdownProgram(program);
}

void GUICompleteParser::GUICompleteParser::SendInputToCompleteParser()
{
	m_systemPtr->m_parser->GetVariables()->Clear();
	m_systemPtr->m_parser->ClearNodes();
	this->programOutputTextBox->Text = "";

	if(this->runComboBox->SelectedItem->ToString() == RUN_OPTIONS_CARET)
		m_systemPtr->m_input->SetBuffer(DotNetToNative(this->programInputTextBox->Text->Substring(0, this->programInputTextBox->SelectionStart)));
	else
		m_systemPtr->m_input->SetBuffer(DotNetToNative(this->programInputTextBox->Text));
	m_systemPtr->m_parser->Update();

	// Set the scoping prior to running the program.
	if(this->scopingComboBox->SelectedItem == SCOPE_OPTIONS_STATIC)
		m_systemPtr->m_parser->SetScoping(SCOPING_STATIC);
	else if(this->scopingComboBox->SelectedItem == SCOPE_OPTIONS_OFF)
		m_systemPtr->m_parser->SetScoping(SCOPING_OFF);

	m_programFinished = false;
	//m_systemPtr->m_parser->RunProgram();
}

void GUICompleteParser::GUICompleteParser::UpdateOutput()
{
	this->programOutputTextBox->Text += NativeToDotNet(m_systemPtr->m_parser->GetTextOutput());
}

void GUICompleteParser::GUICompleteParser::UpdateVariables()
{
	Variables* variablesPtr = m_systemPtr->m_parser->GetVariables();
	map<int, Variable>& variables = *variablesPtr->GetVariables();
	
	this->variablesGridView->Rows->Clear();

	if(variables.empty()) return;
	
	this->variablesGridView->Rows->Add(variables.size());

	int row = 0;
	for(map<int, Variable>::iterator it = variables.begin(); it != variables.end(); it++)
	{
		this->variablesGridView->Rows[row]->Cells[0]->Value = NativeToDotNet(it->second.name);
		this->variablesGridView->Rows[row]->Cells[1]->Value = NativeToDotNet(variablesPtr->GetTypeString(it->second.typeID));
		this->variablesGridView->Rows[row]->Cells[2]->Value = NativeToDotNet(it->second.value[0]);
		row++;
	}
}

void GUICompleteParser::GUICompleteParser::MemberIdle(System::Object^ sender, System::EventArgs^ e)
{
	if(this->m_systemPtr->m_parser->IsLooping())
	{
		this->m_systemPtr->m_parser->EvaluateOpenNodes();
	}

	if(!m_programFinished && this->m_systemPtr->m_parser->DoneRunning())
	{
		UpdateVariables();
		UpdateOutput();
		m_programFinished = true;
	}

	// This calls a paint event which in-turn calls the idle event. Otherwise the MemberIdle function only runs
	// when it detects changes in input.
	this->Invalidate();
}

int GUICompleteParser::GUICompleteParser::FindOpenBrackets(int position)
{
	int open = 0;
	for(int i = 0; i < position; i++)
	{
		if(this->programInputTextBox->Text[i] == '{')
			open++;
		else if(this->programInputTextBox->Text[i] == '}')
			open--;
	}

	return open;
}

bool GUICompleteParser::GUICompleteParser::WordAloneOnLine(System::String^ word, int startPosition, int& distanceToLeft)
{
	distanceToLeft = 0;
	// Check to the left.
	for(int i = startPosition - word->Length; i >= 0; i--)
	{
		char c = this->programInputTextBox->Text[i];
		// Newline reached.
		if(this->programInputTextBox->Text[i] == '\r' || this->programInputTextBox->Text[i] == '\n')
			break;
		if(!isspace(this->programInputTextBox->Text[i])) return false;
		distanceToLeft++;
	}
	// Check to the right.
	for(int i = startPosition + word->Length; i < this->programInputTextBox->Text->Length; i++)
	{
		char c=  this->programInputTextBox->Text[i];
		// Newline reached.
		if(this->programInputTextBox->Text[i] == '\r' || this->programInputTextBox->Text[i] == '\n')
			break;
		if(!isspace(this->programInputTextBox->Text[i])) return false;
	}

	return true;
}

void GUICompleteParser::GUICompleteParser::BracketSpacing()
{
	String^ tabDistance = "   ";
	int caret = this->programInputTextBox->SelectionStart;
	int lastChar = caret - 2;
	if(lastChar >= 0)
	{
		int openBrackets = FindOpenBrackets(caret);
		if(openBrackets < 0) return;
		String^ insertStr = "";

		// Check if a bracket should be moved backward.
		if(this->programInputTextBox->Text[lastChar] == '}')
		{
			int dist = 0;
			if(WordAloneOnLine(this->programInputTextBox->Text[lastChar].ToString(), lastChar, dist))
			{
				// Remove everything on the line up to this point including '}'.
				int lenght = this->programInputTextBox->Text->Length;
				this->programInputTextBox->Text = this->programInputTextBox->Text->Remove(lastChar - dist - 1, dist + 2);
				// Insert this line back.
				for(int i = 0; i < openBrackets; i++)
					insertStr += tabDistance;
				insertStr += "}\n";
				caret = lastChar - dist;
			}
		}

		for(int i = 0; i < openBrackets; i++)
			insertStr += tabDistance;
		this->programInputTextBox->Text = this->programInputTextBox->Text->Insert(caret, insertStr);
		this->programInputTextBox->SelectionStart = caret + insertStr->Length;
	}
}