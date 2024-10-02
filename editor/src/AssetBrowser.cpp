#include "AssetBrowser.h"

void AssetBrowser::init(GUI &gui)
{
    u32 hndlIconD = hk::assets()->load("hk_icon_directory.png");
    u32 hndlIconF = hk::assets()->load("hk_icon_file.png");
    u32 hndlIconB = hk::assets()->load("hk_icon_arrow_back.png");
    iconD = gui.addTexture(hk::assets()->getTexture(hndlIconD).texture->view());
    iconF = gui.addTexture(hk::assets()->getTexture(hndlIconF).texture->view());
    iconB = gui.addTexture(hk::assets()->getTexture(hndlIconB).texture->view());

    iconSize = 128.f;
    iconAlphaNonloaded = 0.5f;

    root_ = hk::assets()->folder();
    curPath_ = root_;

    filter_ = "";

    // FIX: tmp, because assets loading in renderer happens before editor init
    for (auto &asset : hk::assets()->assets()) {
        cachedPaths.push_back(asset->path);
    }

    hk::evesys()->subscribe(hk::EventCode::EVENT_ASSET_LOADED,
        [&](const hk::EventContext &context, void *listener) {
            const u32 handle = context.u32[0];
            std::string path = hk::assets()->getAssetPath(handle);

            cachedPaths.push_back(path);

            hk::assets()->attachCallback(handle, [&](){
                cachedPaths.at(hk::getIndex(handle)) =
                    hk::assets()->getAssetPath(handle);
            });
        },
    this);
}

void AssetBrowser::display(GUI &gui)
{
    gui.pushCallback([&](){
        if (ImGui::Begin("Assets")) {

            addExplorer();

            ImGui::SameLine();

            ImGui::BeginChild("Assets Browser");

            addControlPanel();

            addAssetsPanel();

            ImGui::EndChild();

        } ImGui::End();

    });
}

void AssetBrowser::addExplorer()
{
    f32 width = ImGui::GetContentRegionAvail().x * 0.15f;

    ImGui::BeginChild("Explorer Pane", ImVec2(width, 0),
                      // ImGuiChildFlags_AlwaysUseWindowPadding |
                      ImGuiChildFlags_Border |
                      ImGuiChildFlags_ResizeX);

    std::function<void(const std::string&)> drawDirectory;
    drawDirectory = [&drawDirectory](const std::string &directory)
    {
        for (auto &entry : hk::filesystem::directory_iterator(directory)) {
            ImGui::PushID(entry.path.c_str());

            if (entry.isDirectory) {
                if (ImGui::TreeNode(entry.name.c_str())) {
                    drawDirectory(entry.path);

                    ImGui::TreePop();
                }
            } else {
                ImGui::TreeNodeEx(entry.name.c_str(),
                                  ImGuiTreeNodeFlags_Leaf |
                                  ImGuiTreeNodeFlags_NoTreePushOnOpen);
            }

            ImGui::PopID();
        }
    };

    drawDirectory(root_);

    ImGui::Separator();

    std::string path;
    std::unordered_map<std::string, hk::vector<hk::Asset*>> bundles;

    for (auto &asset : hk::assets()->assets()) {
        if (asset->path.find(root_) != std::string::npos) { continue; }

        u32 filename = asset->path.find_last_of('\\');
        path = asset->path.substr(0, filename);

        if (bundles.count(path)) {
            bundles[path].push_back(asset);
        } else {
            bundles[path] = hk::vector<hk::Asset*>();
        }
    }

    ImGui::PushStyleColor(ImGuiCol_Text, {0.f, 1.f, 0.f, 1.f});
    for (auto &bundle : bundles) {
        if (ImGui::TreeNodeEx(bundle.first.c_str(), ImGuiTreeNodeFlags_AllowItemOverlap)) {
            for (auto &asset : bundle.second) {
                ImGui::PushID(asset->path.c_str());
                ImGui::TreeNodeEx(asset->name.c_str(), ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
                ImGui::PopID();
            }

            ImGui::TreePop();
        }
    }
    ImGui::PopStyleColor();

    ImGui::EndChild();
}

void AssetBrowser::addControlPanel()
{
    f32 height = ImGui::GetContentRegionAvail().y * 0.15f;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0, height * 0.25f});

    ImGui::BeginChild("Control Panel", ImVec2(0, height),
                      // ImGuiChildFlags_Border  |
                      // ImGuiChildFlags_ResizeY |
                      ImGuiWindowFlags_NoScrollbar |
                      ImGuiChildFlags_None);

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    if (ImGui::ImageButton("Back", iconB, {16, 16})) {
        curPath_ = curPath_.substr(0, curPath_.find_last_of('\\'));
    }
    ImGui::PopStyleColor();

    ImGui::SameLine();

    ImGui::InputTextWithHint("##FilterArea", "Filter", filter_.data(), 256);

    ImGui::EndChild();

    ImGui::PopStyleVar();
}

void AssetBrowser::addAssetsPanel()
{
    ImGui::BeginChild("Assets Panel");

    f32 width = ImGui::GetContentRegionAvail().x;

    void* icon = nullptr;
    f32 alpha = 1.f;

    u32 columnCount = width / iconSize;
    if (columnCount < 1) { columnCount = 1; }

    ImGui::Columns(columnCount, 0, false);

    for (auto &entry : hk::filesystem::directory_iterator(curPath_)) {

        icon = entry.isDirectory ? iconD : iconF;

        alpha = entry.isDirectory ? 1.f : iconAlphaNonloaded;

        if (!entry.isDirectory) {
            for (auto &path : cachedPaths) {
                if (entry.path == path) {
                    alpha = 1.f;
                    break;
                }
            }
        }

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        if (ImGui::ImageButton(entry.name.c_str(), icon, {iconSize, iconSize},
                               {0, 0}, {1, 1}, {0, 0, 0, 0},
                               {1.f, 1.f, 1.f, alpha}))
        {
            if (entry.isDirectory) {
                curPath_ = entry.path;
                // if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                //         cur = entry.path;
                // }
            }
        }
        ImGui::PopStyleColor();

        if (!entry.isDirectory && ImGui::BeginDragDropSource()) {
            ImGui::SetDragDropPayload("ASSET_PAYLOAD", entry.path.c_str(), entry.path.size());
            ImGui::Text("%s", entry.path.c_str());
            ImGui::EndDragDropSource();
        }

        ImGui::TextWrapped("%s", entry.name.c_str());

        ImGui::NextColumn();
    }

    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows)) {
        if (hk::input::isKeyDown(hk::input::Button::KEY_LCTRL)) {
            iconSize += hk::input::getMouseWheelDelta() * 16.f;
            iconSize = hkm::clamp(iconSize, 16.f, 512.f);
        }
    }

    ImGui::EndChild();
}
