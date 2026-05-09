
struct VertexInput {
    float2 position;
    float3 color;
};

struct UniformVertexBuffer {
    float4x4 model;
    float4x4 view;
    float4x4 proj;
};
ConstantBuffer<UniformVertexBuffer> ubo;

struct VertexOutput {
    float4 pos : SV_Position;
    float3 color;
};

[shader("vertex")]
VertexOutput vert_main(VertexInput input) {
    VertexOutput output;

    float4 pos = float4(input.position, 0, 1);
    pos = mul(ubo.model, pos);
    pos = mul(ubo.view, pos);
    pos = mul(ubo.proj, pos);

    output.pos = pos;
    output.color = input.color;

    return output;
}

[shader("fragment")]
float4 frag_main(VertexOutput in_vert) : SV_Target
{
    float3 color = in_vert.color;

    return float4(color, 1.0);
}
