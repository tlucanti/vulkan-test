[[vk::push_constant]]
cbuffer ShadertoyUniforms
{
    float time;
};

float2 cmul(float2 a, float2 b)
{
    return float2(
        a.x * b.x - a.y * b.y,
        a.x * b.y + a.y * b.x
    );
}

float4 main_image(float2 coord, float time)
{
    coord.x -= 0.5;
    float3 color = float3(0, 0, 0);

    const float ITER = 50;
    const float OUT = 4;

    float2 z = float2(0, 0);
    int i;

    for (i = 0; i < ITER; i++) {
        z = cmul(z, z) + coord;
        if (z.x * z.x + z.y * z.y > OUT) {
            break;
        }
    }

    float c = 1.0f - (float)i / ITER;
    color = float3(c, c, c);
    return float4(color, 1.0);
}

[shader("vertex")]
float4 vert_main(uint vertex_id : SV_VertexID) : SV_Position
{
    float2 positions[3] = {
        float2( -1.0f, -1.0f),
        float2( 3.0f,  -1.0f),
        float2(-1.0f,  3.0f),
    };

    return float4(positions[vertex_id], 0.0, 1.0);
}

[shader("fragment")]
float4 frag_main(float4 sv_position : SV_Position) : SV_Target
{
    float2 resolution = float2(800, 600);
    float2 uv = sv_position.xy / resolution;
    float2 coord = uv * 2.0 - 1.0;
    coord.x *= 800.0f / 600.0f;
    return main_image(coord, time);
}
