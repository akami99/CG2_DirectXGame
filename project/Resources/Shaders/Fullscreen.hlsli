
struct VertexShaderInput
{
    float4 position : POSITION;
};

struct VertexShaderOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

struct TransformationMatrix
{
    float4x4 WVP;
};