static const float3x3 yuvMat = {
    1.1644,  1.1644, 1.1644,
    0.0000, -0.3918, 2.0172,
    1.5960, -0.8130, 0.0000 };

static const float3 yuvOff = { 0.0625, 0.5000, 0.5000 };

Texture2D lumaTex: register(t0);
Texture2D chromaTex: register(t1);
SamplerState texSampler: register(s0);

float4 main(in float4 pos: SV_Position, in float2 uv: TEXCOORD0) : SV_Target
{
    float3 yuv;
    yuv.x = lumaTex.Sample(texSampler, uv).r;
    yuv.yz = chromaTex.Sample(texSampler, uv).rg;

    yuv -= yuvOff;
    float3 rgb = mul(yuv, yuvMat);

    return float4(rgb.r, rgb.g, rgb.b, 1.0);
}
