#include "InspectorPanel.h"

void InspectorPanel::init(GUI *gui)
{
    this->gui = gui;
    // hndlSelectedModel = 0;
}

void InspectorPanel::display(u32 selected)
{
    gui->pushCallback([&](){
        if (ImGui::Begin("Inspector")) {

            if (selected) {
                hk::ModelAsset asset = hk::assets()->getModel(selected);
                addTransform(asset.model->transform_);
                addMaterials(asset.model);
            }

        } ImGui::End();
    });
}

void InspectorPanel::addTransform(Transform &tr)
{
    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::DragFloat3("Position", &tr.pos[0]);
        ImGui::DragFloat3("Scale", &tr.scale[0]);
    }
}

void InspectorPanel::addMaterials(hk::Model *model)
{
    if (ImGui::CollapsingHeader("Materials", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::TreeNodeEx("Diffuse", ImGuiTreeNodeFlags_DefaultOpen)) {
            static auto im = gui->addTexture(model->diffuse->view());
            ImGui::Image(im, {100.f, 100.f});

            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PAYLOAD")) {
                    std::string path = reinterpret_cast<const char*>(payload->Data);
                    LOG_DEBUG("Path recivied:", path);
                    model->diffuse = hk::assets()->getTexture(hk::assets()->load(path)).texture;
                }
                ImGui::EndDragDropTarget();
            }

            ImGui::TreePop();
        }
    }

}
