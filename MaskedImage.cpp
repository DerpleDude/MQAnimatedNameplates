#include "MaskedImage.h"

#include "mq/Plugin.h"
#include "spdlog/spdlog.h"

#include <d3d9.h>
#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler")

namespace Ui {

static const char* kMaskShaderSrc = R"(
sampler2D srcTex   : register(s0);
sampler2D maskTex  : register(s1);

float4 SrcRange  : register(c0);
float4 MaskRange : register(c1);

struct PS_INPUT
{
    float4 col : COLOR0;
    float2 uv  : TEXCOORD0;
};

float4 main(PS_INPUT input) : COLOR
{
    float2 srcUv  = SrcRange.xy  + input.uv * (SrcRange.zw  - SrcRange.xy);
    float2 maskUv = MaskRange.xy + input.uv * (MaskRange.zw - MaskRange.xy);

    float4 src  = tex2D(srcTex,  srcUv)  * input.col;
    float4 mask = tex2D(maskTex, maskUv);

    return src * mask.a;
}
)";

static IDirect3DPixelShader9* s_pMaskPS    = nullptr;
static IDirect3DPixelShader9* s_pSavedPS   = nullptr;
static IDirect3DBaseTexture9* s_pSavedTex1 = nullptr;

static bool InitMaskShader(IDirect3DDevice9* dev)
{
    if (s_pMaskPS)
        return true;

    ID3DBlob* code = nullptr;
    ID3DBlob* err  = nullptr;

    HRESULT hr = D3DCompile(
        kMaskShaderSrc, strlen(kMaskShaderSrc),
        nullptr, nullptr, nullptr,
        "main", "ps_2_0", 0, 0,
        &code, &err);

    if (FAILED(hr))
    {
        if (err)
        {
            SPDLOG_ERROR("MaskedImage: shader compile error: {}",
                static_cast<const char*>(err->GetBufferPointer()));
            err->Release();
        }
        return false;
    }
    if (err)
        err->Release();

    hr = dev->CreatePixelShader(
        static_cast<const DWORD*>(code->GetBufferPointer()), &s_pMaskPS);
    code->Release();

    return SUCCEEDED(hr);
}

struct MaskCbData
{
    ImTextureID maskTexId;
    float       srcRange[4];
    float       maskRange[4];
};

static void MaskBefore(const ImDrawList*, const ImDrawCmd* cmd)
{
    if (!gpD3D9Device)
        return;

    if (!InitMaskShader(gpD3D9Device))
        return;

    const auto* data = static_cast<const MaskCbData*>(cmd->UserCallbackData);

    gpD3D9Device->GetPixelShader(&s_pSavedPS);
    gpD3D9Device->GetTexture(1, &s_pSavedTex1);

    gpD3D9Device->SetTexture(1, static_cast<IDirect3DTexture9*>(data->maskTexId));
    gpD3D9Device->SetPixelShaderConstantF(0, data->srcRange,  1);
    gpD3D9Device->SetPixelShaderConstantF(1, data->maskRange, 1);
    gpD3D9Device->SetPixelShader(s_pMaskPS);
}

static void MaskAfter(const ImDrawList*, const ImDrawCmd*)
{
    if (!gpD3D9Device)
        return;

    gpD3D9Device->SetPixelShader(s_pSavedPS);
    if (s_pSavedPS)
    {
        s_pSavedPS->Release();
        s_pSavedPS = nullptr;
    }

    gpD3D9Device->SetTexture(1, s_pSavedTex1);
    if (s_pSavedTex1)
    {
        s_pSavedTex1->Release();
        s_pSavedTex1 = nullptr;
    }
}

static ImVec2 GetTextureSize(ImTextureID texId)
{
    if (!texId)
        return { 1.0f, 1.0f };

    auto* tex = static_cast<IDirect3DTexture9*>(texId);
    D3DSURFACE_DESC desc{};
    if (SUCCEEDED(tex->GetLevelDesc(0, &desc)))
        return { static_cast<float>(desc.Width), static_cast<float>(desc.Height) };

    return { 1.0f, 1.0f };
}

// Helper: emit one masked quad with independent source and mask UVs.
static void AddMaskedImage(ImDrawList* dl,
    ImTextureID srcTex,  const ImVec2& destMin, const ImVec2& destMax,
    const ImVec2& srcUvMin,  const ImVec2& srcUvMax,
    ImTextureID maskTex, const ImVec2& maskUvMin, const ImVec2& maskUvMax,
    ImU32 tint)
{
    MaskCbData data{
        maskTex,
        { srcUvMin.x,  srcUvMin.y,  srcUvMax.x,  srcUvMax.y  },
        { maskUvMin.x, maskUvMin.y, maskUvMax.x, maskUvMax.y }
    };
    dl->AddCallback(MaskBefore, &data, sizeof(data));
    dl->AddImage(srcTex, destMin, destMax, ImVec2(0, 0), ImVec2(1, 1), tint);
    dl->AddCallback(MaskAfter, nullptr);
}

// --- MaskedImage ----------------------------------------------------------

MaskedImage::MaskedImage(const std::string& sourcePath, const std::string& maskPath)
{
    m_pSource = mq::CreateTexturePtr(sourcePath);
    m_pMask   = mq::CreateTexturePtr(maskPath);
}

bool MaskedImage::IsValid() const
{
    return m_pSource && m_pSource->IsValid()
        && m_pMask   && m_pMask->IsValid();
}

void MaskedImage::Render(ImDrawList* dl, const ImVec2& min, const ImVec2& max, ImU32 tint) const
{
    if (!IsValid())
        return;

    AddMaskedImage(dl,
        m_pSource->GetTextureID(), min, max,
        ImVec2(0, 0), ImVec2(1, 1),
        m_pMask->GetTextureID(),
        ImVec2(0, 0), ImVec2(1, 1),
        tint);
}

void MaskedImage::RenderNineSlice(ImDrawList* dl, const ImVec2& min, const ImVec2& max, const ImVec4& margins, ImU32 tint) const
{
    if (!IsValid())
        return;

    const float destW = max.x - min.x;
    const float destH = max.y - min.y;

    float sx = 1.0f;
    float sy = 1.0f;

    if (margins.x + margins.z > destW && destW > 0.0f)
    {
        sx = destW / (margins.x + margins.z);
    }

    if (margins.y + margins.w > destH && destH > 0.0f)
    {
        sy = destH / (margins.y + margins.w);
    }

    const float left   = margins.x * sx;
    const float top    = margins.y * sy;
    const float right  = margins.z * sx;
    const float bottom = margins.w * sy;

    const ImVec2 srcSize  = GetTextureSize(m_pSource->GetTextureID());
    const ImVec2 maskSize = GetTextureSize(m_pMask->GetTextureID());

    const float srcUvX[4]  = { 0.0f, left / srcSize.x,  1.0f - right  / srcSize.x,  1.0f };
    const float srcUvY[4]  = { 0.0f, top  / srcSize.y,  1.0f - bottom / srcSize.y,  1.0f };
    const float maskUvX[4] = { 0.0f, left / maskSize.x, 1.0f - right  / maskSize.x, 1.0f };
    const float maskUvY[4] = { 0.0f, top  / maskSize.y, 1.0f - bottom / maskSize.y, 1.0f };

    const float dX[4] = { min.x, min.x + left,   max.x - right,  max.x };
    const float dY[4] = { min.y, min.y + top,    max.y - bottom, max.y };

    for (int row = 0; row < 3; ++row)
    {
        for (int col = 0; col < 3; ++col)
        {
            ImVec2 destMin(dX[col],     dY[row]);
            ImVec2 destMax(dX[col + 1], dY[row + 1]);

            if (destMax.x <= destMin.x || destMax.y <= destMin.y)
                continue;

            ImVec2 srcUvMin(srcUvX[col],     srcUvY[row]);
            ImVec2 srcUvMax(srcUvX[col + 1], srcUvY[row + 1]);

            ImVec2 maskUvMin(maskUvX[col],     maskUvY[row]);
            ImVec2 maskUvMax(maskUvX[col + 1], maskUvY[row + 1]);

            AddMaskedImage(dl,
                m_pSource->GetTextureID(), destMin, destMax,
                srcUvMin, srcUvMax,
                m_pMask->GetTextureID(),
                maskUvMin, maskUvMax,
                tint);
        }
    }
}

void MaskedImage::ReleaseShader()
{
    if (s_pMaskPS)
    {
        s_pMaskPS->Release();
        s_pMaskPS = nullptr;
    }
}

} // namespace Ui
