#include "Fullscreen.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

// グレースケール・セピア共通の定数バッファ
cbuffer ColorFilter : register(b0)
{
    float3 gColorScale; // (1,1,1) でグレースケール、セピア値でセピア調
    float  gPadding;    // 16バイトアライメント用パディング
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
    // gColorScale = (1, 1, 1)           -> グレースケール
    // gColorScale = (1, 0.69, 0.40) 等 -> セピア調
    output.color.rgb = luminance * gColorScale;
    output.color.a   = textureColor.a;

    return output;
}