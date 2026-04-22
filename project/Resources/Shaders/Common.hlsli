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
    float3 worldPosition : POSITION0;
    float4 color : COLOR0;
};

struct PixelShaderOutput {
    float4 color : SV_TARGET0;
};

struct Material {
    float4 color;
    int enableLighting;
    float shininess;
    float environmentCoefficient;
    float _padding_dummy;
    float4x4 uvTransform;
};

struct TransformationMatrix {
    float4x4 WVP;
    float4x4 World;
    float4x4 WorldInverseTranspose;
};

struct DirectionalLight {
    float4 color;
    float3 direction;
    float _padding_dummy;
    float intensity;
};

struct PointLight {
    float4 color;
    float3 position;
    float intensity;
    float radius;
    float decay;
    float _padding_dummy[2];
};

struct SpotLight {
    float4 color;
    float3 position;
    float intensity;
    float3 direction;
    float distance;
    float decay;
    float cosAngle;
    float cosFalloffStart;
    float _padding_dummy;
};

struct Camera {
    float3 worldPosition;
};

struct ParticleInstanceData {
    float4x4 WVP;
    float4x4 World;
    float4 color;
};