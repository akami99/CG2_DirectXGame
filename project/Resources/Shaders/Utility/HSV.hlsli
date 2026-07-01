// Range Clamping (for periodic color rotation)
float WrapValue(float value, float minRange, float maxRange)
{
    float range = maxRange - minRange;
    float modValue = fmod(value - minRange, range);
    if (modValue < 0.0f)
    {
        modValue += range;
    }
    return minRange + modValue;
}
// RGB to HSV conversion
float3 RGBToHSV(float3 rgb)
{
    float4 K = float4(0.0f, -1.0f / 3.0f, 2.0f / 3.0f, -1.0f);
    float4 p = rgb.g < rgb.b ? float4(rgb.bg, K.wz) : float4(rgb.gb, K.xy);
    float4 q = rgb.r < p.x ? float4(p.xyw, rgb.r) : float4(rgb.r, p.yzx);
    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10f;
    return float3(abs(q.z + (q.w - q.y) / (6.0f * d + e)), d / (q.x + e), q.x);
}
// HSV to RGB conversion
float3 HSVToRGB(float3 hsv)
{
    float4 K = float4(1.0f, 2.0f / 3.0f, 1.0f / 3.0f, 3.0f);
    float3 p = abs(frac(hsv.xxx + K.xyz) * 6.0f - K.www);
    return hsv.z * lerp(K.xxx, saturate(p - K.xxx), hsv.y);
}