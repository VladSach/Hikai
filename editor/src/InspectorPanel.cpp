#include "InspectorPanel.h"

void InspectorPanel::init(GUI *gui)
{
    this->gui = gui;

    // std::string path = hk::filesystem::canonical("C:\\Users\\Kyonari\\Projects\\Hikai\\editor\\assets\\textures\\prototype\\PNG\\Light\\texture_06.png");
    // diffuseThumbnail = *hk::assets()->getTexture(hk::assets()->load(path)).texture;
    // gui->addTexture(diffuseThumbnail.view());
}

void InspectorPanel::display(hk::SceneNode *node)
{
    gui->pushCallback([&, node](){
        if (ImGui::Begin("Inspector")) {
            if (node) {
                addBasicAssetProperties(node);
                addTransform(node);

                if (!node->object && node->handle) {
                    hk::Asset *asset = hk::assets()->get(node->handle);
                    hk::ModelAsset *model = reinterpret_cast<hk::ModelAsset*>(asset);
                } else if (node->object) {
                    hk::MeshAsset &mesh = hk::assets()->getMesh(node->entity->hndlMesh);
                    addMeshInfo(mesh);

                    addMaterials(hk::assets()->getMaterial(node->entity->hndlMaterial).material);
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

        b8 pos   = ImGui::DragFloat3("Position", &tr.pos[0], .1f);
        b8 scale = ImGui::DragFloat3("Scale",    &tr.scale[0], .1f, 0.001f, 100.f);
        hkm::vec3f rot = hkm::toEulerAngles(tr.rotation) * hkm::rad2degree;
        b8 rota = ImGui::DragFloat3("Rotation", &rot[0], .1f);
        tr.rotation = hkm::fromEulerAngles(rot * hkm::degree2rad);

        if (pos || scale || rota) {
            node->local = tr;
            node->dirty = true;
        }

        // FIX: debug
        if (ImGui::TreeNodeEx("WorldTransform")) {
            drawMatrix4x4(node->world.toMat4f());
            ImGui::TreePop();
        }

        if (ImGui::Button("Reset")) {
            node->local = node->loaded;
            node->dirty = true;
        }
    }
}

void InspectorPanel::addMaterials(hk::Material *material)
{
    if (ImGui::CollapsingHeader("Materials", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::TreeNodeEx("Diffuse", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (!material->diffuse->set_) {
                gui->addTexture(material->diffuse);
            }

            ImGui::Image(material->diffuse->set_, {100.f, 100.f});

            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PAYLOAD")) {
                    std::string path = reinterpret_cast<const char*>(payload->Data);
                    material->diffuse = hk::assets()->getTexture(hk::assets()->load(path)).texture;
                }
                ImGui::EndDragDropTarget();
            }

            ImGui::TreePop();
        }
    }

}
