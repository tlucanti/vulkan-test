[[vk::push_constant]]
cbuffer ShadertoyUniforms
{
    float time;
};

float4 main_image(float2 coord, float time)
{
    float2 p = coord * 0.01;
    float3 color = 0.5 + 0.5 * cos(time + float3(p.x, p.y, p.x + p.y));
    return float4(color, 1.0);
}

[shader("vertex")]
float4 vert_main(uint vertex_id : SV_VertexID) : SV_Position
{
    float2 positions[3] = {
        float2( 0.0f, -0.5f),
        float2( 0.5f,  0.5f),
        float2(-0.5f,  0.5f),
    };

    return float4(positions[vertex_id], 0.0, 1.0);
}

[shader("fragment")]
float4 frag_main(float4 sv_position : SV_Position) : SV_Target
{
    return main_image(sv_position.xy, time);
}
