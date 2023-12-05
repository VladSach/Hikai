#include "ShaderManager.h"

// FIX: this is a dirty solution just to get it up and running

#include "Windows.h"
#include "vendor/dxc/dxcapi.h"

#include <fstream>

#include <wrl/client.h>
using namespace Microsoft::WRL;

hk::vector<u32> ShaderManager::loadShader(const std::string &path, const b8 type)
{
    std::ifstream file(path, std::ios::binary | std::ios::in | std::ios::ate);
    ALWAYS_ASSERT(file.is_open(), "Failed to open a shader file:", path);

    hk::vector<u8> shaderData;

    u32 size = file.tellg();
    file.seekg(0, file.beg);
    shaderData.resize(size);
    file.read((char*)(shaderData.data()), size);
    file.close();

    ALWAYS_ASSERT(shaderData.size() > 0, "Failed to read file");

    ComPtr<IDxcUtils> dxcUtils;
    ComPtr<IDxcCompiler3> dxcCompiler;
    DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(dxcUtils.ReleaseAndGetAddressOf()));
    DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));

    ComPtr<IDxcBlobEncoding> source;
    dxcUtils->CreateBlob(shaderData.data(), shaderData.size(), CP_UTF8, source.GetAddressOf());

    hk::vector<LPCWSTR> arguments;

    arguments.push_back(L"-Zpc");
    arguments.push_back(L"-HV");
    arguments.push_back(L"2021");

    // -E for the entry point (eg. 'main')
    arguments.push_back(L"-E");
    arguments.push_back(L"main");

    // -T for the target profile (eg. 'ps_6_6')
    arguments.push_back(L"-T");
    arguments.push_back(type ? L"vs_5_0" : L"ps_5_0");

    arguments.push_back(L"-spirv");
    arguments.push_back(L"-fspv-target-env=vulkan1.0");

    // Strip reflection data and pdbs (see later)
    // arguments.push_back(L"-Qstrip_debug");
    // arguments.push_back(L"-Qstrip_reflect");

    // arguments.push_back(DXC_ARG_WARNINGS_ARE_ERRORS); //-WX
    // arguments.push_back(DXC_ARG_DEBUG); //-Zi

    DxcBuffer sourceBuffer;
    sourceBuffer.Ptr = source->GetBufferPointer();
    sourceBuffer.Size = source->GetBufferSize();
    sourceBuffer.Encoding = 0;

    ComPtr<IDxcResult> result;
    dxcCompiler->Compile(&sourceBuffer, arguments.data(), (u32)arguments.size(), nullptr, IID_PPV_ARGS(result.GetAddressOf()));

    ComPtr<IDxcBlobUtf8> errors;
    result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(errors.GetAddressOf()), nullptr);
    if (errors && errors->GetStringLength() > 0)
    {
        LOG_ERROR("Shader Error:", (char*)errors->GetBufferPointer());
    }

    ComPtr<IDxcBlob> shaderObj;
    result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(shaderObj.GetAddressOf()), nullptr);

    hk::vector<u32> spirvBuffer;
    spirvBuffer.resize(shaderObj->GetBufferSize() / sizeof(u32));

    for (u32 i = 0; i < spirvBuffer.size(); i++) {
        u32 spvByte = static_cast<u32*>(shaderObj->GetBufferPointer())[i];
        spirvBuffer[i] = spvByte;
    }

    return spirvBuffer;
}
