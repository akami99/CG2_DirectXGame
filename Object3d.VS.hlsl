#include "Object3d.hlsli"

struct TransformmationMatrix {
    float4x4 WVP;
    float4x4 World;
};
ConstantBuffer<TransformmationMatrix> gTransformmationMatrix : register(b0);

struct VertexShaderInput {
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
};

VertexShaderOutput main(VertexShaderInput input) {
    VertexShaderOutput output;
    output.position = mul(input.position, gTransformmationMatrix.WVP);
    output.texcoord = input.texcoord;
    output.normal = normalize(mul(input.normal, (float3x3) gTransformmationMatrix.World));
    return output;
}