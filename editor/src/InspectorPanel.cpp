#include "InspectorPanel.h"

void InspectorPanel::init(GUI *gui)
{
    this->gui = gui;

    // std::string path = hk::filesystem::canonical("C:\\Users\\Kyonari\\Projects\\Hikai\\editor\\assets\\textures\\prototype\\PNG\\Light\\texture_06.png");
    // diffuseThumbnail = *hk::assets()->getTexture(hk::assets()->load(path)).texture;
    // gui->addTexture(diffuseThumbnail.view());
}

void InspectorPanel::display(u32 selected)
{
    gui->pushCallback([&, selected](){
        if (ImGui::Begin("Inspector")) {
            if (selected) {
                hk::Asset *asset = hk::assets()->get(selected);
                addBasicAssetProperties(asset);

                switch (asset->type) {
                case hk::Asset::Type::MODEL: {
                    hk::ModelAsset *model = reinterpret_cast<hk::ModelAsset*>(asset);
                    addTransform(model->model->transform_);
                    addMaterials(model->model);
                } break;
                case hk::Asset::Type::MESH: {

                } break;
                defaul:
                    LOG_ERROR("Unknown asset type selected");
                }
            }

        } ImGui::End();
    });
}

void InspectorPanel::addBasicAssetProperties(hk::Asset *asset)
{
    char buffer[256] = {};
    strcpy_s(buffer, asset->name.c_str());
    if (ImGui::InputTextWithHint("##AssetName", "Name", buffer, 256,
                                 ImGuiInputTextFlags_EnterReturnsTrue))
    {
        if (buffer[0] != '\0') { asset->name = buffer; }
    }
}

void InspectorPanel::addTransform(Transform &tr)
{
    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
        // https://github.com/CedricGuillemet/ImGuizmo/issues/294
        f32 matrixTranslation[3], matrixRotation[3], matrixScale[3];
        ImGuizmo::DecomposeMatrixToComponents(tr.toMat4f().n[0],
                                              matrixTranslation,
                                              matrixRotation,
                                              matrixScale);

        ImGui::DragFloat3("Position", matrixTranslation, .1f);
        ImGui::DragFloat3("Scale",    matrixScale, .1f, 0.001f, 100.f);
        ImGui::DragFloat3("Rotation", matrixRotation, .1f);

        hkm::mat4f matrix;
        ImGuizmo::RecomposeMatrixFromComponents(matrixTranslation,
                                                matrixRotation,
                                                matrixScale, matrix.n[0]);

        tr = Transform(matrix);

        // ImGui::DragFloat3("Position", &tr.pos[0], .1f);
        // ImGui::DragFloat3("Scale",    &tr.scale[0], .1f, 0.001f, 100.f);
        // hkm::vec3f rot = hkm::toEulerAngles(tr.rotation);
        // rot *= hkm::rad2degree;
        // ImGui::DragFloat3("Rotation", &rot[0], .1f);
        // f64 rotat[3] = {rot.x, rot.y, rot.z};
        // ImGui::DragScalarN("Rotation", ImGuiDataType_Double, rotat, 3, .1f);
        // tr.rotation = hkm::fromEulerAngles(rot * hkm::degree2rad);
    }
}

void InspectorPanel::addMaterials(hk::Model *model)
{
    if (ImGui::CollapsingHeader("Materials", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::TreeNodeEx("Diffuse", ImGuiTreeNodeFlags_DefaultOpen)) {
            hk::Material *material = hk::assets()->getMaterial(model->hndlMaterial).material;
            static void* thumbnail = gui->addTexture(material->diffuse->view());

            ImGui::Image(thumbnail, {100.f, 100.f});

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
