#include "Particle.hlsli"

struct TransformationMatrix {
    float4x4 WVP;
    float4x4 World;
};
StructuredBuffer<TransformationMatrix> gTransformationMatrices : register(t0);

struct VertexShaderInput {
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
};

VertexShaderOutput main(VertexShaderInput input, uint instanceID : SV_InstanceID) {
    VertexShaderOutput output;
    output.position = mul(input.position, gTransformationMatrices[instanceID].WVP);
    output.texcoord = input.texcoord;
    output.normal = normalize(mul(input.normal, (float3x3) gTransformationMatrices[instanceID].World));
    //output.normal = float3(0.0f, 1.0f, 0.0f);
    return output;
}