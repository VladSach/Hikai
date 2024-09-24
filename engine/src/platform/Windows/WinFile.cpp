#include "WinFile.h"

#include "platform/platform.h"

#include <fstream>
#include <string>

namespace hk::platform {

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

}
