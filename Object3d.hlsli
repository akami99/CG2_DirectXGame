
struct VertexShaderOutput {
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
};

struct DirectionalLight {
    float4 color; //!< ライトの色
    float3 direction; //!< ライトの向き(正規化する)
    float intensity; //!< 輝度
    float3 padding; // float3 + float = 16バイトになるよう調整
};
ConstantBuffer<DirectionalLight> gDirectionalLight : register(b1);