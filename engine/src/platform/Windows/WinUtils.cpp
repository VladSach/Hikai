#include "platform/utils.h"

#include "platform/platform.h"

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

}
