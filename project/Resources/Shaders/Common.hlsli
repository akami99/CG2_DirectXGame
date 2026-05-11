// This file dont using japanese comments to avoid encording issues. 

// Common structures and definitions for shaders

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
