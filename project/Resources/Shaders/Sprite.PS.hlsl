#include "SpriteCommon.hlsli"

// Root Parameter 0 (b0) に対応する Material CBV を宣言
ConstantBuffer<Material> gMaterial : register(b0);

// Root Parameter 2 (t0) に対応する Texture SRV
Texture2D<float4> gTexture : register(t0);
Texture2D<float4> gMaskTexture : register(t1); // Dissolve用マスク
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input) {
    PixelShaderOutput output;
    
    // UV変換を適用
    float2 uv = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform).xy;
    
    float edge = 0.0f;
    if (gMaterial.enableDissolve != 0)
    {
        float mask = gMaskTexture.Sample(gSampler, uv).r;
        if (mask <= gMaterial.dissolveThreshold)
        {
            discard;
        }
        edge = 1.0f - smoothstep(gMaterial.dissolveThreshold, gMaterial.dissolveThreshold + gMaterial.dissolveEdgeRange, mask);
    }
    
    // テクスチャの色を取得
    float4 textureColor = gTexture.Sample(gSampler, uv);
    
    // マテリアルの色と掛け合わせる
    float4 finalColor = textureColor * gMaterial.color;
    
    // 結果をPixelShaderOutputに格納
    output.color = finalColor;
    
    if (gMaterial.enableDissolve != 0)
    {
        output.color.rgb += edge * gMaterial.dissolveEdgeColor;
    }
    
    // アルファテスト
    if (output.color.a < 0.01f) {
        discard;
    }
    
    return output;
}