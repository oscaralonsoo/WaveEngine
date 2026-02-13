struct Out
{
    float4 position: SV_Position;
    float2 uv: TEXCOORD0;
};

static float2 positions[] =
{
    { -1.0f, 1.0f },
    { 1.0f, 1.0f },
    { -1.0f, -1.0f },
    { -1.0f, -1.0f },
    { 1.0f, 1.0f },
    { 1.0f, -1.0f }
};

static float2 uvs[] =
{
    { 0.0f, 0.0f },
    { 1.0f, 0.0f },
    { 0.0f, 1.0f },
    { 0.0f, 1.0f },
    { 1.0f, 0.0f },
    { 1.0f, 1.0f }
};

Out main(in uint id: SV_VertexID)
{
    Out o;
    o.position = float4(positions[id], 1.0f, 1.0f);
    o.uv = uvs[id];
    return o;
}
