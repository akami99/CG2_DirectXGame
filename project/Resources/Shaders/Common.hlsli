// This file dont using japanese comments to avoid encording issues. 

// Common structures and definitions for shaders

struct VertexShaderInput {
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
};

struct VertexShaderOutput {
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
};

struct PixelShaderOutput {
    float4 color : SV_TARGET0;
};

struct Material {
    float4 color;
    int enableLighting;
    float4x4 uvTransform;
};

struct TransformationMatrix {
    float4x4 WVP;
    float4x4 World;
};

struct DirectionalLight {
    float4 color;
    float3 direction;
    float _padding_dummy;
    float intensity;
};