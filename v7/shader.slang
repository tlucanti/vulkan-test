
struct UniformVertexBuffer {
    uint2 resolution;
    float2 center;
    float zoom;
};
ConstantBuffer<UniformVertexBuffer> ubo;

float2 cmul(float2 a, float2 b)
{
    return float2(
        a.x * b.x - a.y * b.y,
        a.x * b.y + a.y * b.x
    );
}

float3 main(float2 coord, float2 center)
{
    const int ITER = 50;
    const float OUT = 4.0f;

    float2 z = float2(0.0f, 0.0f);
    int i;

    for (i = 0; i < ITER; i++) {
        z = cmul(z, z) + coord;
        if (z.x * z.x + z.y * z.y > OUT) {
            break;
        }
    }

    float c = 1.0f - (float)i / ITER;
    float3 color = float3(c, c, c);

    return color;
}

[shader("vertex")]
float4 vert_main(uint vertex_id : SV_VertexID) : SV_Position
{
    const float2[3] positions = {
        float2(-1.0f, -1.0f),
        float2( 3.0f, -1.0f),
        float2(-1.0f,  3.0f)
    };

    return float4(positions[vertex_id], 0.0f, 1.0f);
}

[shader("fragment")]
float4 frag_main(float4 sv_position : SV_Position) : SV_Target
{
    float2 resolution = float2(ubo.resolution);
    float2 coord = sv_position.xy / resolution;
    coord = coord * 2.0f - 1.0f;
    coord.x *= resolution.x / resolution.y;

    coord /= ubo.zoom;
    coord -= ubo.center;

    float3 color = main(coord, ubo.center);
    return float4(color, 1.0f);
}
