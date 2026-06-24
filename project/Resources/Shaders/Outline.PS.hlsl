#include "Fullscreen.hlsli"

Texture2D<float4> gTexture : register(t0);       // 元画像
Texture2D<float> gDepthTexture : register(t1); // 深度テクスチャ

SamplerState gSamplerLinear : register(s0); // LINEARサンプラー
SamplerState gSamplerPoint : register(s2);  // POINTサンプラー (追加した s2 を使用)

cbuffer OutlineParams : register(b0)
{
    float4x4 gProjectionInverse;
    float gDepthEdgeMultiplier;
    float gColorEdgeMultiplier;
};

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

// Sobel カーネルの定義
static const float kSobelHorizontalKernel[3][3] = {
    { -1.0f, 0.0f, 1.0f },
    { -2.0f, 0.0f, 2.0f },
    { -1.0f, 0.0f, 1.0f }
};

static const float kSobelVerticalKernel[3][3] = {
    { -1.0f, -2.0f, -1.0f },
    {  0.0f,  0.0f,  0.0f },
    {  1.0f,  2.0f,  1.0f }
};

// 3x3 相対位置のインデックス
static const int2 kIndex3x3[3][3] = {
    { int2(-1, -1), int2(0, -1), int2(1, -1) },
    { int2(-1,  0), int2(0,  0), int2(1,  0) },
    { int2(-1,  1), int2(0,  1), int2(1,  1) }
};

// RGBカラーを輝度（Luminance）に変換する関数
float getLuminance(float3 color)
{
    return dot(color, float3(0.2126f, 0.7152f, 0.0722f));
}

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    
    uint width, height;
    gTexture.GetDimensions(width, height);
    float2 uvStepSize = float2(1.0f / float(width), 1.0f / float(height));
    
    float2 depthDifference = float2(0.0f, 0.0f);
    float2 colorDifference = float2(0.0f, 0.0f);
    
    for (int x = 0; x < 3; ++x)
    {
        for (int y = 0; y < 3; ++y)
        {
            float2 texcoord = input.texcoord + kIndex3x3[x][y] * uvStepSize;
            
            // 1. 深度情報のサンプリングとView Zの復元
            float ndcDepth = gDepthTexture.Sample(gSamplerPoint, texcoord);
            float4 viewSpace = mul(float4(0.0f, 0.0f, ndcDepth, 1.0f), gProjectionInverse);
            float viewZ = viewSpace.z * rcp(viewSpace.w);
            
            depthDifference.x += viewZ * kSobelHorizontalKernel[x][y];
            depthDifference.y += viewZ * kSobelVerticalKernel[x][y];
            
            // 2. カラー情報のサンプリングと輝度への変換
            float4 colorSample = gTexture.Sample(gSamplerLinear, texcoord);
            float luminance = getLuminance(colorSample.rgb);
            
            colorDifference.x += luminance * kSobelHorizontalKernel[x][y];
            colorDifference.y += luminance * kSobelVerticalKernel[x][y];
        }
    }
    
    // それぞれのエッジ強度を算出
    float depthWeight = length(depthDifference) * gDepthEdgeMultiplier;
    
    // 遠すぎる場所（スカイボックスなど NDC Depth = 1.0 付近）はカラーエッジを無効化
    float ndcDepthCenter = gDepthTexture.Sample(gSamplerPoint, input.texcoord);
    float farMask = (ndcDepthCenter < 0.9999f) ? 1.0f : 0.0f;
    
    // カラーエッジにしきい値処理（0.1以下の微小な輝度差はノイズとしてカット）を適用
    float colorWeight = 0.0f;
    float colorDiff = length(colorDifference);
    if (colorDiff > 0.1f)
    {
        colorWeight = (colorDiff - 0.1f) * gColorEdgeMultiplier;
    }
    colorWeight *= farMask;
    
    // 深度エッジとカラーエッジを合成 (大きい方を採用)
    float weight = saturate(max(depthWeight, colorWeight));
    
    // 元画像（中心）の色をサンプリング
    float3 centerColor = gTexture.Sample(gSamplerLinear, input.texcoord).rgb;
    
    // エッジ部分を黒く合成
    output.color.rgb = (1.0f - weight) * centerColor;
    output.color.a = 1.0f;
    
    return output;
}
