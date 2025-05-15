#include "Object3d.hlsli"

cbuffer MaterialCB : register(b0) {
    float4 gMaterialColor;
}

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput {
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input) {
    PixelShaderOutput output;
    
    float4 textureColor = gTexture.Sample(gSampler, input.texcoord);
    
    output.color = gMaterialColor * textureColor;
    return output;
}