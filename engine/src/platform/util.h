// FIX: tmp file
// Source: https://forum.juce.com/t/detecting-if-a-process-is-being-run-under-a-debugger/2098

#include "utility/hktypes.h"

b8 is_under_debugger();

#ifdef HKLINUX
#include <sys/types.h>
#include <sys/ptrace.h>

i32 underDebugger = 0;
// This works under any BSD kernel
b8 is_under_debugger()
{
    static bool isCheckedAlready = false;
    if (!isCheckedAlready) {
        if (ptrace(PTRACE_TRACEME, 0, 1, 0) < 0) {
             underDebugger = 1;
        } else {
            ptrace(PTRACE_DETACH, 0, 1, 0);
        }

        isCheckedAlready = true;
    }
    return underDebugger == 1;
}
#endif // HKLINUX

#ifdef HKWIN32
#include "backend/Win32/win32.h"

b8 is_under_debugger()
{
     return IsDebuggerPresent() == TRUE;
}
#endif // HKWIN32
