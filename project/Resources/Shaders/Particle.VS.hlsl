#include "Common.hlsli"

StructuredBuffer<TransformationMatrix> gTransformationMatrices : register(t0);

VertexShaderOutput main(VertexShaderInput input, uint instanceID : SV_InstanceID) {
    VertexShaderOutput output;
    output.position = mul(input.position, gTransformationMatrices[instanceID].WVP);
    output.texcoord = input.texcoord;
    output.normal = normalize(mul(input.normal, (float3x3) gTransformationMatrices[instanceID].World));
    return output;
}