
struct UniformVertexBuffer {
    float4x4 model;
    float4x4 view;
    float4x4 proj;
};
ConstantBuffer<UniformVertexBuffer> ubo;

[shader("vertex")]
float4 vert_main(uint vertex_id : SV_VertexID) : SV_Position
{
    const float2[3] positions = {
        float2(-1.0f, -1.0f),
        float2( 1.0f, -1.0f),
        float2(-1.0f,  1.0f)
    };

    return float4(positions[vertex_id], 0.0f, 1.0f);
}

[shader("fragment")]
float4 frag_main(float4 sv_position : SV_Position) : SV_Target
{
    float3 color = float3(1.0f, 0.0f, 0.0f);

    return float4(color, 1.0);
}
