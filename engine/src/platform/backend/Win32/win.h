#ifndef HK_WIN_H
#define HK_WIN_H

#ifdef UNDEFINED_MIN
#pragma pop_macro("min")
#endif

#ifdef UNDEFINED_MAX
#pragma pop_macro("max")
#endif

#ifdef UNDEFINED_NEAR
#pragma pop_macro("near")
#endif

#ifdef UNDEFINED_FAR
#pragma pop_macro("far")
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <hidusage.h>
#include <shellapi.h>
#include <commctrl.h>
#include <wrl/client.h>

#ifdef min
#define UNDEFINED_MIN
#pragma push_macro("min")
#undef min
#endif

#ifdef max
#define UNDEFINED_MAX
#pragma push_macro("max")
#undef max
#endif

#ifdef near
#define UNDEFINED_NEAR
#pragma push_macro("near")
#undef near
#endif

#ifdef far
#define UNDEFINED_FAR
#pragma push_macro("far")
#undef far
#endif

// INFO: Needed to get Version 6 of Common Controls
#if defined _M_IX86
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif


#endif // HK_WIN_H
