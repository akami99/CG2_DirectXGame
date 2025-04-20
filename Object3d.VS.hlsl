struct TransformmationMatrix {
    float4x4 WVP;
};
ConstantBuffer<TransformmationMatrix> gTransformmationMatrix : register(b0);
struct VertexShaderOutput {
    float4 position : SV_POSITION;
};

struct VertexShaderInput {
    float4 position : POSITION0;
};

VertexShaderOutput main(VertexShaderInput input) {
    VertexShaderOutput output;
    output.position = mul(input.position, gTransformmationMatrix.WVP);
    return output;
}