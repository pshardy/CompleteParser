#include "Global.h"

#if !_CONSOLE
#include <msclr/marshal_cppstd.h>

System::String^ NativeToDotNet(string& input)
{
	return System::Runtime::InteropServices::Marshal::PtrToStringAnsi((System::IntPtr)(char*)input.c_str());
}

string DotNetToNative(System::String^ input)
{
	return msclr::interop::marshal_as<std::string>(input);
}
#endif