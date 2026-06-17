#include "Fullscreen.hlsli"

Texture2D<float4> gTexture : register(t0);       // 元画像
Texture2D<float4> gMaskTexture : register(t1);   // マスクテクスチャ

SamplerState gSamplerLinear : register(s0);       // LINEARサンプラー

cbuffer DissolveParams : register(b0)
{
    float3 gEdgeColor;
    float gThreshold;
    float gEdgeRange;
};

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    
    // マスクテクスチャから値をサンプリング（Rチャンネルを利用）
    float mask = gMaskTexture.Sample(gSamplerLinear, input.texcoord).r;
    
    // 閾値以下はdiscardして切り抜く
    if (mask <= gThreshold)
    {
        discard;
    }
    
    // 元画像の色をサンプリング
    float4 texColor = gTexture.Sample(gSamplerLinear, input.texcoord);
    output.color = texColor;
    
    // エッジ部分の係数を計算 (1.0f - smoothstep で閾値に近い部分ほど強いウェイトにする)
    float edge = 1.0f - smoothstep(gThreshold, gThreshold + gEdgeRange, mask);
    
    // エッジに指定色を加算
    output.color.rgb += edge * gEdgeColor;
    
    return output;
}
