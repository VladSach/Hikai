#ifndef HK_ASSET_BROWSER_H
#define HK_ASSET_BROWSER_H

#include "hikai.h"

class AssetBrowser {
public:
    void init(GUI &gui);

    void display(GUI &gui);

private:
    void addExplorer();
    void addControlPanel();
    void addAssetsPanel();

private:
    void* iconD = nullptr;
    void* iconF = nullptr;
    void* iconB = nullptr;

    f32 iconSize;
    f32 iconAlphaNonloaded;

    std::string root_;
    std::string curPath_;

    std::string filter_ = "";

    hk::vector<std::string> cachedPaths;
};

#endif // HK_ASSET_BROWSER_H
