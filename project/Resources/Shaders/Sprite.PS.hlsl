#include "Common.hlsli"

// Root Parameter 0 (b0) に対応する Material CBV を宣言
ConstantBuffer<Material> gMaterial : register(b0);

// Root Parameter 2 (t0) に対応する Texture SRV
Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

// 戻り値も Common.hlsli で定義した構造体を使う
PixelShaderOutput main(VertexShaderOutput input) {
    PixelShaderOutput output;
    
    // UV変換を適用
    float2 uv = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform).xy;
    
    // テクスチャの色を取得
    float4 textureColor = gTexture.Sample(gSampler, uv);
    
    // マテリアルの色と掛け合わせる
    float4 finalColor = textureColor * gMaterial.color;
    
    // 結果をPixelShaderOutputに格納
    output.color = finalColor;
    
    // スプライトはライティングしないので、シンプルに色を返す
    // アルファテスト
    if (output.color.a < 0.01f) {
        discard;
    }
    
    return output;
}