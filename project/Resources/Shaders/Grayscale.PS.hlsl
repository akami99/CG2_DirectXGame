#include "Fullscreen.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

// グレースケール・セピア共通の定数バッファ
cbuffer ColorFilter : register(b0)
{
    float3 gColorScale; // (1,1,1) でグレースケール、セピア値でセピア調
    float  gStrength;   // 0.0f: 元の色, 1.0f: フィルター色
};

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    float4 textureColor = gTexture.Sample(gSampler, input.texcoord);

    // 輝度 (Luminance) の計算
    float luminance = dot(textureColor.rgb, float3(0.2125f, 0.7154f, 0.0721f));

    // 輝度にカラースケールを掛ける
    float3 filterColor = luminance * gColorScale;

    // strength に基づいて lerp
    output.color.rgb = lerp(textureColor.rgb, filterColor, gStrength);
    output.color.a   = textureColor.a;

    return output;
}