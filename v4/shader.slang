
struct VertexInput {
    float3 position;
    float2 tex_coord;
};

struct UniformVertexBuffer {
    float4x4 model;
    float4x4 view;
    float4x4 proj;
};
ConstantBuffer<UniformVertexBuffer> ubo;

struct VertexOutput {
    float4 pos : SV_Position;
    float2 frag_coord;
};

[shader("vertex")]
VertexOutput vert_main(VertexInput input) {
    VertexOutput output;

    float4 pos = float4(input.position, 1.0f);
    pos = mul(ubo.model, pos);
    pos = mul(ubo.view, pos);
    pos = mul(ubo.proj, pos);

    output.pos = pos;
    output.frag_coord = input.tex_coord;

    return output;
}

Sampler2D texture;

[shader("fragment")]
float4 frag_main(VertexOutput in_vert) : SV_Target
{
    return texture.Sample(in_vert.frag_coord);
}
