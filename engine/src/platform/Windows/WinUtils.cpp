#include "platform/utils.h"

#include "platform/platform.h"

#include "hkstl/strings/hklocale.h"

#include <shellapi.h>

#ifdef UNDEFINED_NEAR
    #pragma pop_macro("near")
#endif // UNDEFINED_FAR

#include <commctrl.h>

#ifdef far
    #define UNDEFINED_NEAR
    #pragma push_macro("near")
    #undef far
#endif // far

// Needed to get Version 6 of Common Controls
#if defined _M_IX86
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

namespace hk::platform {

b8 copyToClipboard(const std::string &target)
{
    if (!OpenClipboard(NULL)) { return false; }

    if (!EmptyClipboard()) {
        CloseClipboard();
        return false;
    }

    HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE, target.size() + 1);
    if (!hg) {
        CloseClipboard();
        return false;
    }

    LPVOID lock = GlobalLock(hg);
    memcpy(lock, target.c_str(), target.size() + 1);
    GlobalUnlock(hg);

    if (!SetClipboardData(CF_TEXT, hg)) {
        GlobalFree(hg);
        CloseClipboard();
        return false;
    }

    CloseClipboard();
    return false;
}

void addMessageBox(const std::string &name, const std::string &message)
{
    MessageBox(0, message.c_str(), name.c_str(), MB_ICONERROR);
}

// Callback handler for the task dialog
HRESULT CALLBACK TaskDialogCallback(
    HWND hwndFocus,
    UINT uNotification,
    WPARAM wParam,
    LPARAM lParam,
    LONG_PTR dwRefData)
{
    (void) hwndFocus; (void) wParam; (void) dwRefData;

    switch (uNotification) {
    case TDN_HYPERLINK_CLICKED: {
        PCWSTR pszHREF = (PCWSTR)lParam;
        ShellExecuteW(NULL, L"open", pszHREF, NULL, NULL, SW_SHOWNORMAL);
    } break;
    }

    return S_OK;
}

void addTaskDialog(const std::string &title,
                   const std::string &header,
                   const std::string &message)
{
    std::wstring wtitle = hk::string_convert(title);
    std::wstring wheader = hk::string_convert(header);
    std::wstring wmessage = hk::string_convert(message);

    // TASKDIALOG_BUTTON buttons[] = {
    //     { IDOK, L"Continue" },
    //     { IDCANCEL, L"Abort" }
    // };

    /* ISSUE: Enabling hyperlinks when using content from an unsafe source
     * may cause security vulnerabilities */

    TASKDIALOGCONFIG config = {};
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_ENABLE_HYPERLINKS | TDF_SIZE_TO_CONTENT;
    config.pszWindowTitle = wtitle.data();
    config.pszMainIcon = TD_ERROR_ICON;
    config.pszMainInstruction = wheader.data();
    config.pszContent = wmessage.data();
    // config.cButtons = ARRAYSIZE(buttons);
    // config.pButtons = buttons;

    config.pfCallback = TaskDialogCallback;

    TaskDialogIndirect(&config, NULL, NULL, NULL);
}

}
