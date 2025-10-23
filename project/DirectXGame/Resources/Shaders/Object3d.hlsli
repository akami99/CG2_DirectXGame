// This file dont using japanese comments to avoid encording issues. 

struct VertexShaderOutput {
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
};

struct DirectionalLight {
    float4 color;
    float3 direction;
    float _padding_dummy;
    float intensity;
};