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
        
        // --- 2. 鏡面反射 (Phong Reflection) ---
        // 視線ベクトル (カメラ位置 - ピクセル位置)
        float3 toEye = normalize(gCamera.worldPosition - input.worldPosition);
        // 反射ベクトル (reflect関数は 入射方向, 法線 を取る)
        float3 reflectLight = reflect(lightDirection, normal);
        
        // 反射強度 = (反射ベクトル ・ 視線ベクトル) の shininess乗
        float RdotE = dot(reflectLight, toEye);
        float specularPow = pow(saturate(RdotE), gMaterial.shininess);
        
        // 鏡面反射の色 (ここでは白とする)
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