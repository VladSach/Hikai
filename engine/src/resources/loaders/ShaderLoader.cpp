#include "ShaderLoader.h"

#include "platform/platform.h"

#if defined(HKWINDOWS)
#ifdef UNDEFINED_FAR
    #pragma pop_macro("far")
#endif // UNDEFINED_FAR

#include <wrl/client.h>

#ifdef far
    #define UNDEFINED_FAR
    #pragma push_macro("far")
    #undef far
#endif // far

#define CComPtr Microsoft::WRL::ComPtr
#else
#include "vendor/dxc/WinAdapter.h"
#endif

#include "vendor/dxc/dxcapi.h"

#include "platform/filesystem.h"

#include "utils/strings/hklocale.h"

namespace hk::dxc {

static CComPtr<IDxcUtils> dxcUtils;
static CComPtr<IDxcCompiler3> dxcCompiler;
static b8 initialized = false;

struct IncludeHandler : public IDxcIncludeHandler {
    HRESULT STDMETHODCALLTYPE LoadSource(
        _In_z_ LPCWSTR pFilename,
        _COM_Outptr_result_maybenull_ IDxcBlob** ppIncludeSource) override
    {
        HRESULT hr;
        CComPtr<IDxcBlobEncoding> pEncoding;

        hr = dxcUtils->LoadFile(pFilename, nullptr, pEncoding.GetAddressOf());
        if (SUCCEEDED(hr)) {
            *ppIncludeSource = pEncoding.Detach();
        }

        return hr;
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(
        REFIID riid,
        _COM_Outptr_ void __RPC_FAR* __RPC_FAR* ppvObject) override
    {
        (void)riid; (void)ppvObject;
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef(void) override { return 0; }
    ULONG STDMETHODCALLTYPE Release(void) override { return 0; }
};

static struct IncludeHandler dxcIncludeHandler;

void init()
{
    HRESULT err;

    err = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
    ALWAYS_ASSERT(SUCCEEDED(err), "Failed to create IDxcUtils");

    err = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
    ALWAYS_ASSERT(SUCCEEDED(err), "Failed to create IDxcCompiler3");

    initialized = true;
}

void deinit()
{
    dxcCompiler->Release();
    dxcCompiler = nullptr;

    dxcUtils->Release();
    dxcUtils = nullptr;
}

hk::vector<u32> loadShader(const ShaderDesc &desc)
{
    if(!initialized) { init(); }

    hk::vector<u8> shader;
    if (!hk::filesystem::readFile(desc.path, shader)) {
        LOG_ERROR("Failed to read from file:", desc.path);
    }

    HRESULT err;

    CComPtr<IDxcBlobEncoding> source;
    err = dxcUtils->CreateBlob(shader.data(), shader.size(), CP_UTF8, &source);
    ALWAYS_ASSERT(SUCCEEDED(err), "Failed to create IDxcBlobEncoding");

    hk::vector<LPCWSTR> args;
    std::wstring wmain(desc.entry.begin(), desc.entry.end());
    // -E for the entry point (eg. 'main')
    args.push_back(L"-E");
    args.push_back((LPCWSTR)wmain.c_str());

    constexpr wchar_t const *types[] = {
        L"vs_",
        L"hs_",
        L"ds_",
        L"gs_",
        L"ps_",
        L"cs_"
    };
    constexpr wchar_t const *models[] = {
        L"6_0",
        L"6_1",
        L"6_2",
        L"6_3",
        L"6_4",
        L"6_5",
        L"6_6",
        L"6_7",
    };

    std::wstring target(types[static_cast<u8>(desc.type)]);
    target += models[static_cast<u8>(desc.model)];

    // -T for the target profile (eg. 'ps_6_6')
    args.push_back(L"-T");
    args.push_back((LPCWSTR)target.c_str());

    switch (desc.ir) {
    case ShaderIR::DXIL: {
        args.push_back(L"-Wno-ignored-attributes");
    } break;
    case ShaderIR::SPIRV: {
        args.push_back(L"-spirv");
        args.push_back(L"-fspv-target-env=vulkan1.3");
        args.push_back(L"-fvk-use-dx-layout");
    } break;
    }

    if (desc.debug) {
        // Disable optimizations
        args.push_back(L"-Od");

        // Enable debug information
        args.push_back(L"-Zi");

        // Embed PDB in shader container (must be used with /Zi)
        args.push_back(L"-Qembed_debug");

        // args.push_back(L"-fspv-debug=vulkan-with-source");
    } else {
        args.push_back(L"-O3");
    }


    args.push_back(L"-I");
    args.push_back(L"..\\engine\\assets\\shaders\\includes"); // TODO: make conf.

    IncludeHandler handler;

    // Pack matrices in Column-major order
    // args.push_back(L"-Zpc");

    DxcBuffer sourceBuffer;
    sourceBuffer.Ptr = source->GetBufferPointer();
    sourceBuffer.Size = source->GetBufferSize();
    sourceBuffer.Encoding = 0;

    CComPtr<IDxcResult> result;
    err = dxcCompiler->Compile(&sourceBuffer, args.data(), (u32)args.size(),
                               &handler, IID_PPV_ARGS(&result));
    ALWAYS_ASSERT(SUCCEEDED(err), "Failed to compile shader");

    CComPtr<IDxcBlobUtf8> errors;
    err = result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
    ALWAYS_ASSERT(SUCCEEDED(err), "Failed to get shader errors");
    if (errors && errors->GetStringLength() > 0) {
        LOG_ERROR("Shader Error:", (char*)errors->GetBufferPointer());
    }

    CComPtr<IDxcBlob> shaderObj;
    err = result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderObj), nullptr);
    ALWAYS_ASSERT(SUCCEEDED(err), "Failed to get shader");

    const u32 rsz = static_cast<u32>(shaderObj->GetBufferSize()) / sizeof(u32);
    hk::vector<u32> spirvBuffer(rsz);

    for (u32 i = 0; i < spirvBuffer.size(); i++) {
        u32 spvByte = static_cast<u32*>(shaderObj->GetBufferPointer())[i];
        spirvBuffer[i] = spvByte;
    }

    return spirvBuffer;
}

}
