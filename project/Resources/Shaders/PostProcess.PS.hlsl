struct PostProcessOutput {
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD0;
};

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PostProcessParams {
    float intensity;
};
ConstantBuffer<PostProcessParams> gParams : register(b0);

float4 main(PostProcessOutput input) : SV_TARGET {
    float4 textureColor = gTexture.Sample(gSampler, input.texcoord);
    
    // Example effect: Grayscale based on intensity
    float gray = dot(textureColor.rgb, float3(0.2125f, 0.7154f, 0.0721f));
    float3 finalColor = lerp(textureColor.rgb, float3(gray, gray, gray), gParams.intensity);
    
    return float4(finalColor, textureColor.a);
}
