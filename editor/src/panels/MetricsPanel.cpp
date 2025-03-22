#include "MetricsPanel.h"

#include "platform/info.h"

void MetricsPanel::init()
{
    is_open_ = false;

    // FIX: temp
    hk::platform::updateCpuInfo();
}

void MetricsPanel::deinit()
{
}

void MetricsPanel::display()
{
    if (!is_open_) { return; }

    hk::imgui::push([&](){
        if (ImGui::Begin("Metrics", &is_open_)) {

            addLogMetrics();
            addMonitorInfo();
            addCPUInfo();
            addGPUInfo();

        } ImGui::End();
    });
}

void MetricsPanel::addLogMetrics()
{
    if (ImGui::CollapsingHeader("Logs")) {
        hk::log::DebugInfo log = hk::log::getDebugInfo();

        ImGui::Text("Logs Issued: %d", log.logsIssued);
    }
}

void MetricsPanel::addGPUInfo()
{
    if (ImGui::CollapsingHeader("Physical Device")) {
        const auto &infos = hk::context()->allPhysicalInfos();

        for (u32 i = 0; i < infos.size(); ++i) {
            auto &device = infos.at(i);
            auto &properties = device.properties;
            if (ImGui::TreeNode(properties.deviceName)) {
                ImGui::Text("Type: %s", hk::vkDeviceTypeToString(properties.deviceType).c_str());
                ImGui::Text("API Version: %s", hk::vkApiToString(properties.apiVersion).c_str());
                ImGui::Text("Driver Version: %s", hk::vkDeviceDriveToString(properties.driverVersion, properties.vendorID).c_str());
                ImGui::Text("Vendor: %s", hk::vkDeviceVendorToString(properties.vendorID).c_str());
                // ImGui::Text("Device ID: %d", device.deviceID);

                if (ImGui::TreeNode("Queue Families")) {
                    ImGuiTableFlags flags = ImGuiTableFlags_RowBg;
                    flags |= ImGuiTableFlags_BordersOuter;
                    if (ImGui::BeginTable("Families", 6, flags)) {
                        ImGui::TableSetupColumn("Index");
                        ImGui::TableSetupColumn("Graphics");
                        ImGui::TableSetupColumn("Compute");
                        ImGui::TableSetupColumn("Transfer");
                        ImGui::TableSetupColumn("Sparse");
                        ImGui::TableSetupColumn("Protected");
                        ImGui::TableHeadersRow();
                        for (u32 j = 0; j < infos.at(i).families.size(); ++j) {
                            auto &family = infos.at(i).families.at(j);
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::Text("%u", j);
                            ImGui::TableSetColumnIndex(1);
                            ImGui::Text("%c", (family.queueFlags & VK_QUEUE_GRAPHICS_BIT) ? 'X' : ' ');
                            ImGui::TableSetColumnIndex(2);
                            ImGui::Text("%c", (family.queueFlags & VK_QUEUE_COMPUTE_BIT) ? 'X' : ' ');
                            ImGui::TableSetColumnIndex(3);
                            ImGui::Text("%c", (family.queueFlags & VK_QUEUE_TRANSFER_BIT) ? 'X' : ' ');
                            ImGui::TableSetColumnIndex(4);
                            ImGui::Text("%c", (family.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) ? 'X' : ' ');
                            ImGui::TableSetColumnIndex(5);
                            ImGui::Text("%c", (family.queueFlags & VK_QUEUE_PROTECTED_BIT) ? 'X' : ' ');
                        }
                        ImGui::EndTable();
                    }
                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("Extensions")) {
                    for (auto &ext : device.extensions) {
                        if (ImGui::TreeNode(ext.extensionName)) {
                            ImGui::Text("Spec Version: %d", ext.specVersion);

                            ImGui::TreePop();
                        }
                    }
                    ImGui::TreePop();
                }

                ImGui::TreePop();
            }
        }
    }

    if (ImGui::CollapsingHeader("Logical Device")) {
        hk::VulkanContext::LogicalDeviceInfo info = hk::context()->deviceInfo();

        if (ImGui::TreeNode("Chosen Queues")) {
            ImGuiTableFlags flags = ImGuiTableFlags_RowBg;
            flags |= ImGuiTableFlags_BordersOuter;

            ImGui::Text("Graphics Family");

            if (ImGui::BeginTable("Graphics Family", 6, flags)) {
                ImGui::TableSetupColumn("Index");
                ImGui::TableSetupColumn("Graphics");
                ImGui::TableSetupColumn("Compute");
                ImGui::TableSetupColumn("Transfer");
                ImGui::TableSetupColumn("Sparse");
                ImGui::TableSetupColumn("Protected");
                ImGui::TableHeadersRow();

                hk::QueueFamily &family = info.graphicsFamily;
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%u", family.index_);
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%c", (family.properties.queueFlags & VK_QUEUE_GRAPHICS_BIT) ? 'X' : ' ');
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%c", (family.properties.queueFlags & VK_QUEUE_COMPUTE_BIT) ? 'X' : ' ');
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("%c", (family.properties.queueFlags & VK_QUEUE_TRANSFER_BIT) ? 'X' : ' ');
                ImGui::TableSetColumnIndex(4);
                ImGui::Text("%c", (family.properties.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) ? 'X' : ' ');
                ImGui::TableSetColumnIndex(5);
                ImGui::Text("%c", (family.properties.queueFlags & VK_QUEUE_PROTECTED_BIT) ? 'X' : ' ');

                ImGui::EndTable();
            }

            ImGui::Text("Compute Family");

            if (ImGui::BeginTable("Compute Family", 6, flags)) {
                ImGui::TableSetupColumn("Index");
                ImGui::TableSetupColumn("Graphics");
                ImGui::TableSetupColumn("Compute");
                ImGui::TableSetupColumn("Transfer");
                ImGui::TableSetupColumn("Sparse");
                ImGui::TableSetupColumn("Protected");
                ImGui::TableHeadersRow();

                hk::QueueFamily &family = info.computeFamily;
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%u", family.index_);
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%c", (family.properties.queueFlags & VK_QUEUE_GRAPHICS_BIT) ? 'X' : ' ');
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%c", (family.properties.queueFlags & VK_QUEUE_COMPUTE_BIT) ? 'X' : ' ');
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("%c", (family.properties.queueFlags & VK_QUEUE_TRANSFER_BIT) ? 'X' : ' ');
                ImGui::TableSetColumnIndex(4);
                ImGui::Text("%c", (family.properties.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) ? 'X' : ' ');
                ImGui::TableSetColumnIndex(5);
                ImGui::Text("%c", (family.properties.queueFlags & VK_QUEUE_PROTECTED_BIT) ? 'X' : ' ');

                ImGui::EndTable();
            }

            ImGui::Text("Transfer Family");

            if (ImGui::BeginTable("Transfer Family", 6, flags)) {
                ImGui::TableSetupColumn("Index");
                ImGui::TableSetupColumn("Graphics");
                ImGui::TableSetupColumn("Compute");
                ImGui::TableSetupColumn("Transfer");
                ImGui::TableSetupColumn("Sparse");
                ImGui::TableSetupColumn("Protected");
                ImGui::TableHeadersRow();

                hk::QueueFamily &family = info.transferFamily;
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%u", family.index_);
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%c", (family.properties.queueFlags & VK_QUEUE_GRAPHICS_BIT) ? 'X' : ' ');
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%c", (family.properties.queueFlags & VK_QUEUE_COMPUTE_BIT) ? 'X' : ' ');
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("%c", (family.properties.queueFlags & VK_QUEUE_TRANSFER_BIT) ? 'X' : ' ');
                ImGui::TableSetColumnIndex(4);
                ImGui::Text("%c", (family.properties.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) ? 'X' : ' ');
                ImGui::TableSetColumnIndex(5);
                ImGui::Text("%c", (family.properties.queueFlags & VK_QUEUE_PROTECTED_BIT) ? 'X' : ' ');

                ImGui::EndTable();
            }

            ImGui::TreePop();
        }
    }
}

void MetricsPanel::addMonitorInfo()
{
    if (ImGui::CollapsingHeader("Monitors")) {
        const auto infos = hk::platform::getAllMonitorInfos();

        ImGui::Text("Monitors found: %i", infos.size());
        for (auto &info : infos) {
            ImGui::Separator();

            ImGui::Text("Name: %s", info.name.c_str());
            ImGui::Text("Resolution: %i x %i", info.width, info.height);
            ImGui::Text("Scale: %i%%", static_cast<u32>(info.scale * 100.f));
            ImGui::Text("Refresh rate: %i hz", info.hz);
            ImGui::Text("Color depth: %i", info.depth);
        }
    }
}

void MetricsPanel::addCPUInfo()
{
    if (ImGui::CollapsingHeader("CPU")) {
        const auto info = hk::platform::getCpuInfo();

        ImGui::Text("Brand: %s", info.brand.c_str());

        ImGui::Text("Cores: %d", info.cores);
        ImGui::Text("Threads: %d", info.threads);
        ImGui::Text("Page Size: %d", info.page_size);

        ImGui::Separator();

        if (ImGui::TreeNode("Version")) {
            ImGui::Text("Stepping ID: %d", info.version.stepping);
            ImGui::Text("Model: 0x%x", info.version.model);
            ImGui::Text("Family: 0x%x", info.version.family);
            // ImGui::Text("Processor Type: %d", info.version.processor_type);
            //
            ImGui::TreePop();
        }

        ImGui::Separator();

        if (ImGui::TreeNode("SIMD")) {
            ImGuiTableFlags flags =
                ImGuiTableFlags_RowBg |
                ImGuiTableFlags_BordersOuter |
                // ImGuiTableFlags_SizingFixedFit  |
                ImGuiTableFlags_None;

            if (ImGui::BeginTable("SIMD", 2, flags)) {
                ImGui::TableSetupColumn("Type");
                ImGui::TableSetupColumn("Available");
                ImGui::TableHeadersRow();

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("MMX");
                ImGui::TableSetColumnIndex(1); ImGui::Text("%c", info.simd.mmx ? 'X' : ' ');

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("SSE");
                ImGui::TableSetColumnIndex(1); ImGui::Text("%c", info.simd.sse ? 'X' : ' ');

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("SSE2");
                ImGui::TableSetColumnIndex(1); ImGui::Text("%c", info.simd.sse2 ? 'X' : ' ');

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("SSE3");
                ImGui::TableSetColumnIndex(1); ImGui::Text("%c", info.simd.sse3 ? 'X' : ' ');

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("SSSE3");
                ImGui::TableSetColumnIndex(1); ImGui::Text("%c", info.simd.ssse3 ? 'X' : ' ');

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("SSE4.1");
                ImGui::TableSetColumnIndex(1); ImGui::Text("%c", info.simd.sse4_1 ? 'X' : ' ');

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("SSE4.2");
                ImGui::TableSetColumnIndex(1); ImGui::Text("%c", info.simd.sse4_2 ? 'X' : ' ');

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("SSE4A");
                ImGui::TableSetColumnIndex(1); ImGui::Text("%c", info.simd.sse4a ? 'X' : ' ');

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("FMA3");
                ImGui::TableSetColumnIndex(1); ImGui::Text("%c", info.simd.fma3 ? 'X' : ' ');

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("AVX");
                ImGui::TableSetColumnIndex(1); ImGui::Text("%c", info.simd.avx ? 'X' : ' ');

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("AVX2");
                ImGui::TableSetColumnIndex(1); ImGui::Text("%c", info.simd.avx2 ? 'X' : ' ');

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("AVX512");
                ImGui::TableSetColumnIndex(1); ImGui::Text("%c", info.simd.avx512 ? 'X' : ' ');

                ImGui::EndTable();
            }

            ImGui::TreePop();
        }

        ImGui::Separator();

        if (ImGui::TreeNode("Cache")) {
            auto ld = info.cache.L1.data;
            auto li = info.cache.L1.instr;
            auto l2 = info.cache.L2;
            auto l3 = info.cache.L3;
            ImGui::Text("L1 Data: %d x %d KB, %d-way", ld.count, ld.size / 1024, ld.associativity);
            ImGui::Text("L1 Inst: %d x %d KB, %d-way", li.count, li.size / 1024, li.associativity);
            ImGui::Text("Level 2: %d x %d KB, %d-way", l2.count, l2.size / 1024, l2.associativity);
            ImGui::Text("Level 3: %d x %d KB, %d-way", l3.count, l3.size / 1024, l3.associativity);

            // ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen;
            //
            // auto cache_info = [](hk::platform::CpuInfo::Cache::CacheInfo cache)
            // {
            //     ImGui::Text("Size: %d KB", cache.size / 1024);
            //     ImGui::Text("Num of caches: %d", cache.count);
            //
            //     ImGui::Text("Sets: %d", cache.sets);
            //     ImGui::Text("Ways of cache associativity: %d", cache.associativity);
            //
            //     ImGui::Text("Max Threads: %d", cache.max_threads);
            // };
            //
            // if (ImGui::TreeNodeEx("L1", flags)) {
            //     if (ImGui::TreeNodeEx("L1 Data", flags)) {
            //         cache_info(info.cache.L1.data);
            //         ImGui::TreePop();
            //     }
            //
            //     if (ImGui::TreeNodeEx("L1 Instruction", flags)) {
            //         cache_info(info.cache.L1.instr);
            //         ImGui::TreePop();
            //     }
            //
            //     ImGui::TreePop();
            // }
            //
            // if (ImGui::TreeNodeEx("L2", flags)) {
            //     cache_info(info.cache.L2);
            //
            //     ImGui::TreePop();
            // }
            //
            // if (ImGui::TreeNodeEx("L3", flags)) {
            //     cache_info(info.cache.L3);
            //
            //     ImGui::TreePop();
            // }

            ImGui::TreePop();
        }

        ImGui::Separator();

        if (ImGui::TreeNode("Features")) {
            ImGuiTableFlags flags =
                ImGuiTableFlags_RowBg |
                ImGuiTableFlags_BordersOuter |
                // ImGuiTableFlags_SizingFixedFit  |
                ImGuiTableFlags_None;

            if (ImGui::BeginTable("Features", 2, flags)) {
                ImGui::TableSetupColumn("Type");
                ImGui::TableSetupColumn("Available");
                ImGui::TableHeadersRow();

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Virtual 8086 mode extensions");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%c", info.feature.vme ? 'X' : ' ');

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Page Size Extension (4 MB pages)");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%c", info.feature.pse ? 'X' : ' ');

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Physical Address Extension");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%c", info.feature.pae ? 'X' : ' ');

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Machine Check Exception");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%c", info.feature.mce ? 'X' : ' ');

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Machine check architecture");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%c", info.feature.mca ? 'X' : ' ');

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Virtual Machine eXtensions");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%c", info.feature.vmx ? 'X' : ' ');

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Process context identifiers (CR4 bit 17)");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%c", info.feature.pcid ? 'X' : ' ');

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("POPCNT instruction");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%c", info.feature.popcnt ? 'X' : ' ');

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("AES instruction set");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%c", info.feature.aes ? 'X' : ' ');

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Hypervisor");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%c", info.feature.hypervisor ? 'X' : ' ');

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("SHA");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%c", info.feature.sha ? 'X' : ' ');

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("SHA512");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%c", info.feature.sha512 ? 'X' : ' ');

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Translation Cache Extension");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%c", info.feature.tce ? 'X' : ' ');

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Trailing Bit Manipulation");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%c", info.feature.tbm ? 'X' : ' ');

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Long mode");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%c", info.feature.lm ? 'X' : ' ');

                ImGui::EndTable();
            }

            ImGui::TreePop();
        }
    }
}
