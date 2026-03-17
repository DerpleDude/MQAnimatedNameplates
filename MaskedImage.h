#pragma once

#include "imgui/imgui.h"
#include "mq/api/Textures.h"

#include <string>

namespace Ui {

// Renders a source image with an alpha mask applied via a DX9 pixel shader.
// The mask's alpha channel is multiplied against the source, cutting out regions
// where the mask is transparent.
class MaskedImage
{
public:
    MaskedImage() = default;
    MaskedImage(const std::string& sourcePath, const std::string& maskPath);

    bool IsValid() const;

    // Render source image masked by the mask texture into [min, max].
    // tint is applied to the source image (same as ImDrawList::AddImage tint).
    void Render(ImDrawList* dl, const ImVec2& min, const ImVec2& max,
        ImU32 tint = IM_COL32_WHITE) const;

    // Render with 9-slice scaling so the image fits any destination size while
    // keeping corners at a fixed pixel size.
    // margins: ImVec4(left, top, right, bottom) — pixel distances from each edge
    // that define the 9-slice split points in the source texture.
    void RenderNineSlice(ImDrawList* dl, const ImVec2& min, const ImVec2& max,
        const ImVec4& margins, ImU32 tint = IM_COL32_WHITE) const;

    // Release the static DX9 pixel shader. Call on plugin shutdown.
    static void ReleaseShader();

private:
    mq::MQTexturePtr m_pSource;
    mq::MQTexturePtr m_pMask;
};

} // namespace Ui
