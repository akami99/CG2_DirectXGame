
struct DirectionalLight
{
    float4 color;
    float3 direction;
    float _padding_dummy;
    float intensity;
};

struct PointLight
{
    float4 color;
    float3 position;
    float intensity;
    float radius;
    float decay;
    float _padding_dummy[2];
};

struct SpotLight
{
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

struct Camera
{
    float3 worldPosition;
};