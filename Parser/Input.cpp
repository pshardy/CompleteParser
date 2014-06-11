#include "Input.h"

Input::Input()
{
	m_currentLineNumber	= 0;
	m_buffer			= 0;
	m_usedBufferSize	= m_currentIndex = m_lastGoodIndex = m_maxBufferSize = 0;
	m_eol				= false;
	m_fileOpened		= false;
}

Input::~Input()
{
}

bool Input::Initialize(unsigned int maxSize)
{
	m_currentLineNumber = 0;

	if(maxSize == 0)
		return false;

	m_buffer = new char[m_maxBufferSize = maxSize];

	if(!m_buffer)
		return false;

	ClearBuffer();

	return true;
}

void Input::Shutdown()
{
	ClearBuffer();

	if(m_buffer)
	{
		delete[] m_buffer;
		m_buffer = 0;
	}

	m_file.close();
}

void Input::ClearBuffer()
{
	// Clear all data stored in the buffer.
	if(m_buffer)
	{
		memset(m_buffer, NULL_ENTRY, m_maxBufferSize);
	}

	// Reset indices.
	m_usedBufferSize = m_currentIndex = 0;

	// Clear eof flag.
	m_eol = false;

	// Reset line number.
	m_currentLineNumber = 0;
}

bool Input::IsEndOfLine(char c)
{
	return c == '\n' || c == '\r' || c == '\0' || c == NULL_ENTRY;
}

bool Input::IsEOL()
{
	return m_eol;
}

bool Input::IsEOF()
{
	if(m_fileOpened && (m_file.eof() || m_file.bad()))
		return true;

	return cin.bad() || cin.eof();
}

__declspec(dllexport) bool Input::OpenFile(char* fileName)
{
	m_file.open(fileName);

	m_fileOpened = m_file.good();

	return m_file.good();
}

__declspec(dllexport) bool Input::SetFile(string& file)
{
	m_fileMem = file;

	m_fileOpened = file.length();

	return m_fileOpened;
}

void Input::CloseFile()
{
	m_file.close();
	m_fileMem.clear();
	m_fileOpened = false;
}

__declspec(dllexport) void Input::SetBuffer(string& text)
{
	// Make sure buffer is clear before writing.
	ClearBuffer();
	
	m_usedBufferSize = text.size();
	if(m_usedBufferSize >= m_maxBufferSize)
	{
		cout << "WARNING: BUFFER OVERFLOW\n";
		return;
	}

	for(int i = 0; i < text.size(); i++)
	{
		m_buffer[i] = text[i];
	}

	// No input.
	m_eol = !m_usedBufferSize;
}

void Input::ReadLine()
{
	// Make sure buffer is clear before writing.
	ClearBuffer();

	// Check for memory to read data from.
	if(m_fileMem.length())
	{
		for(unsigned int c = 0; c < m_fileMem.length(); c++)
		{
			m_buffer[c] = m_fileMem[c];
			m_usedBufferSize++;
		}
		m_buffer[m_usedBufferSize++] = '\0';
		// Clear memory since it is just a string.
		m_fileMem.clear();
	} // Check for a file to read data from.
	else if(m_file.good())
	{
		string line;
		getline(m_file, line);

		for(unsigned int c = 0; c < line.length(); c++)
		{
			m_buffer[c] = line[c];
			m_usedBufferSize++;
		}
		m_buffer[m_usedBufferSize++] = '\0';
	}
	// Standard user input.
	else
	{
		// Loop through all of stdin and complete the buffer.
		cin.clear();
		cin.getline(m_buffer, m_maxBufferSize);

		while((m_usedBufferSize < m_maxBufferSize) && (m_buffer[m_usedBufferSize++] != NULL_ENTRY));
	}
	// No input.
	m_eol = !m_usedBufferSize;
}

bool Input::PeekChar(char& c, int offset)
{
	if((m_currentIndex + offset) >= m_usedBufferSize)
		return false;

	c = m_buffer[m_currentIndex + offset];

	return true;
}

bool Input::GetCurrentChar(char& c)
{
	// Check bounds.
	if(m_currentIndex >= m_usedBufferSize)
	{
		// Set the last word index only if it was just white space before the end of the file.
		if((m_currentIndex - 1) >= 0 && !isspace((int)m_buffer[(m_currentIndex - 1)]))
			m_lastGoodIndex = m_currentIndex;
		m_eol = true;
		return false;
	}

	// Set the last word index.
	m_lastGoodIndex = m_currentIndex;

	// Check white space. Increment to end of white space but return false indicating a word was read in.
	bool whiteSpace = (bool)isspace((int)m_buffer[m_currentIndex]);

	while(whiteSpace && m_currentIndex < m_usedBufferSize && isspace((int)(m_buffer[m_currentIndex])))
	{
		if(IsEndOfLine(m_buffer[m_currentIndex]))
		{
			m_currentLineNumber++;
		}
		m_currentIndex++;
	}

	// Check if a space is right before a line break. @#$@! This fixed a bug which kept me up for @#$*&@ HOURS!
	if(whiteSpace)
	{
		if(m_buffer[m_currentIndex] == NULL_ENTRY)
		{
			m_currentIndex = m_usedBufferSize;
		}
		return false;
	}

	c = m_buffer[m_currentIndex++];

	if(c == '\0')
		return false;

	return true;
}

int Input::GetCurrentIndex()
{
	return m_currentIndex;
}

int Input::GetLastGoodIndex()
{
	return m_lastGoodIndex;
}

__declspec(dllexport) int Input::GetLineNumber()
{
	return m_currentLineNumber;
}

bool Input::DecrementIndex()
{
	if(m_currentIndex == 0) return false;
	m_lastGoodIndex--;
	m_currentIndex--;
	return true;
}

string Input::CopyBuffer(int start, int end, bool includeSpace)
{
	if(end > (int)m_usedBufferSize)
		end = m_usedBufferSize;

	int size = end - start;

	if(size <= 0)
		return "";

	// Add one to str for null terminator through c_str() -- not really necessary.
	string newStr;
	newStr.reserve(size + 1);

	for(int i = 0; i < size; i++)
	{
		if(includeSpace || !isspace(m_buffer[i + start]))
			newStr.push_back(m_buffer[i + start]);
	}

	return newStr;
}