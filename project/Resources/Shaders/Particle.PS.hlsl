#include "Particle.hlsli"

ConstantBuffer<Material> gMaterial : register(b0);

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s1); // CLAMP用のサンプラーを使用

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input) {
    PixelShaderOutput output;
    
    float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);
    output.color = gMaterial.color * textureColor * input.color;
    // アルファテスト
    if (textureColor.a == 0.0f) {
        discard; // Pixelを棄却
    }
    
    return output;
}