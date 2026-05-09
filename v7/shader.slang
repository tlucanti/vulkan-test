struct UniformVertexBuffer {
    uint2 resolution;
    uint2 resolution_padding;
    double2 center;
    double zoom;
    double zoom_padding;
    int iter;
};
ConstantBuffer<UniformVertexBuffer> ubo;

double2 cmul(double2 a, double2 b)
{
    return double2(
        a.x * b.x - a.y * b.y,
        a.x * b.y + a.y * b.x
    );
}

double3 main(double2 coord)
{
    const int ITER = ubo.iter;
    const double OUT = 4.0;

    double2 z = double2(0.0, 0.0);
    int i;

    for (i = 0; i < ITER; i++) {
        z = cmul(z, z) + coord;
        if (z.x * z.x + z.y * z.y > OUT) {
            break;
        }
    }

    double c = 1.0 - (double)i / ITER;
    double3 color = double3(c, c, c);

    return color;
}

[shader("vertex")]
float4 vert_main(uint vertex_id : SV_VertexID) : SV_Position
{
    const double2[3] positions = {
        double2(-1.0, -1.0),
        double2( 3.0, -1.0),
        double2(-1.0,  3.0)
    };

    return float4(float2(positions[vertex_id]), 0.0, 1.0);
}

[shader("fragment")]
float4 frag_main(float4 sv_position : SV_Position) : SV_Target
{
    double2 resolution = double2(ubo.resolution);
    double2 coord = double2(sv_position.xy) / resolution;
    coord = coord * 2.0 - 1.0;
    coord.x *= resolution.x / resolution.y;

    coord /= ubo.zoom;
    coord -= ubo.center;

    double3 color = main(coord);
    return float4(float3(color), 1.0);
}
