#include "filewatch.h"

#include "debug.h"
#include "defines.h"

#include "platform/platform.h"
#include "utils/strings/hklocale.h"
#include "utils/containers/hkvector.h"

#include <thread>

namespace hk::filewatch {

static constexpr DWORD buffer_size = 1024;
static constexpr DWORD notify_filter =
    // FILE_NOTIFY_CHANGE_ATTRIBUTES  |
    // FILE_NOTIFY_CHANGE_SIZE        |
    // FILE_NOTIFY_CHANGE_LAST_ACCESS |
    // FILE_NOTIFY_CHANGE_SECURITY    |
    FILE_NOTIFY_CHANGE_FILE_NAME   |
    FILE_NOTIFY_CHANGE_DIR_NAME    |
    FILE_NOTIFY_CHANGE_LAST_WRITE  |
    FILE_NOTIFY_CHANGE_CREATION;

class target {
public:
    void init(const std::string &path, onStateChange callback)
    {
        path_ = path;
        callback_ = callback;
        watcher_ = std::thread([this](){ watch(); });

        destroy_ = CreateEvent(NULL, TRUE, FALSE, NULL);
        watching_ = true;
    }

    void deinit()
    {
        watching_ = false;
        SetEvent(destroy_);
        watcher_.join();
        CloseHandle(handle_);
    }

    void watch()
    {
        handle_ = CreateFile(
            path_.c_str(),
            FILE_LIST_DIRECTORY,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            NULL,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
            NULL
        );

        ALWAYS_ASSERT(handle_ != INVALID_HANDLE_VALUE,
                      "Failed to open directory");

        hk::vector<BYTE> buffer(buffer_size);
        DWORD bytes_returned = 0;

        OVERLAPPED overlapped = { 0 };
        overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        ALWAYS_ASSERT(overlapped.hEvent,
                      "Failed to create overlapped data event");

        HANDLE events[] = { overlapped.hEvent, destroy_ };

        while (watching_) {
            BOOL result = ReadDirectoryChangesW(
                handle_,
                buffer.data(), static_cast<DWORD>(buffer.size()),
                TRUE, notify_filter,
                &bytes_returned, &overlapped, NULL);

            if (!result && GetLastError() != ERROR_IO_PENDING) {
                LOG_WARN("Failed to read directory changes:", GetLastError());
                break;
            }

            switch (WaitForMultipleObjects(2, events, FALSE, INFINITE)) {
            case WAIT_OBJECT_0 + 0: {
                GetOverlappedResult(handle_, &overlapped,
                                    &bytes_returned, TRUE);

                if (!bytes_returned) { break; }

                FILE_NOTIFY_INFORMATION *notify_info =
                    reinterpret_cast<FILE_NOTIFY_INFORMATION*>(buffer.data());

                do {
                    constexpr State states[] = {
                        State::NONE,

                        State::ADDED,
                        State::REMOVED,
                        State::MODIFIED,
                        State::RENAMED_OLD,
                        State::RENAMED_NEW,
                    };

                    std::wstring in(notify_info->FileName,
                                    notify_info->FileNameLength);
                    callback_(hk::wstring_convert(in),
                              states[notify_info->Action]);

                    if (!notify_info->NextEntryOffset) { break; }

                    notify_info = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(
                        reinterpret_cast<LPBYTE>(notify_info) +
                        notify_info->NextEntryOffset
                    );
                } while (true);
            } break;
            case WAIT_OBJECT_0 + 1:
            case WAIT_TIMEOUT:
            default:
                break;
            }
        }
        return;
    }

private:
    std::string path_;
    onStateChange callback_;
    HANDLE handle_;
    std::thread watcher_;

    HANDLE destroy_;
    b8 watching_;
};

static std::unordered_map<std::string, target*> targets;

void init()
{
    targets.clear();
}

void deinit()
{
    for (auto &target : targets) {
        target.second->deinit();
        delete target.second;
    }

    targets.clear();
}

void watch(const std::string &path, onStateChange callback)
{
    if (!targets.size()) { init(); }

    target *t = new target();
    t->init(path, callback);
    targets[path] = t;
}

void unwatch(const std::string &path)
{
    // if (!targets.at(path)) { return; }
    targets[path]->deinit();
    delete targets[path];
}

}
