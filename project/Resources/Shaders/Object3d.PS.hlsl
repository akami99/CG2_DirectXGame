#include "Common.hlsli"

ConstantBuffer<Material> gMaterial : register(b0);
ConstantBuffer<DirectionalLight> gDirectionalLight : register(b1);
ConstantBuffer<Camera> gCamera : register(b2);
ConstantBuffer<PointLight> gPointLight : register(b3);
ConstantBuffer<SpotLight> gSpotLight : register(b4);

Texture2D<float4> gTexture : register(t0);
TextureCube<float4> gEnvironmentTexture : register(t1);
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
        float3 normal = normalize(input.normal); // 法線ベクトル (N)
        float3 toEye = normalize(gCamera.worldPosition - input.worldPosition); // 視線ベクトル (V)
        
        // ===============================================
        //  平行光源 (Directional Light) の計算
        // ===============================================
        // ライトに向かうベクトル (－ライトの方向)
        float3 directionToDirLight = normalize(-gDirectionalLight.direction);
        
        // 拡散反射 (Half-Lambert)
        float NdotL_Dir = dot(normal, directionToDirLight);
        float cos_Dir = pow(NdotL_Dir * 0.5f + 0.5f, 2.0f);
        float3 diffuseDirectionalLight = gMaterial.color.rgb * textureColor.rgb * gDirectionalLight.color.rgb * cos_Dir * gDirectionalLight.intensity;

        // 鏡面反射 (Blinn-Phong)
        float3 halfVector_Dir = normalize(directionToDirLight + toEye); // ハーフベクトル
        float NdotH_Dir = dot(normal, halfVector_Dir); // 法線とハーフベクトルの内積
        float specularPow_Dir = pow(saturate(NdotH_Dir), gMaterial.shininess); // 反射強度
        float3 specularDirectionalLight = gDirectionalLight.color.rgb * gDirectionalLight.intensity * specularPow_Dir * float3(1.0f, 1.0f, 1.0f); // 鏡面反射の色
        
        // ===============================================
        //  ポイントライト (Point Light) の計算
        // ===============================================
        // ライトに向かうベクトル (ライト座標 - ピクセル座標)
        float3 directionToPointLight = normalize(gPointLight.position - input.worldPosition);

        // 距離による減衰
        float distanceToPointLight = length(gPointLight.position - input.worldPosition); // ライトからピクセルまでの距離
        float factor = pow(saturate(-distanceToPointLight / gPointLight.radius + 1.0f), gPointLight.decay); // 減衰率を考慮した減衰係数

        // 拡散反射 (Half-Lambert) : ポイントライト専用の角度(NdotL)を計算する
        float NdotL_Point = dot(normal, directionToPointLight);
        float cos_Point = pow(NdotL_Point * 0.5f + 0.5f, 2.0f);
        float3 diffusePointLight = gMaterial.color.rgb * textureColor.rgb * gPointLight.color.rgb * cos_Point * gPointLight.intensity * factor;

        // 鏡面反射 (Blinn-Phong) : ポイントライト専用のハーフベクトルを計算する
        float3 halfVector_Point = normalize(directionToPointLight + toEye);
        float NdotH_Point = dot(normal, halfVector_Point);
        float specularPow_Point = pow(saturate(NdotH_Point), gMaterial.shininess);
        float3 specularPointLight = gPointLight.color.rgb * gPointLight.intensity * specularPow_Point * float3(1.0f, 1.0f, 1.0f) * factor;
        
        // ===============================================
        //  スポットライト (Spot Light) の計算
        // ===============================================
        // ライトに向かうベクトル
        float3 directionToSpotLight = normalize(gSpotLight.position - input.worldPosition);

        // 距離による減衰 (ポイントライトと同じ計算)
        float distanceToSpotLight = length(gSpotLight.position - input.worldPosition);
        float attenuationFactor = pow(saturate(-distanceToSpotLight / gSpotLight.distance + 1.0f), gSpotLight.decay);

        // 角度による減衰 (コーンの計算)
        // ライトの照射方向と、ピクセルへ向かう方向の余弦(cos)を求める
        float3 spotLightDirectionOnFace = normalize(input.worldPosition - gSpotLight.position);
        float cosAngle = dot(spotLightDirectionOnFace, normalize(gSpotLight.direction));
        
        // 資料の計算式：指定された角度(cosAngle)から中心(1.0)に向かって減衰させる
        float falloffFactor = saturate((cosAngle - gSpotLight.cosAngle) / (gSpotLight.cosFalloffStart - gSpotLight.cosAngle));
        
        // 最終的な減衰係数
        float spotFactor = attenuationFactor * falloffFactor;

        // 拡散反射 (Half-Lambert)
        float NdotL_Spot = dot(normal, directionToSpotLight);
        float cos_Spot = pow(NdotL_Spot * 0.5f + 0.5f, 2.0f);
        float3 diffuseSpotLight = gMaterial.color.rgb * textureColor.rgb * gSpotLight.color.rgb * cos_Spot * gSpotLight.intensity * spotFactor;

        // 鏡面反射 (Blinn-Phong)
        float3 halfVector_Spot = normalize(directionToSpotLight + toEye);
        float NdotH_Spot = dot(normal, halfVector_Spot);
        float specularPow_Spot = pow(saturate(NdotH_Spot), gMaterial.shininess);
        float3 specularSpotLight = gSpotLight.color.rgb * gSpotLight.intensity * specularPow_Spot * float3(1.0f, 1.0f, 1.0f) * spotFactor;
        
        // ===============================================
        //  環境マップ (Environment Map) の計算
        // ===============================================
        float3 cameraToPosition = normalize(input.worldPosition - gCamera.worldPosition);
        float3 reflectedVector = reflect(cameraToPosition, normal);
        float4 environmentColor = gEnvironmentTexture.Sample(gSampler, reflectedVector);
        float3 environmentReflection = environmentColor.rgb * gMaterial.environmentCoefficient;

        // ===============================================
        //  最終合成
        // ===============================================
        // すべての光を足し合わせる
        output.color.rgb = diffuseDirectionalLight + specularDirectionalLight +
                           diffusePointLight + specularPointLight +
                           diffuseSpotLight + specularSpotLight +
                           environmentReflection;
        output.color.a = gMaterial.color.a * textureColor.a;
        
    } else { // Lightingしない場合。前回までと同じ演算
        output.color = gMaterial.color * textureColor;
    }
    
    if (output.color.a == 0.0f) {
        discard; // Pixelを棄却
    }
    
    return output;
}