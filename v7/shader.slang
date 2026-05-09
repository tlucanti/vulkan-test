
struct UniformVertexBuffer {
    uint2 resolution;
    float2 center;
    float zoom;
};
ConstantBuffer<UniformVertexBuffer> ubo;

float3 main(float2 coord, float2 center, float zoom)
{
    if (coord.x * coord.x + coord.y * coord.y < 1) {
        return float3(1.0f, 1.0f, 1.0f);
    } else {
        return float3(0.0, 0.0, 0.0);
    }
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
    float2 resolution = float2(ubo.resolution.x, ubo.resolution.y);
    float2 coord = sv_position.xy / resolution;
    coord = coord * 2.0f - 1.0f;
    coord.x *= resolution.x / resolution.y;

    coord -= ubo.center;

    float3 color = main(coord, ubo.center, ubo.zoom);
    return float4(color, 1.0f);
}
