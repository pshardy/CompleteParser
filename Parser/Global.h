#include <string>

using namespace std;

#if !_CONSOLE
// String handling
extern System::String^ NativeToDotNet(string& input);	// Convert unmanaged string to managed.
extern string DotNetToNative(System::String^ input);	// Convert managed string to unmanaged.
#endif
