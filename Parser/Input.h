////////////////////////////////////////////////////////////////////////////////
// Filename: Input.h
////////////////////////////////////////////////////////////////////////////////
#ifndef _INPUT_H_
#define _INPUT_H_

//#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <string>
#include <iostream>
#include <fstream>
#include <algorithm>

using namespace std;

#define NULL_ENTRY	'\n'

////////////////////////////////////////////////////////////////////////////////
// Class name: Input
//
// Loads user input into a buffer. Provides regulated access to the buffer.
// The buffer can also be copied to std::string for self-contained memory management.
////////////////////////////////////////////////////////////////////////////////
class Input
{
public:
	Input();
	~Input();

	bool Initialize(unsigned int maxSize);
	void Shutdown();

	__declspec(dllexport) bool OpenFile(char* fileName);
													// Open a file to m_file.

	__declspec(dllexport) bool SetFile(string&);	// Manually load a file from memory.
	void CloseFile();								// Close m_file.
	void ReadLine();								// Store an entire line of input into the buffer. Clears EOF.
	__declspec(dllexport) void SetBuffer(string& text);		
													// Manually set the buffer.

	bool GetCurrentChar(char& cOut);				// Retrieve the char at the current index and increase it. False if EOF.
	bool PeekChar(char& cOut, int offset = 0);		// Peek at the current char + offset without increasing the index. Whitespace is valid.
	int GetCurrentIndex();							// Returns current index without increasing it.
	int GetLastGoodIndex();							// Returns the last index prior to a white space.
	__declspec(dllexport) int GetLineNumber();		// Returns m_currentLineNumber.
	bool DecrementIndex();							// Lower the current index if greater than 0.
	bool IsEOL();									// Returns m_eol.
	bool IsEOF();									// Checks stdin or opened file if it is EOF.
	bool IsEndOfLine(char c);						// Is the char a new line character.
	string CopyBuffer(int start, int end,			// Returns a new string of the correct length with contents from the buffer.
		bool includeSpace = false);			

private:
	void ClearBuffer();
	
private:
	ifstream		m_file;							// Read input from a file (TOKEN_DEBUG)
	string			m_fileMem;						// Contents loaded directly to memory.
	char			*m_buffer;						// Buffer containing entered input.
	unsigned int	m_maxBufferSize;				// The maximum size the buffer is allowed.
	unsigned int	m_usedBufferSize;				// The current used size of the buffer.
	unsigned int	m_lastGoodIndex;				// Last position before white space.
	int				m_currentLineNumber;			// The current line number of the line being processed.
	int				m_currentIndex;					// Current index of the buffer.
	bool			m_eol;							// When the line has been fully read in.
	bool			m_fileOpened;					// If m_file has been opened.
};





#endif