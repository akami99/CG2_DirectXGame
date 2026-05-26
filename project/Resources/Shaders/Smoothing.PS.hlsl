#include "Fullscreen.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s1); // CLAMPサンプラーを使用

cbuffer SmoothingParams : register(b0)
{
    int gKernelSize; // カーネルサイズ (3, 5, 7, 9, ...)
};

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    
    uint width, height;
    gTexture.GetDimensions(width, height);
    float2 uvStepSize = float2(1.0f / float(width), 1.0f / float(height));
    
    // 半径を計算
    int radius = (gKernelSize - 1) / 2;
    // ガード（最大ループ制限）
    radius = clamp(radius, 0, 15);
    
    float3 sumColor = float3(0.0f, 0.0f, 0.0f);
    float sampleCount = 0.0f;
    
    for (int x = -radius; x <= radius; ++x)
    {
        for (int y = -radius; y <= radius; ++y)
        {
            float2 offset = float2(x, y) * uvStepSize;
            float3 fetchColor = gTexture.Sample(gSampler, input.texcoord + offset).rgb;
            sumColor += fetchColor;
            sampleCount += 1.0f;
        }
    }
    
    output.color.rgb = sumColor / sampleCount;
    output.color.a = 1.0f;
    
    return output;
}
