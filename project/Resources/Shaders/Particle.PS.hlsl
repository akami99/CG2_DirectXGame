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
    
    float2 uv = input.texcoord;
    float4 textureColor = gTexture.Sample(gSampler, uv);
    
    float4 vertexColor = float4(1.0f, 1.0f, 1.0f, 1.0f);
    if (gMaterial.isRing != 0 || gMaterial.isCylinder != 0) {
        float r = uv.y;
        float a = uv.x;
        if (gMaterial.isUvSwap != 0) {
            r = uv.x;
            a = uv.y;
        }

        r = saturate(r);
        a = saturate(a);

        vertexColor = lerp(gMaterial.outerColor, gMaterial.innerColor, r);

        if (gMaterial.fadeRange > 0.0f) {
            float fadeFactor = 1.0f;
            if (a < gMaterial.fadeRange) {
                fadeFactor = lerp(gMaterial.fadeStartAlpha, 1.0f, a / gMaterial.fadeRange);
            } else if (a > 1.0f - gMaterial.fadeRange) {
                fadeFactor = lerp(1.0f, gMaterial.fadeEndAlpha, (a - (1.0f - gMaterial.fadeRange)) / gMaterial.fadeRange);
            }
            vertexColor.a *= fadeFactor;
        }
    }
    
    output.color = gMaterial.color * textureColor * input.color * vertexColor;
    
    // アルファテスト
    if (output.color.a <= gMaterial.alphaReference) {
        discard; // Pixelを棄却
    }
    
    return output;
}