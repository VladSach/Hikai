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
    u32 hndlIconD;
    u32 hndlIconF;
    u32 hndlIconB;

    f32 iconSize;
    f32 iconAlphaNonloaded;

    std::string root_;
    std::string curPath_;

    hk::vector<std::string> cachedPaths;

    // Control Panel
    std::string filter_ = "";
    u32 idxTypeFilter = 0;
};

#endif // HK_ASSET_BROWSER_H
