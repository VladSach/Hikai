#include "platform/platform.h"

#ifdef HKLINUX

#include "filesystem.h"

#include <sys/dir.h>

namespace hk::platform {

b8 find_file(const hk::string &root, const hk::string &target, hk::string *out)
{
    std::string search_path(root);

    // Remove trailing and '\' and '/'
    while (!search_path.empty() &&
           (search_path.back() == '\\' || search_path.back() == L'/'))
    {
        search_path.pop_back();
    }

    search_path += "\\*";

    // FIX: ? a little quick fix, maybe should change this
    if (search_path.find(target) != std::string::npos) {
        if (out) { *out = root; }
        return true;
    }

    DIR *dir = opendir(search_path.c_str());

    if (!dir) { return false; }

    // every call of readdir() yields the next non-read entry
    dirent *entry = readdir(dir);

    while (entry) {
        if (!strcmp(entry->d_name, target.c_str())) {
            if (out) { *out = root + '\\' + entry->d_name; }
            // LOG_DEBUG("File found at:", out);
            closedir(dir);
            return true;
        }

        // if type(entry) == DIR
        if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")) {
            hk::string sub_dir(search_path.substr(0, search_path.size() - 1));
            sub_dir += entry->d_name;

            if (find_file(sub_dir, target, out)) { return true; }
        }

        entry = readdir(dir);
    }

    closedir(dir);

    return false;

}

b8 exists(const std::string &path)
{
    return false;
}

// Converts path to weakly canonical absolute path
std::string canonical(const std::string &path)
{

}

// Returns relative path from base to path
std::string relative(const std::string &path, const std::string &base)
{

}

}

#endif // HKLINUX
