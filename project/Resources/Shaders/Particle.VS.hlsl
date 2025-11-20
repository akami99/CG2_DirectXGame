#include "Common.hlsli"

StructuredBuffer<ParticleInstanceData> gParticleData : register(t0);

VertexShaderOutput main(VertexShaderInput input, uint instanceID : SV_InstanceID) {
    VertexShaderOutput output;
    output.position = mul(input.position, gParticleData[instanceID].WVP);
    output.texcoord = input.texcoord;
    output.color = gParticleData[instanceID].color;
    output.normal = normalize(mul(input.normal, (float3x3) gParticleData[instanceID].World));
    
    return output;
}