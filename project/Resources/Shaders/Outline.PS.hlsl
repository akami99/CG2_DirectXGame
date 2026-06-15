#include "Fullscreen.hlsli"

Texture2D<float4> gTexture : register(t0);       // 元画像
Texture2D<float> gDepthTexture : register(t1); // 深度テクスチャ

SamplerState gSamplerLinear : register(s0); // LINEARサンプラー
SamplerState gSamplerPoint : register(s2);  // POINTサンプラー (追加した s2 を使用)

cbuffer OutlineParams : register(b0)
{
    float4x4 gProjectionInverse;
    float gEdgeMultiplier;
};

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

// Prewitt カーネルの定義
static const float kPrewittHorizontalKernel[3][3] = {
    { -1.0f / 6.0f, 0.0f, 1.0f / 6.0f },
    { -1.0f / 6.0f, 0.0f, 1.0f / 6.0f },
    { -1.0f / 6.0f, 0.0f, 1.0f / 6.0f }
};

static const float kPrewittVerticalKernel[3][3] = {
    { -1.0f / 6.0f, -1.0f / 6.0f, -1.0f / 6.0f },
    {  0.0f,         0.0f,         0.0f },
    {  1.0f / 6.0f,  1.0f / 6.0f,  1.0f / 6.0f }
};

// 3x3 相対位置のインデックス
static const int2 kIndex3x3[3][3] = {
    { int2(-1, -1), int2(0, -1), int2(1, -1) },
    { int2(-1,  0), int2(0,  0), int2(1,  0) },
    { int2(-1,  1), int2(0,  1), int2(1,  1) }
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    
    uint width, height;
    gTexture.GetDimensions(width, height);
    float2 uvStepSize = float2(1.0f / float(width), 1.0f / float(height));
    
    float2 difference = float2(0.0f, 0.0f);
    
    for (int x = 0; x < 3; ++x)
    {
        for (int y = 0; y < 3; ++y)
        {
            float2 texcoord = input.texcoord + kIndex3x3[x][y] * uvStepSize;
            float ndcDepth = gDepthTexture.Sample(gSamplerPoint, texcoord);
            
            // NDC -> View空間のZ（深度値）に復元
            float4 viewSpace = mul(float4(0.0f, 0.0f, ndcDepth, 1.0f), gProjectionInverse);
            float viewZ = viewSpace.z * rcp(viewSpace.w);
            
            difference.x += viewZ * kPrewittHorizontalKernel[x][y];
            difference.y += viewZ * kPrewittVerticalKernel[x][y];
        }
    }
    
    // 差分の大きさをエッジの強さ（ウェイト）とする
    float weight = length(difference);
    
    // エッジの明瞭さ向上のためのスケール
    weight = saturate(weight * gEdgeMultiplier);
    
    // 元画像の色をサンプリング
    float3 color = gTexture.Sample(gSamplerLinear, input.texcoord).rgb;
    
    // エッジ部分を黒く合成
    output.color.rgb = (1.0f - weight) * color;
    output.color.a = 1.0f;
    
    return output;
}
