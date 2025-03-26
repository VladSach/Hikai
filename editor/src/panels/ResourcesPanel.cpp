#include "ResourcesPanel.h"

#include <algorithm>

void ResourcesPanel::init(Renderer *renderer)
{
    is_open_ = true;

    renderer_ = renderer;
}

void ResourcesPanel::deinit()
{
}

void ResourcesPanel::display()
{
    if (!is_open_) { return; }

    hk::imgui::push([&](){
        if (ImGui::Begin("Resources", &is_open_)) {

            ImGuiTableFlags flags =
                ImGuiTableFlags_Resizable |
                // ImGuiTableFlags_Sortable |
                ImGuiTableFlags_RowBg |
                ImGuiTableFlags_ScrollY |
                ImGuiTableFlags_None;

            if (ImGui::TreeNode("Buffers")) {

                if (ImGui::BeginTable("##Buffers", 6, flags)) {
                    ImGui::TableSetupColumn("Handle");
                    ImGui::TableSetupColumn("Name");
                    ImGui::TableSetupColumn("Type");
                    ImGui::TableSetupColumn("Access");
                    ImGui::TableSetupColumn("Size");
                    ImGui::TableSetupColumn("Stride");

                    ImGui::TableSetupScrollFreeze(0, 1);
                    ImGui::TableHeadersRow();

                    auto &metadata = hk::bkr::metadata();
                    auto &descriptors = hk::bkr::descriptors();

                    // ImGuiTableSortSpecs* specs = ImGui::TableGetSortSpecs();
                    // if (specs && specs->SpecsDirty) {
                    //     u32 column = specs->Specs[0].ColumnIndex;
                    //
                    //     if (column < 2) {
                    //         std::sort(metadata.begin(), metadata.end(),
                    //             [&](const hk::ResourceMetadata &lhs, const hk::ResourceMetadata &rhs)
                    //         {
                    //             b8 is_ascending = specs->Specs[0].SortDirection == ImGuiSortDirection_Ascending;
                    //
                    //             switch(column) {
                    //             case 0: return is_ascending ?
                    //                   lhs.handle.index < rhs.handle.index :
                    //                   lhs.handle.index > rhs.handle.index;
                    //             case 1: return is_ascending ?
                    //                   lhs.name < rhs.name :
                    //                   lhs.name > rhs.name;
                    //             }
                    //
                    //             return false;
                    //         });
                    //     }
                    //
                    //     // std::sort(descriptors.begin(), descriptors.end(),
                    //     //     [&](const ) {
                    //     //     
                    //     //
                    //     // });
                    //
                    //     specs->SpecsDirty = false;
                    // }

                    for (u32 i = 0; i < hk::bkr::size(); ++i) {
                        const auto &meta = metadata.at(i);
                        const auto &desc = descriptors.at(i);

                        ImGui::TableNextRow();

                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("%u : %u", meta.handle.index, meta.handle.gen);

                        ImGui::TableSetColumnIndex(1);
                        ImGui::Text("%s", meta.name.c_str());

                        ImGui::TableSetColumnIndex(2);
                        ImGui::Text("%s", to_string(desc.type));

                        ImGui::TableSetColumnIndex(3);
                        ImGui::Text("%s", to_string(desc.access));

                        ImGui::TableSetColumnIndex(4);
                        ImGui::Text("%u", desc.size);

                        ImGui::TableSetColumnIndex(5);
                        ImGui::Text("%u bit", desc.stride);
                    }

                    ImGui::EndTable();
                }

                ImGui::TreePop();
            }

            if (ImGui::TreeNode("Images")) {

                if (ImGui::BeginTable("##Images", 6, flags)) {
                    ImGui::TableSetupColumn("Handle");
                    ImGui::TableSetupColumn("Name");
                    ImGui::TableSetupColumn("Type");
                    ImGui::TableSetupColumn("Format");
                    ImGui::TableSetupColumn("Size");
                    ImGui::TableSetupColumn("Preview");

                    ImGui::TableSetupScrollFreeze(0, 1);
                    ImGui::TableHeadersRow();

                    auto &metadata = hk::bkr::image_metadata();
                    auto &descriptors = hk::bkr::image_descriptors();

                    for (u32 i = 0; i < hk::bkr::image_size(); ++i) {
                        const auto &meta = metadata.at(i);
                        const auto &desc = descriptors.at(i);

                        ImGui::TableNextRow();

                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("%u : %u", meta.handle.index, meta.handle.gen);

                        ImGui::TableSetColumnIndex(1);
                        ImGui::Text("%s", meta.name.c_str());

                        ImGui::TableSetColumnIndex(2);
                        ImGui::Text("%s", to_string(desc.type));

                        ImGui::TableSetColumnIndex(3);
                        ImGui::Text("%s", to_string(desc.format));

                        ImGui::TableSetColumnIndex(4);
                        ImGui::Text("%u x %u x %u", desc.width, desc.height, desc.channels);

                        ImGui::TableSetColumnIndex(5);
                        ImGui::Text("*");

                        if (desc.type != hk::ImageType::TEXTURE) { continue; }

                        if (ImGui::IsItemHovered()) {
                            ImGui::BeginTooltip();

                            hk::ImageHandle handle;
                            handle.value = meta.handle.value;
                            hk::imgui::Image(handle, renderer_->samplers_.linear.repeat, { 255, 255 });

                            ImGui::EndTooltip();
                        }
                    }

                    ImGui::EndTable();
                }

                ImGui::TreePop();
            }

            if (ImGui::TreeNode("Assets")) {
                if (ImGui::BeginTable("##Resources", 3, flags)) {
                    ImGui::TableSetupColumn("ID");
                    ImGui::TableSetupColumn("Type");
                    ImGui::TableSetupColumn("Name");

                    ImGui::TableSetupScrollFreeze(0, 1);
                    ImGui::TableHeadersRow();

                    // ImGui::TableGetSortSpecs();

                    for (const auto &asset : hk::assets()->assets()) {
                        ImGui::TableNextRow();

                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("%u", hk::getIndex(asset->handle));

                        ImGui::TableSetColumnIndex(1);
                        ImGui::Text("%s", hk::asset::to_string(asset->type));

                        ImGui::TableSetColumnIndex(2);
                        ImGui::Text("%s", asset->name.c_str());
                    }

                    ImGui::EndTable();
                }
                ImGui::TreePop();
            }

        } ImGui::End();
    });
}

