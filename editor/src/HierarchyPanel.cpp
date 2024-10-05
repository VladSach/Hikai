#include "HierarchyPanel.h"

void HierarchyPanel::init()
{
    hndlSelectedAsset = 0;

    // FIX: tmp, don't have a scene graph yet
    hk::evesys()->subscribe(hk::EventCode::EVENT_ASSET_LOADED,
        [&](const hk::EventContext &context, void *listener) {
            const u32 handle = context.u32[0];
            const hk::Asset::Type type = static_cast<hk::Asset::Type>(context.u32[1]);

            if (type != hk::Asset::Type::MODEL) { return; }

            models.push_back(&hk::assets()->getModel(handle));
        },
    this);
}

void HierarchyPanel::display(GUI &gui)
{
    gui.pushCallback([&](){
        if (ImGui::Begin("Scene")) {

            static i32 selection_mask = (1 << 2);
            i32 node_clicked = -1;

            for (i32 i = 0; i < models.size(); ++i) {
                hk::ModelAsset &asset = *models[i];

                // Disable the default "open on single-click behavior" + set
                // Selected flag according to our selection.
                //
                // To alter selection we use
                // IsItemClicked() && !IsItemToggledOpen(),
                // so clicking on an arrow doesn't alter selection.
                ImGuiTreeNodeFlags node_flags =
                    ImGuiTreeNodeFlags_OpenOnArrow |
                    ImGuiTreeNodeFlags_OpenOnDoubleClick;

                const b8 is_selected = (selection_mask & (1 << i)) != 0;

                if (is_selected) {
                    node_flags |= ImGuiTreeNodeFlags_Selected;
                }

                // The only reason we use TreeNode at all is to allow selection of the leaf. Otherwise we can
                // use BulletText() or advance the cursor by GetTreeNodeToLabelSpacing() and call Text().
                //node_flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
                b8 node_open = ImGui::TreeNodeEx(asset.name.c_str(), node_flags);

                if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
                    node_clicked = i;
                    hndlSelectedAsset = asset.handle;
                }

                if (ImGui::BeginDragDropSource()) {
                    ImGui::SetDragDropPayload("_TREENODE", NULL, 0);
                    ImGui::Text("Just a test");
                    ImGui::EndDragDropSource();
                }

                if (node_open) {

                    std::function<void(hk::MeshAsset *root)> loadMeshes;
                    loadMeshes = [&](hk::MeshAsset *root){
                        ImGuiTreeNodeFlags node_flags =
                            ImGuiTreeNodeFlags_OpenOnArrow |
                            ImGuiTreeNodeFlags_OpenOnDoubleClick;
                        b8 is_leaf = false;

                        if (!root->children.size()) {
                            node_flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
                            is_leaf = true;
                        }

                        b8 node_open = (ImGui::TreeNodeEx(root->name.c_str(), node_flags) && !is_leaf);

                        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
                            hndlSelectedAsset = root->handle;
                        }

                        if (node_open) {
                            for (u32 i = 0; i < root->children.size(); ++i) {
                                hk::MeshAsset *child = root->children.at(i);
                                loadMeshes(child);
                            }

                            ImGui::TreePop();
                        }
                    };

                    loadMeshes(&hk::assets()->getMesh(asset.model->hndlRootMesh));

                    ImGui::TreePop();
                }
            }

            if (node_clicked != -1) {
                // Update selection state
                // (process outside of tree loop to avoid visual inconsistencies during the clicking frame)
                if (ImGui::GetIO().KeyCtrl)
                    selection_mask ^= (1 << node_clicked);          // CTRL+click to toggle
                else //if (!(selection_mask & (1 << node_clicked))) // Depending on selection behavior you want, may want to preserve selection when clicking on item that is part of the selection
                    selection_mask = (1 << node_clicked);           // Click to single-select
            }

        } ImGui::End();
    });
}
