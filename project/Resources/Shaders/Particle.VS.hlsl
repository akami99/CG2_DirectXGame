#include "Particle.hlsli"

StructuredBuffer<ParticleInstanceData> gParticleData : register(t0);

VertexShaderOutput main(VertexShaderInput input, uint instanceID : SV_InstanceID) {
    VertexShaderOutput output;
    output.position = mul(input.position, gParticleData[instanceID].WVP);
    
    float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gParticleData[instanceID].uvTransform);
    output.texcoord = transformedUV.xy;
    
    output.color = gParticleData[instanceID].color;
    output.normal = normalize(mul(input.normal, (float3x3) gParticleData[instanceID].World));
    
    return output;
}