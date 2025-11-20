#include "Common.hlsli"

// Root Parameter 1 (b1) に対応するTransformationMatrix CBV
ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b1);

VertexShaderOutput main(VertexShaderInput input) {
    VertexShaderOutput output;
    
    // WVPを適用し、クリップ座標へ変換
    output.position = mul(input.position, gTransformationMatrix.WVP);
    
    // テクスチャ座標をそのままPSへ渡す
    output.texcoord = input.texcoord;
    
    // 法線は計算に不要だが、structの要件を満たすためにダミー値を渡す
    output.normal = float3(0.0f, 0.0f, -1.0f);
    
    return output;
}