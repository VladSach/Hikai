#include "platform/filesystem.h"

#include "win.h"

#include <fstream>
#include <string>
#include <filesystem>

namespace hk::filesystem {

b8 readFile(const std::string &path, hk::vector<u8> &out)
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

b8 findFile(const std::string &root, const std::string &target,
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

            if (findFile(subDir, target, out)) { return true; }
        }

    } while (FindNextFile(hFind, &data));

    FindClose(hFind);
    return false;
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
