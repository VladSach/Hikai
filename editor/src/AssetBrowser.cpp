#include "AssetBrowser.h"

#include "renderer/ui/imguiwrapper.h"

#include "utils/thumbnails.h"

void AssetBrowser::init()
{
    is_open_ = true;

    hndlIconD = hk::assets()->load("hk_icon_directory.png");
    hndlIconF = hk::assets()->load("hk_icon_file.png");
    hndlIconB = hk::assets()->load("hk_icon_arrow_back.png");

    iconSize = 128.f;
    iconAlphaNonloaded = 0.5f;

    root_ = hk::assets()->folder();
    curPath_ = root_;

    filter_ = "";
    idxTypeFilter = 0;

    // FIX: tmp, because assets loading in renderer happens before editor init
    for (auto &asset : hk::assets()->assets()) {
        cachedPaths.push_back(asset->path);
    }

    hk::event::subscribe(hk::event::EVENT_ASSET_LOADED,
        [&](const hk::event::EventContext &context, void *listener) {
            (void)listener;

            const u32 handle = context.u32[0];
            std::string path = hk::assets()->getAssetPath(handle);

            cachedPaths.push_back(path);

            hk::assets()->attachCallback(handle, [&](){
                // FIX: restore
                // cachedPaths.at(hk::getIndex(handle)) =
                //     hk::assets()->getAssetPath(handle);
            });
        },
    this);
}

void AssetBrowser::display()
{
    if (!is_open_) { return; }

    hk::imgui::push([&](){
        if (ImGui::Begin("Assets", &is_open_)) {

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
    ImGui::SetNextWindowSizeConstraints(
        ImVec2(ImGui::GetContentRegionAvail().x * 0.15f, 0.f),
        ImVec2(ImGui::GetContentRegionAvail().x * 0.40f, FLT_MAX)
    );

    ImGui::BeginChild("Explorer Pane", ImVec2(0, 0),
                      ImGuiChildFlags_Border |
                      // ImGuiChildFlags_AlwaysUseWindowPadding |
                      ImGuiChildFlags_ResizeX |
                      ImGuiChildFlags_None,
                      ImGuiWindowFlags_None);

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

        u64 filename = asset->path.find_last_of('\\');
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
    ImGui::BeginChild("Control Panel", ImVec2(0, 0),
                      // ImGuiChildFlags_Border  |
                      ImGuiChildFlags_AutoResizeY |
                      ImGuiChildFlags_AlwaysUseWindowPadding |
                      ImGuiChildFlags_None,
                      ImGuiWindowFlags_NoScrollbar |
                      ImGuiWindowFlags_NoResize |
                      ImGuiWindowFlags_None);

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    if (ImGui::ImageButton("Back", hke::thumbnail::get(hndlIconB), {16, 16})) {
        curPath_ = curPath_.substr(0, curPath_.find_last_of('\\'));
    }
    ImGui::PopStyleColor();

    ImGui::SameLine();

    ImGui::InputTextWithHint("##FilterArea", "Filter", filter_.data(), 256);

    ImGui::SameLine();

    constexpr const char *asset_types[] = {
        "None",
        "Shaders",
        "Textures",
        "Meshes",
        "Materials",
        "Models",
    };

    const char *asset_type_preview = (idxTypeFilter) ? asset_types[idxTypeFilter] : "Type";

    if (ImGui::BeginCombo("##AssetTypeFilter", asset_type_preview, ImGuiComboFlags_WidthFitPreview)) {
        for (u32 i = 0; i < IM_ARRAYSIZE(asset_types); ++i) {
            const b8 is_selected = (idxTypeFilter == i);
            if (ImGui::Selectable(asset_types[i], is_selected)) {
                idxTypeFilter = i;
            }

            // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
            if (is_selected) { ImGui::SetItemDefaultFocus(); }
        }
        ImGui::EndCombo();
    }

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    for (auto &subpath : hk::filesystem::split(hk::filesystem::relative(curPath_, root_))) {
        if (ImGui::Button(subpath.c_str())) {
            //curPath_ = hk::filesystem::canonical(subpath);
        }

        ImGui::SameLine();
        ImGui::Text("/");
        ImGui::SameLine();
    }
    ImGui::PopStyleColor();

    // TODO: at to right side buttons "Render directory", "Render only loaded assets"

    ImGui::EndChild();
}

void AssetBrowser::addAssetsPanel()
{
    ImGui::BeginChild("Assets Panel");

    f32 width = ImGui::GetContentRegionAvail().x;

    f32 alpha = 1.f;

    u32 columnCount = static_cast<u32>(width / iconSize);
    if (columnCount < 1) { columnCount = 1; }

    ImGui::Columns(columnCount, 0, false);

    for (auto &entry : hk::filesystem::directory_iterator(curPath_)) {

        auto icon = entry.isDirectory ?
            hke::thumbnail::get(hndlIconD) : hke::thumbnail::get(hndlIconF);

        alpha = entry.isDirectory ? 1.f : iconAlphaNonloaded;

        if (!entry.isDirectory) {
            for (auto &path : cachedPaths) {
                if (entry.path == path) {
                    alpha = 1.f;
                    break;
                }
            }
        }

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
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
        ImGui::PopStyleVar();

        if (!entry.isDirectory && ImGui::BeginDragDropSource()) {
            ImGui::SetDragDropPayload("ASSET_PAYLOAD", entry.path.c_str(), entry.path.size() + 1);
            ImGui::Text("%s", entry.name.c_str());
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

    ImGui::EndChild(); // Asset Panel
}
