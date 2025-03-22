#include "imguidebug.h"

#include "resources/AssetManager.h"

namespace hk::imgui::debug {

void push(const hk::Pipeline &pipeline)
{
    hk::imgui::push([=](){
        if (ImGui::Begin("Pipeline Info")) {
            const hk::Pipeline::Info &info = pipeline.info_;

            if (ImGui::TreeNode(info.name.c_str())) {
                if (ImGui::TreeNode("Layout Info")) {
                    ImGui::Text("Descriptor Sets: %i", info.desc_layouts.size());

                    ImGui::Text("Push Constants: %i", info.push_ranges.size());
                    if (info.push_ranges.size() && ImGui::TreeNode("Push Ranges")) {
                        for (auto &range : info.push_ranges) {
                            ImGui::Text("Push Offset: %u", range.offset);
                            ImGui::Text("Push Size: %u",   range.size);
                            ImGui::Text("Push Stage: %u",  range.stageFlags);
                        }
                        ImGui::TreePop();
                    }

                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("Shader Stages")) {
                    constexpr char const *stages[] = {
                        "none",

                        "Vertex",
                        "Hull",
                        "Domain",
                        "Geometry",
                        "Pixel",
                        "Compute"
                    };
                    for (auto &stage : info.shaders) {
                        if (!ImGui::TreeNode(stages[stage.first])) { continue; }

                        const auto &shader = hk::assets()->getShader(stage.second);

                        ImGui::Text("Name: %s", shader.name.c_str());
                        ImGui::Text("Entry Point: %s", shader.desc.entry.c_str());

                        ImGui::TreePop();
                    }

                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("Vertex Layout")) {

                    ImGui::TreePop();
                }

                ImGui::TreePop();
            }

        } ImGui::End();
    });
}

}
