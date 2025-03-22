#ifndef HK_FILESYSTEM_H
#define HK_FILESYSTEM_H

#include "hkcommon.h"
#include "Windows/win.h"

#include "hkstl/containers/hkvector.h"

namespace hk::filesystem {

HKAPI b8 readFile(const std::string &path, hk::vector<u8>& out);

HKAPI b8 findFile(const std::string &root, const std::string &target,
                  std::string *out = nullptr);

HKAPI b8 exists(const std::string &path);

// Converts path to weakly canonical absolute path
HKAPI std::string canonical(const std::string &path);

// Returns relative path from base to path
HKAPI std::string relative(const std::string &path, const std::string &base);

HKAPI hk::vector<std::string> split(const std::string &path);



class DirectoryIterator {
public:
    HKAPI DirectoryIterator(const std::string& path);
    // DirectoryIterator(const DirectoryIterator&) = delete;
    // DirectoryIterator& operator =(const DirectoryIterator&) = delete;
    HKAPI ~DirectoryIterator();

    HKAPI b8 operator !=(const DirectoryIterator&) const;
    HKAPI DirectoryIterator& operator ++();

    struct Entry {
        std::string path = "";
        std::string name = "";
        b8 isDirectory = false;

        Entry() = default;
        Entry(const std::string& p, const std::string& n, b8 d) :
            path(p), name(n), isDirectory(d)
        {}
    };

    HKAPI const Entry& operator *();
    HKAPI const Entry* operator ->() const;

private:
    struct Impl;
    Impl* impl;

    Entry cur;
};

class directory_iterator {
private:
    std::string startPath;
    std::string endPath;

public:
    directory_iterator(const std::string& path) :
        startPath(path), endPath(path + "\\" + "*")
    {}

    DirectoryIterator begin() const {
        return DirectoryIterator(startPath);
    }

    DirectoryIterator end() const {
        return DirectoryIterator(endPath);
    }
};

}

#endif // HK_FILESYSTEM_H
