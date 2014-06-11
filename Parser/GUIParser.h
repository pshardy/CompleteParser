#pragma once

#include "ParserManager.h"

#define RUN_OPTIONS_AUTO		"Auto"
#define RUN_OPTIONS_MANUAL		"Manual"
#define RUN_OPTIONS_CARET		"Caret"

#define SCOPE_OPTIONS_STATIC	"Static"
#define SCOPE_OPTIONS_OFF		"Off"

namespace GUICompleteParser {

	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;

	/// <summary>
	/// Summary for GUICompleteParser
	/// </summary>
	public ref class GUICompleteParser : public System::Windows::Forms::Form
	{
	public:
		GUICompleteParser(void)
		{
			InitializeComponent();
		}

	protected:
		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		~GUICompleteParser()
		{
			if (components)
			{
				delete components;
			}
		}
	public: void InitializeUserComponent(ParserManager* system);
	private: void SendInputToCompleteParser();
	private: void CompileProgram();
	private: void UpdateVariables();
	private: void UpdateOutput();
	private: void BracketSpacing();
	private: int FindOpenBrackets(int position);
	private: bool WordAloneOnLine(System::String^ word, int startPosition, int& distanceToLeft);
	public : void MemberIdle(System::Object^ sender, System::EventArgs^ e);
	private: System::Windows::Forms::RichTextBox^  programInputTextBox;
	protected: 
	private: System::Windows::Forms::DataGridView^  variablesGridView;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^  varNameCol;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^  varTypeCol;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^  varValueCol;

	private: ParserManager* m_systemPtr;
	private: System::Windows::Forms::Label^  programLabel;
	private: System::Windows::Forms::Label^  variablesLabel;
	private: System::Windows::Forms::Label^  runLabel;
	private: System::Windows::Forms::ComboBox^  runComboBox;

	private: System::Windows::Forms::Label^  scopeLabel;
	private: System::Windows::Forms::ComboBox^  scopingComboBox;
	private: System::Windows::Forms::RichTextBox^  programOutputTextBox;
	private: System::Windows::Forms::Label^  outputLabel;

	private: bool m_programFinished;
	private: System::Windows::Forms::SplitContainer^  splitContainer1;
	private: System::Windows::Forms::Button^  compileButton;
	private:
		/// <summary>
		/// Required designer variable.
		/// </summary>
		System::ComponentModel::Container ^components;

#pragma region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		void InitializeComponent(void)
		{
			this->programInputTextBox = (gcnew System::Windows::Forms::RichTextBox());
			this->variablesGridView = (gcnew System::Windows::Forms::DataGridView());
			this->varNameCol = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->varTypeCol = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->varValueCol = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->programLabel = (gcnew System::Windows::Forms::Label());
			this->variablesLabel = (gcnew System::Windows::Forms::Label());
			this->runLabel = (gcnew System::Windows::Forms::Label());
			this->runComboBox = (gcnew System::Windows::Forms::ComboBox());
			this->scopeLabel = (gcnew System::Windows::Forms::Label());
			this->scopingComboBox = (gcnew System::Windows::Forms::ComboBox());
			this->programOutputTextBox = (gcnew System::Windows::Forms::RichTextBox());
			this->outputLabel = (gcnew System::Windows::Forms::Label());
			this->splitContainer1 = (gcnew System::Windows::Forms::SplitContainer());
			this->compileButton = (gcnew System::Windows::Forms::Button());
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->variablesGridView))->BeginInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->splitContainer1))->BeginInit();
			this->splitContainer1->Panel1->SuspendLayout();
			this->splitContainer1->Panel2->SuspendLayout();
			this->splitContainer1->SuspendLayout();
			this->SuspendLayout();
			// 
			// programInputTextBox
			// 
			this->programInputTextBox->AcceptsTab = true;
			this->programInputTextBox->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Bottom) 
				| System::Windows::Forms::AnchorStyles::Left) 
				| System::Windows::Forms::AnchorStyles::Right));
			this->programInputTextBox->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9.75F, System::Drawing::FontStyle::Regular, 
				System::Drawing::GraphicsUnit::Point, static_cast<System::Byte>(0)));
			this->programInputTextBox->Location = System::Drawing::Point(3, 16);
			this->programInputTextBox->Name = L"programInputTextBox";
			this->programInputTextBox->Size = System::Drawing::Size(488, 584);
			this->programInputTextBox->TabIndex = 0;
			this->programInputTextBox->Text = L"";
			this->programInputTextBox->SelectionChanged += gcnew System::EventHandler(this, &GUICompleteParser::programInputTextBox_SelectionChanged);
			this->programInputTextBox->TextChanged += gcnew System::EventHandler(this, &GUICompleteParser::programInputTextBox_TextChanged);
			this->programInputTextBox->KeyPress += gcnew System::Windows::Forms::KeyPressEventHandler(this, &GUICompleteParser::programInputTextBox_KeyPress);
			// 
			// variablesGridView
			// 
			this->variablesGridView->AllowUserToAddRows = false;
			this->variablesGridView->AllowUserToDeleteRows = false;
			this->variablesGridView->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left) 
				| System::Windows::Forms::AnchorStyles::Right));
			this->variablesGridView->ColumnHeadersHeightSizeMode = System::Windows::Forms::DataGridViewColumnHeadersHeightSizeMode::AutoSize;
			this->variablesGridView->Columns->AddRange(gcnew cli::array< System::Windows::Forms::DataGridViewColumn^  >(3) {this->varNameCol, 
				this->varTypeCol, this->varValueCol});
			this->variablesGridView->Location = System::Drawing::Point(6, 16);
			this->variablesGridView->Name = L"variablesGridView";
			this->variablesGridView->ReadOnly = true;
			this->variablesGridView->Size = System::Drawing::Size(434, 228);
			this->variablesGridView->TabIndex = 1;
			// 
			// varNameCol
			// 
			this->varNameCol->HeaderText = L"Name";
			this->varNameCol->Name = L"varNameCol";
			this->varNameCol->ReadOnly = true;
			// 
			// varTypeCol
			// 
			this->varTypeCol->HeaderText = L"Type";
			this->varTypeCol->Name = L"varTypeCol";
			this->varTypeCol->ReadOnly = true;
			// 
			// varValueCol
			// 
			this->varValueCol->HeaderText = L"Value";
			this->varValueCol->Name = L"varValueCol";
			this->varValueCol->ReadOnly = true;
			// 
			// programLabel
			// 
			this->programLabel->AutoSize = true;
			this->programLabel->BackColor = System::Drawing::Color::Transparent;
			this->programLabel->Location = System::Drawing::Point(3, 0);
			this->programLabel->Name = L"programLabel";
			this->programLabel->Size = System::Drawing::Size(46, 13);
			this->programLabel->TabIndex = 2;
			this->programLabel->Text = L"Program";
			// 
			// variablesLabel
			// 
			this->variablesLabel->AutoSize = true;
			this->variablesLabel->BackColor = System::Drawing::Color::Transparent;
			this->variablesLabel->Location = System::Drawing::Point(3, 0);
			this->variablesLabel->Name = L"variablesLabel";
			this->variablesLabel->Size = System::Drawing::Size(50, 13);
			this->variablesLabel->TabIndex = 3;
			this->variablesLabel->Text = L"Variables";
			// 
			// runLabel
			// 
			this->runLabel->AutoSize = true;
			this->runLabel->BackColor = System::Drawing::Color::Transparent;
			this->runLabel->Location = System::Drawing::Point(3, 258);
			this->runLabel->Name = L"runLabel";
			this->runLabel->Size = System::Drawing::Size(66, 13);
			this->runLabel->TabIndex = 4;
			this->runLabel->Text = L"Run Options";
			// 
			// runComboBox
			// 
			this->runComboBox->AutoCompleteMode = System::Windows::Forms::AutoCompleteMode::Suggest;
			this->runComboBox->AutoCompleteSource = System::Windows::Forms::AutoCompleteSource::ListItems;
			this->runComboBox->DropDownStyle = System::Windows::Forms::ComboBoxStyle::DropDownList;
			this->runComboBox->FormattingEnabled = true;
			this->runComboBox->Items->AddRange(gcnew cli::array< System::Object^  >(3) {L"Auto", L"Caret", L"Manual"});
			this->runComboBox->Location = System::Drawing::Point(6, 274);
			this->runComboBox->Name = L"runComboBox";
			this->runComboBox->Size = System::Drawing::Size(121, 21);
			this->runComboBox->TabIndex = 5;
			this->runComboBox->SelectionChangeCommitted += gcnew System::EventHandler(this, &GUICompleteParser::runComboBox_SelectionChangeCommitted);
			// 
			// scopeLabel
			// 
			this->scopeLabel->AutoSize = true;
			this->scopeLabel->BackColor = System::Drawing::Color::Transparent;
			this->scopeLabel->Location = System::Drawing::Point(229, 258);
			this->scopeLabel->Name = L"scopeLabel";
			this->scopeLabel->Size = System::Drawing::Size(85, 13);
			this->scopeLabel->TabIndex = 7;
			this->scopeLabel->Text = L"Scoping Options";
			// 
			// scopingComboBox
			// 
			this->scopingComboBox->AutoCompleteMode = System::Windows::Forms::AutoCompleteMode::Suggest;
			this->scopingComboBox->AutoCompleteSource = System::Windows::Forms::AutoCompleteSource::ListItems;
			this->scopingComboBox->DropDownStyle = System::Windows::Forms::ComboBoxStyle::DropDownList;
			this->scopingComboBox->Enabled = false;
			this->scopingComboBox->FormattingEnabled = true;
			this->scopingComboBox->Items->AddRange(gcnew cli::array< System::Object^  >(2) {L"Static", L"Off"});
			this->scopingComboBox->Location = System::Drawing::Point(232, 275);
			this->scopingComboBox->Name = L"scopingComboBox";
			this->scopingComboBox->Size = System::Drawing::Size(121, 21);
			this->scopingComboBox->TabIndex = 8;
			this->scopingComboBox->SelectionChangeCommitted += gcnew System::EventHandler(this, &GUICompleteParser::scopingComboBox_SelectionChangeCommitted);
			// 
			// programOutputTextBox
			// 
			this->programOutputTextBox->AcceptsTab = true;
			this->programOutputTextBox->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Bottom) 
				| System::Windows::Forms::AnchorStyles::Left) 
				| System::Windows::Forms::AnchorStyles::Right));
			this->programOutputTextBox->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9.75F, System::Drawing::FontStyle::Regular, 
				System::Drawing::GraphicsUnit::Point, static_cast<System::Byte>(0)));
			this->programOutputTextBox->Location = System::Drawing::Point(6, 331);
			this->programOutputTextBox->Name = L"programOutputTextBox";
			this->programOutputTextBox->ReadOnly = true;
			this->programOutputTextBox->Size = System::Drawing::Size(434, 237);
			this->programOutputTextBox->TabIndex = 9;
			this->programOutputTextBox->Text = L"";
			// 
			// outputLabel
			// 
			this->outputLabel->AutoSize = true;
			this->outputLabel->BackColor = System::Drawing::Color::Transparent;
			this->outputLabel->Location = System::Drawing::Point(3, 315);
			this->outputLabel->Name = L"outputLabel";
			this->outputLabel->Size = System::Drawing::Size(39, 13);
			this->outputLabel->TabIndex = 10;
			this->outputLabel->Text = L"Output";
			// 
			// splitContainer1
			// 
			this->splitContainer1->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Bottom) 
				| System::Windows::Forms::AnchorStyles::Left) 
				| System::Windows::Forms::AnchorStyles::Right));
			this->splitContainer1->Location = System::Drawing::Point(12, 12);
			this->splitContainer1->Name = L"splitContainer1";
			// 
			// splitContainer1.Panel1
			// 
			this->splitContainer1->Panel1->Controls->Add(this->programLabel);
			this->splitContainer1->Panel1->Controls->Add(this->programInputTextBox);
			// 
			// splitContainer1.Panel2
			// 
			this->splitContainer1->Panel2->Controls->Add(this->compileButton);
			this->splitContainer1->Panel2->Controls->Add(this->variablesLabel);
			this->splitContainer1->Panel2->Controls->Add(this->outputLabel);
			this->splitContainer1->Panel2->Controls->Add(this->variablesGridView);
			this->splitContainer1->Panel2->Controls->Add(this->programOutputTextBox);
			this->splitContainer1->Panel2->Controls->Add(this->runLabel);
			this->splitContainer1->Panel2->Controls->Add(this->scopingComboBox);
			this->splitContainer1->Panel2->Controls->Add(this->runComboBox);
			this->splitContainer1->Panel2->Controls->Add(this->scopeLabel);
			this->splitContainer1->Size = System::Drawing::Size(944, 568);
			this->splitContainer1->SplitterDistance = 497;
			this->splitContainer1->TabIndex = 11;
			// 
			// compileButton
			// 
			this->compileButton->Location = System::Drawing::Point(133, 273);
			this->compileButton->Name = L"compileButton";
			this->compileButton->Size = System::Drawing::Size(75, 23);
			this->compileButton->TabIndex = 11;
			this->compileButton->Text = L"Compile";
			this->compileButton->UseVisualStyleBackColor = true;
			this->compileButton->Click += gcnew System::EventHandler(this, &GUICompleteParser::compileButton_Click);
			// 
			// GUICompleteParser
			// 
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::None;
			this->ClientSize = System::Drawing::Size(968, 592);
			this->Controls->Add(this->splitContainer1);
			this->MinimumSize = System::Drawing::Size(800, 600);
			this->Name = L"GUICompleteParser";
			this->ShowIcon = false;
			this->Text = L"CompleteParser";
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->variablesGridView))->EndInit();
			this->splitContainer1->Panel1->ResumeLayout(false);
			this->splitContainer1->Panel1->PerformLayout();
			this->splitContainer1->Panel2->ResumeLayout(false);
			this->splitContainer1->Panel2->PerformLayout();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->splitContainer1))->EndInit();
			this->splitContainer1->ResumeLayout(false);
			this->ResumeLayout(false);

		}
#pragma endregion
private: System::Void programInputTextBox_TextChanged(System::Object^  sender, System::EventArgs^  e)
		 {
			//if(this->runComboBox->SelectedItem->ToString() != RUN_OPTIONS_MANUAL)
			//	SendInputToCompleteParser();
		 }
private: System::Void programInputTextBox_KeyPress(System::Object^  sender, System::Windows::Forms::KeyPressEventArgs^  e)
		 {
			 // Enter pressed.
			 if(e->KeyChar == 13)
			 {
				 BracketSpacing();
			 }
		 }
private: System::Void programInputTextBox_SelectionChanged(System::Object^  sender, System::EventArgs^  e)
		 {
			 if(this->runComboBox->SelectedItem->ToString() == RUN_OPTIONS_CARET)
				SendInputToCompleteParser();
		 }
private: System::Void scopingComboBox_SelectionChangeCommitted(System::Object^  sender, System::EventArgs^  e)
		 {
			 SendInputToCompleteParser();
		 }
private: System::Void runComboBox_SelectionChangeCommitted(System::Object^  sender, System::EventArgs^  e)
		 {
			 SendInputToCompleteParser();
		 }
private: System::Void compileButton_Click(System::Object^  sender, System::EventArgs^  e)
		 {
			 CompileProgram();
		 }
};
}
