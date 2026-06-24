#include "Fullscreen.hlsli"

Texture2D<float4> gTexture : register(t0);       // 元画像
SamplerState gSamplerLinear : register(s0);       // サンプラー

cbuffer RandomParams : register(b0)
{
    float gTime;       // 経過時間（乱数のシード変更用）
    float gStrength;   // ノイズの適用強度 (0.0~1.0)
};

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

// GPU 2D->1D 疑似乱数生成関数 (Ronja's Tutorials 準拠)
float rand2dTo1d(float2 value)
{
    float2 smallValue = sin(value);
    float random = dot(smallValue, float2(12.9898, 78.233));
    random = frac(sin(random) * 143758.5453);
    return random;
}

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    
    // 元画像の色をサンプリング
    float4 texColor = gTexture.Sample(gSamplerLinear, input.texcoord);
    
    // 経過時間とUV座標から疑似乱数 (0.0 ~ 1.0) を生成
    // 拡大縮小アーティファクトを避けるため、timeを加算してシードを動かす
    float noise = rand2dTo1d(input.texcoord * 50.0f + float2(gTime, -gTime));
    
    // 元画像にノイズを乗算した色と、元の色を強度(gStrength)でブレンドする
    float3 noiseColor = texColor.rgb * noise;
    output.color = float4(lerp(texColor.rgb, noiseColor, gStrength), texColor.a);
    
    return output;
}
