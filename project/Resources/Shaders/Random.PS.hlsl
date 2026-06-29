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

// サイン波を使用しない高品質な3Dハッシュ乱数生成関数
// 3次元目の入力に時間を用いることで、X/Y平面との相関（斜めの縞模様など）を完全に排除
float hash3dTo1d(float3 p)
{
    float3 p3 = frac(p * 0.1031);
    p3 += dot(p3, p3.zyx + 31.32);
    return frac((p3.x + p3.y) * p3.z);
}

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    
    // 元画像の色をサンプリング
    float4 texColor = gTexture.Sample(gSamplerLinear, input.texcoord);
    
    // 空間座標(UV)と時間を3次元座標としてハッシュ関数に渡し、極めて細かく均一な砂嵐ノイズを生成
    // スケールを 1000.0f に大きくすることで、ピクセルレベルの細かい点ノイズを再現
    float noise = hash3dTo1d(float3(input.texcoord * 1000.0f, gTime * 60.0f));
    
    // 元画像にノイズを乗算した色と、元の色を強度(gStrength)でブレンドする
    float3 noiseColor = texColor.rgb * noise;
    output.color = float4(lerp(texColor.rgb, noiseColor, gStrength), texColor.a);
    
    return output;
}
