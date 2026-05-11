struct PostProcessOutput {
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD0;
};

struct VertexShaderInput {
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
};

PostProcessOutput main(VertexShaderInput input) {
    PostProcessOutput output;
    output.position = input.position;
    output.texcoord = input.texcoord;
    return output;
}
