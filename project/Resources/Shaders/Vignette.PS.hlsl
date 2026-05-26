#include "Fullscreen.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

// ビネット調整用の定数バッファ
cbuffer VignetteParams : register(b0)
{
    float gScale;      // スケール値 (例: 16.0f)
    float gExponent;   // べき乗数 (例: 0.8f)
};

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    output.color = gTexture.Sample(gSampler, input.texcoord);

    // 周囲を0に、中心になるほど明るくなるように計算で調整
    float2 correct = input.texcoord * (1.0f - input.texcoord.yx);
    
    // 中心付近の明るさをスケール調整
    float vignette = correct.x * correct.y * gScale;
    
    // べき乗で減光のグラデーション具合を調整
    vignette = saturate(pow(vignette, gExponent));
    
    // カラーに乗算
    output.color.rgb *= vignette;

    return output;
}
