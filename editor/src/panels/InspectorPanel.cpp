#include "InspectorPanel.h"

#include "renderer/ui/imguiwrapper.h"

#include "../utils/thumbnails.h"

void InspectorPanel::init()
{
    is_open_ = true;

    hk::event::subscribe(hk::event::EVENT_ASSET_LOADED,
        [&](const hk::event::EventContext &context, void *listener) {
            (void)listener;

            const u32 handle = context.u32[0];
            const hk::Asset::Type type = static_cast<hk::Asset::Type>(context.u32[1]);

            if (type != hk::Asset::Type::MATERIAL) { return; }

            materials.push_back(&hk::assets()->getMaterial(handle));
        },
    this);
}

void InspectorPanel::display(hk::SceneNode *node)
{
    if (!is_open_) { return; }

    hk::imgui::push([&, node](){
        if (ImGui::Begin("Inspector", &is_open_)) {
            if (node) {
                addBasicAssetProperties(node);
                addTransform(node);

                if (node->object) {
                    hk::MeshAsset &mesh = hk::assets()->getMesh(node->entity->hndlMesh);
                    addMeshInfo(mesh);

                    addMaterialInfo(node);
                }
            }

        } ImGui::End();
    });
}

void InspectorPanel::addBasicAssetProperties(hk::SceneNode *node)
{
    char buffer[256] = {};
    strcpy_s(buffer, node->name.c_str());
    if (ImGui::InputTextWithHint("##AssetName", "Name", buffer, 256,
                                 ImGuiInputTextFlags_EnterReturnsTrue))
    {
        if (buffer[0] != '\0') { node->name = buffer; }
    }

    ImGui::Text("Children: %d", node->children.size());
    ImGui::Text("Index: %d", node->idx);
    ImGui::Checkbox("Object", &node->object);
}

void InspectorPanel::addMeshInfo(const hk::MeshAsset &mesh)
{
    if (ImGui::CollapsingHeader("Info", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Vertices: %d",  mesh.mesh.vertices.size());
        ImGui::Text("Indices: %d",   mesh.mesh.indices.size());
        ImGui::Text("Instances: %d", mesh.instances.size());
    }
}

void InspectorPanel::addTransform(hk::SceneNode *node)
{
    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {

        Transform tr = node->local;

        b8 pos = false;
        b8 scale = false;
        b8 rota = false;

        ImGuiTableFlags flags = ImGuiTableFlags_None;
        if (ImGui::BeginTable("##Transform", 2, flags)) {
            // ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthStretch, 1.f);
            // ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 1.f);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Position");
            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(-1);
            pos = ImGui::DragFloat3("##Position", &tr.pos[0], .1f);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Scale");
            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(-1);
            scale = ImGui::DragFloat3("##Scale", &tr.scale[0], .1f, 0.001f, 100.f);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Rotation");
            ImGui::TableSetColumnIndex(1);
            hkm::vec3f rot = hkm::toEulerAngles(tr.rotation) * hkm::rad2degree;
            ImGui::SetNextItemWidth(-1);
            rota = ImGui::DragFloat3("##Rotation", &rot[0], .1f);
            tr.rotation = hkm::fromEulerAngles(rot * hkm::degree2rad);

            ImGui::EndTable();
        }

        // b8 pos   = ImGui::DragFloat3("Position", &tr.pos[0], .1f);
        // b8 scale = ImGui::DragFloat3("Scale",    &tr.scale[0], .1f, 0.001f, 100.f);
        // hkm::vec3f rot = hkm::toEulerAngles(tr.rotation) * hkm::rad2degree;
        // b8 rota = ImGui::DragFloat3("Rotation", &rot[0], .1f);
        // tr.rotation = hkm::fromEulerAngles(rot * hkm::degree2rad);

        if (pos || scale || rota) {
            node->local = tr;
            node->dirty = true;
        }

        // FIX: debug
        if (ImGui::TreeNodeEx("WorldTransform")) {
            hk::imgui::utils::drawMatrix4x4(node->world.toMat4f());
            ImGui::TreePop();
        }

        if (ImGui::Button("Reset")) {
            node->local = node->loaded;
            node->dirty = true;
        }
    }
}

void InspectorPanel::addMaterialInfo(hk::SceneNode *node)
{
    constexpr f32 combo_height = 40.f;
    static u32 idx = 0;

    if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen)) {

        b8 change_material = false;

        // https://github.com/ocornut/imgui/issues/5963
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 20.f));
        if (ImGui::BeginCombo("##MaterialCombo", "", ImGuiComboFlags_NoArrowButton | ImGuiComboFlags_CustomPreview)) {
            for (u32 i = 0; i < materials.size(); ++i) {
                ImGui::Image(hke::thumbnail::get(materials.at(i)->data.hndlDiffuse), {40.f, combo_height});

                ImGui::SameLine();

                const b8 is_selected = (idx == i);
                if (ImGui::Selectable(materials.at(i)->name.c_str(), is_selected, 0, ImVec2(0, combo_height))) {
                    idx = i;
                    change_material = true;
                }

                if (is_selected) { ImGui::SetItemDefaultFocus(); }
            }

            ImGui::EndCombo();
        }
        ImGui::PopStyleVar();

        if (change_material) {
            node->entity->attachMaterial(materials.at(idx)->handle);
            node->dirty = true;
        }

        hk::MaterialAsset &material = hk::assets()->getMaterial(node->entity->hndlMaterial);

        if (ImGui::BeginComboPreview()) {
            ImGui::Image(hke::thumbnail::get(material.data.hndlDiffuse), {40.f, 40.f});
            ImGui::Text("%s", material.name.c_str());
            ImGui::EndComboPreview();
        }

        if (ImGui::Button("Create")) {
            hk::MaterialAsset *mat = new hk::MaterialAsset();
            mat->name = "New Material";

            node->entity->attachMaterial(hk::assets()->create(hk::Asset::Type::MATERIAL, mat));
            node->dirty = true;
        }

        // Toggles
        if (ImGui::TreeNodeEx("Constants", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::ColorEdit4("Color", &material.data.constants.color.x);

            ImGui::InputFloat("Alpha",        &material.data.constants.alpha,        .0f, 1.f, "%.3f");
            ImGui::InputFloat("Shininess",    &material.data.constants.shininess,    .0f, 1.f, "%.3f");
            ImGui::InputFloat("Reflectivity", &material.data.constants.reflectivity, .0f, 1.f, "%.3f");
            ImGui::TreePop();
        }

        if (ImGui::TreeNodeEx("Diffuse", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Image(hke::thumbnail::get(material.data.hndlDiffuse), {100.f, 100.f});

            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PAYLOAD")) {
                    std::string path = reinterpret_cast<const char*>(payload->Data);
                    material.data.hndlDiffuse = hk::assets()->load(path);
                }
                ImGui::EndDragDropTarget();
            }

            ImGui::TreePop();
        }

        if (ImGui::TreeNodeEx("Normal", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (!material.data.hndlNormal) {
                ImGui::Text("No Texture");
            } else {
                ImGui::Image(hke::thumbnail::get(material.data.hndlNormal), {100.f, 100.f});
            }


            ImGui::TreePop();
        }

        if (ImGui::TreeNodeEx("Shaders", ImGuiTreeNodeFlags_DefaultOpen)) {
            hk::ShaderAsset &vs = hk::assets()->getShader(material.data.hndlVS);
            hk::ShaderAsset &ps = hk::assets()->getShader(material.data.hndlPS);

            ImGui::Text("%s", vs.name.c_str());
            ImGui::Text("%s", ps.name.c_str());

            ImGui::TreePop();
        }
    }

}
