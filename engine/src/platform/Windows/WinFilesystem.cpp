#include "platform/filesystem.h"

#include "win.h"
#include "PathCch.h"

#include "hkstl/strings/hklocale.h"

#include <fstream>
#include <string>
#include <filesystem>

namespace hk::filesystem {

b8 read_file(const std::string &path, hk::vector<u8> &out)
{
    std::ifstream file(path, std::ios::binary | std::ios::in | std::ios::ate);
    // ALWAYS_ASSERT(file.is_open(), "Failed to open a file:", path);
    if (!file.is_open()) { return false; }

    const u32 size = static_cast<u32>(file.tellg());
    file.seekg(0, file.beg);
    out.resize(size);
    file.read((char*)(out.data()), size);
    file.close();

    return true;
}

b8 find_file(const std::string &root, const std::string &target,
            std::string *out)
{
    std::string searchPath(root);

    // Remove trailing and '\' and '/'
    while (!searchPath.empty() &&
           (searchPath.back() == '\\' || searchPath.back() == L'/'))
    {
        searchPath.pop_back();
    }

    searchPath += "\\*";

    // FIX: ? a little quick fix, maybe should change this
    if (searchPath.find(target) != std::string::npos) {
        if (out) { *out = root; }
        return true;
    }

    WIN32_FIND_DATA data;
    HANDLE hFind = FindFirstFileEx(searchPath.c_str(),
                                   FindExInfoBasic,
                                   &data,
                                   FindExSearchLimitToDirectories,
                                   NULL, 0);

    if (hFind == INVALID_HANDLE_VALUE) { return false; }

    do {
        if (strcmp(data.cFileName, target.c_str()) == 0) {
            if (out) { *out = root + '\\' + data.cFileName; }
            // LOG_DEBUG("File found at:", out);
            return true;
        }

        if (strcmp(data.cFileName, ".")  != 0 &&
            strcmp(data.cFileName, "..") != 0)
        {
            std::string subDir(searchPath.substr(0, searchPath.size() - 1));
            subDir += data.cFileName;

            if (find_file(subDir, target, out)) { return true; }
        }

    } while (FindNextFile(hFind, &data));

    FindClose(hFind);
    return false;
}

b8 exists(const std::string &path)
{
    DWORD attr = GetFileAttributesA(path.c_str());

    if (attr == INVALID_FILE_ATTRIBUTES) {
        DWORD err = GetLastError();
        if (err == ERROR_PATH_NOT_FOUND ||
            err == ERROR_FILE_NOT_FOUND ||
            err == ERROR_INVALID_NAME   ||
            err == ERROR_BAD_NETPATH)
        {
            return false;
        }

        LOG_WARN("Path exists but inaccessible");
    }

    return true;
}

hk::vector<std::string> split(const std::string &path)
{
    hk::vector<std::string> subpaths;
    std::string subpath;

    for (auto &c : path) {
        if (c == '\\' || c == '/') {
            if (!subpath.empty()) {
                subpaths.push_back(subpath);
                subpath.clear();
            }
        } else {
            subpath += c;
        }
    }

    if (!subpath.empty()) {
        subpaths.push_back(subpath);
    }

    return subpaths;
}

typedef HRESULT (WINAPI *PathCchCanonicalizeExFunc)(
    PWSTR pszPathOut,
    size_t cchPathOut,
    PCWSTR pszPathIn,
    ULONG dwFlags
);

// https://learn.microsoft.com/en-us/windows/win32/apiindex/windows-apisets
HMODULE hModule = LoadLibraryExW(L"api-ms-win-core-path-l1-1-0.dll",
                                    NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);

std::string canonical(const std::string &path)
{
    // FIX: works, but I don't like the way it's written

    static PathCchCanonicalizeExFunc PathCchCanonicalizeEx =
        (PathCchCanonicalizeExFunc)GetProcAddress(hModule, "PathCchCanonicalizeEx");

    std::wstring wpath = string_convert(path);

    DWORD flags = FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT;
    HANDLE hFile = CreateFileW(wpath.c_str(),
                               GENERIC_READ, FILE_SHARE_READ |
                               FILE_SHARE_WRITE |
                               FILE_SHARE_DELETE,
                               NULL, OPEN_EXISTING, flags, NULL);

    if (hFile == INVALID_HANDLE_VALUE) {
        // Path doesn't exist or we don't have access rights
        //
        // wpath = widen(p.root_path().string());
        // if (!PathCanonicalizeW(wpath.data(), wpath.data())) {
        //     throw std::runtime_error("Failed to canonicalize path");
        // }
        // return narrow(wpath);
    }

    HRESULT hr;

    // Get full path
    WCHAR buffer[MAX_PATH];
    DWORD result = GetFullPathNameW(wpath.c_str(), MAX_PATH, buffer, nullptr);
    if (result == 0 || result > MAX_PATH) {
        CloseHandle(hFile);
        ALWAYS_ASSERT(0, "Failed to get full path");
    }

    // Canonicalize path
    WCHAR buffer2[MAX_PATH];
    hr = PathCchCanonicalizeEx(buffer2, MAX_PATH, buffer, PATHCCH_ALLOW_LONG_PATHS);
    if (hr != S_OK) {
        CloseHandle(hFile);
        ALWAYS_ASSERT(0, "Failed to canonicalize path");
    }

    return wstring_convert(std::wstring(buffer2));
}

typedef BOOL (WINAPI *PathRelativePathToAFunc)(
    LPSTR  pszPath,
    LPCSTR pszFrom,
    DWORD  dwAttrFrom,
    LPCSTR pszTo,
    DWORD  dwAttrTo
);

std::string relative(const std::string &path, const std::string &base)
{
    DWORD fromAttrs = GetFileAttributesA(base.c_str());
    DWORD toAttrs = GetFileAttributesA(path.c_str());

    char outBuf[MAX_PATH];

    static PathRelativePathToAFunc PathRelativePathToA =
        (PathRelativePathToAFunc)GetProcAddress(hModule, "PathRelativePathToA");

    BOOL success = PathRelativePathToA(outBuf, base.data(),
                                       (fromAttrs & FILE_ATTRIBUTE_DIRECTORY) ? FILE_ATTRIBUTE_DIRECTORY : 0,
                                       path.data(),
                                       (toAttrs & FILE_ATTRIBUTE_DIRECTORY) ? FILE_ATTRIBUTE_DIRECTORY : 0);

    if (!success) {
        LOG_WARN("Failed to calculate relative path");
    }

    return std::string(outBuf);
}

struct DirectoryIterator::Impl {
    HANDLE hFind;
    WIN32_FIND_DATA data;

    b8 valid;
    std::string currentPath;

    Impl(const std::string &path) :
        hFind(INVALID_HANDLE_VALUE), valid(false)
    {
        std::string searchPath = path + "\\*";
        hFind = FindFirstFileA(searchPath.c_str(), &data);

        valid = (hFind != INVALID_HANDLE_VALUE);

        if (valid) {
            currentPath = path;

            while (!strcmp(data.cFileName, ".") ||
                   !strcmp(data.cFileName, ".."))
            {
                next();
            }
        }
    }

    ~Impl() {
        if (hFind != INVALID_HANDLE_VALUE) {
            FindClose(hFind);
        }
    }

    b8 next() {
        if (!valid || hFind == INVALID_HANDLE_VALUE) {
            return false;
        }

        if (!FindNextFileA(hFind, &data)) {
            DWORD error = GetLastError();
            if (error != ERROR_NO_MORE_FILES) {
                LOG_ERROR("Error enumerating directory:", error);
            }
            valid = false;
        }

        return valid;
    }

    b8 isDirectory() const {
        return data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
    }

    std::string getPath() const {
        return currentPath + "\\" + data.cFileName;
    }

    std::string getName() const {
        return data.cFileName;
    }
};

DirectoryIterator::DirectoryIterator(const std::string& path)
{
    impl = new Impl(path);
}

DirectoryIterator::~DirectoryIterator()
{
    delete impl;
}


b8 DirectoryIterator::operator !=(const DirectoryIterator&) const
{
    return impl->valid;
}

DirectoryIterator& DirectoryIterator::operator ++()
{
    impl->next();
    return *this;
}

const DirectoryIterator::Entry& DirectoryIterator::operator *()
{
    cur.path = impl->getPath();
    cur.name = impl->getName();
    cur.isDirectory = impl->isDirectory();

    return cur;
}

const DirectoryIterator::Entry* DirectoryIterator::operator ->() const
{
    return new Entry(impl->getPath(), impl->getName(), impl->isDirectory());
}

}
