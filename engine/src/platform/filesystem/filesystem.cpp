#include "filesystem.h"

#include <fstream>
#include <filesystem>

namespace hk::platform {

b8 read_file(const hk::string &path, hk::vector<u8> &out)
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

}
