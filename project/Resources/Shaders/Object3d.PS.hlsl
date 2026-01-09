#include "Common.hlsli"

ConstantBuffer<Material> gMaterial : register(b0);
ConstantBuffer<DirectionalLight> gDirectionalLight : register(b1);
ConstantBuffer<Camera> gCamera : register(b2);

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

PixelShaderOutput main(VertexShaderOutput input) {
    PixelShaderOutput output;
    
    //float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    //float4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);
    float4 textureColor = gTexture.Sample(gSampler, input.texcoord);
    
    // アルファテスト(2値抜き)
    if (textureColor.a < 0.5f) {
        discard; // Pixelを棄却
    }
    
    if (gMaterial.enableLighting != 0) { // Lightingする場合
        // --- 準備 ---
        float3 normal = normalize(input.normal);
        float3 lightDirection = normalize(gDirectionalLight.direction);
        
        // --- 1. 拡散反射 (Half-Lambert) ---
        float NdotL = dot(normal, -lightDirection);
        float cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
        float3 diffuse = gMaterial.color.rgb * textureColor.rgb * gDirectionalLight.color.rgb * cos * gDirectionalLight.intensity;
        
        // --- 2. 鏡面反射 (Blinn-Phong Reflection) ---

        // 視線ベクトル (V)
        float3 toEye = normalize(gCamera.worldPosition - input.worldPosition);

        // 入射光の逆ベクトル（光の源へ向かうベクトル）(L)
        float3 toLight = -normalize(gDirectionalLight.direction);

        // 【HalfVectorの計算】 (H = L + V の正規化)
        float3 halfVector = normalize(toLight + toEye);

        // 反射強度 = (法線 ・ ハーフベクトル) の shininess乗
        float NdotH = dot(normal, halfVector);
        float specularPow = pow(saturate(NdotH), gMaterial.shininess);

        // 鏡面反射の色
        float3 specularColor = float3(1.0f, 1.0f, 1.0f);
        float3 specular = gDirectionalLight.color.rgb * gDirectionalLight.intensity * specularPow * specularColor;
        
        // --- 最終合成 ---
        //output.color.rgb = gMaterial.color.rgb * textureColor.rgb * gDirectionalLight.color.rgb * cos * gDirectionalLight.intensity;
        output.color.rgb = diffuse + specular;
        output.color.a = gMaterial.color.a * textureColor.a;
    } else { // Lightingしない場合。前回までと同じ演算
        output.color = gMaterial.color * textureColor;
    }
    
    if (output.color.a == 0.0f) {
        discard; // Pixelを棄却
    }
    
    return output;
}