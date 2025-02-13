#include "HierarchyPanel.h"

#include "renderer/ui/imguiwrapper.h"

#include <algorithm>

void HierarchyPanel::init(hk::SceneGraph *scene)
{
    scene_ = scene;

    is_open_ = true;
}

void HierarchyPanel::display()
{
    if (!is_open_) { return; }

    hk::imgui::push([&](){
        if (ImGui::Begin("Scene", &is_open_)) {

            if (ImGui::IsWindowHovered() && ImGui::IsMouseReleased(0)) {
                node_clicked = static_cast<u32>(-1);
                selected_ = nullptr;
            }

            u32 node_idx = 0;

            std::function<void(hk::SceneNode*, u32&)> drawSceneGraph;
            drawSceneGraph = [&](hk::SceneNode *root, u32 &idx){
                for (u32 i = 0; i < root->children.size(); ++i) {
                    hk::SceneNode *child = root->children.at(i);

                    ++idx;

                    // Disable the default "open on single-click behavior" + set
                    // Selected flag according to our selection.
                    //
                    // To alter selection we use
                    // IsItemClicked() && !IsItemToggledOpen(),
                    // so clicking on an arrow doesn't alter selection.
                    ImGuiTreeNodeFlags node_flags =
                        ImGuiTreeNodeFlags_OpenOnArrow |
                        ImGuiTreeNodeFlags_OpenOnDoubleClick;

                    b8 is_leaf = false;
                    if (!child->children.size()) {
                        node_flags = ImGuiTreeNodeFlags_Leaf |
                            ImGuiTreeNodeFlags_NoTreePushOnOpen;
                        is_leaf = true;
                    }

                    if (node_clicked == idx) {
                        node_flags |= ImGuiTreeNodeFlags_Selected;
                    }

                    b8 node_open = (ImGui::TreeNodeEx(child->name.c_str(), node_flags) && !is_leaf);

                    if ((ImGui::IsItemClicked(0) || ImGui::IsItemClicked(1)) && !ImGui::IsItemToggledOpen()) {
                        node_clicked = idx;
                        selected_ = child;
                    }

                    if (ImGui::BeginDragDropSource()) {
                        ImGui::SetDragDropPayload("SCENE_NODE", &child, sizeof(child));
                        ImGui::Text("%s", child->name.c_str());
                        ImGui::EndDragDropSource();
                    }

                    if (ImGui::BeginDragDropTarget()) {
                        const ImGuiPayload *payload;
                        if (!child->object && (payload = ImGui::AcceptDragDropPayload("SCENE_NODE"))) {
                            hk::SceneNode *node = *reinterpret_cast<hk::SceneNode**>(payload->Data);
                            child->children.push_back(node);
                            child->dirty = true;

                            auto it = std::find(node->parent->children.begin(), node->parent->children.end(), node);
                            node->parent->children.erase(it);

                            node->parent = child;
                        }
                        ImGui::EndDragDropTarget();
                    }

                    if (node_open) {
                        drawSceneGraph(child, idx);

                        ImGui::TreePop();
                    }
                }
            };

            drawSceneGraph(scene_->root(), node_idx);

            controls();

        } ImGui::End();
    });
}

void HierarchyPanel::controls()
{
    if (ImGui::IsWindowHovered() && ImGui::IsMouseReleased(1)) {
        ImGui::OpenPopup("ScenePopup", ImGuiPopupFlags_NoOpenOverExistingPopup);
    }
    if (ImGui::BeginPopup("ScenePopup")) {
        if (ImGui::MenuItem("Create Group")) {
            hk::SceneNode node;
            node.name = "New Group";
            node.object = false;

            if (node_clicked > 0) {
                node.parent = selected_->object ? selected_->parent : selected_;
            }

            scene_->addNode(node);
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    auto current_window = ImGui::GetCurrentWindow();
    if (ImGui::BeginDragDropTargetCustom(current_window->InnerRect, current_window->ID)) {
        if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("ASSET_PAYLOAD")) {
            std::string path = reinterpret_cast<const char*>(payload->Data);
            scene_->addModel(hk::assets()->load(path));
        }
        ImGui::EndDragDropTarget();
    }
}
