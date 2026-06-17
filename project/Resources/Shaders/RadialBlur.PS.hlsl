#include "Fullscreen.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSamplerLinear : register(s1); // CLAMPサンプラーを使用

cbuffer RadialBlurParams : register(b0)
{
    float2 gCenter;
    float gBlurWidth;
    int gSampleCount;
};

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    
    // 中心からの方向（あえて正規化せず、遠いほど大きくぼかす）
    float2 direction = input.texcoord - gCenter;
    float3 outputColor = float3(0.0f, 0.0f, 0.0f);
    
    // 最大ループ回数は上限を50とし、実際のgSampleCountで制限
    int samples = clamp(gSampleCount, 1, 50);
    
    for (int i = 0; i < samples; ++i)
    {
        float2 texcoord = input.texcoord + direction * gBlurWidth * float(i);
        outputColor += gTexture.Sample(gSamplerLinear, texcoord).rgb;
    }
    
    output.color.rgb = outputColor * rcp(float(samples));
    output.color.a = 1.0f;
    
    return output;
}
