#include "Fullscreen.hlsli"
#include "Utility/HSV.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSamplerLinear : register(s0);

cbuffer HSVParams : register(b0)
{
    float gHue;
    float gSaturation;
    float gValue;
};

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    
    // 元画像の色をサンプリング
    float4 texColor = gTexture.Sample(gSamplerLinear, input.texcoord);
    
    // RGB から HSV へ変換
    float3 hsv = RGBToHSV(texColor.rgb);
    
    // 各パラメータを加算
    hsv.x += gHue;
    hsv.y += gSaturation;
    hsv.z += gValue;
    
    // 各値の範囲をクランプ (色相は環状、彩度・明度は0~1)
    hsv.x = WrapValue(hsv.x, 0.0f, 1.0f);
    hsv.y = saturate(hsv.y);
    hsv.z = saturate(hsv.z);
    
    // HSV から RGB へ逆変換
    output.color.rgb = HSVToRGB(hsv);
    output.color.a = texColor.a;
    
    return output;
}
