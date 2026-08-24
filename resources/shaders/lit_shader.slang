
struct PushConstants {
  //   float4x4 object_matrix;
  float4x4 view_projection_matrix;
  float time;
};
[[vk::push_constant]]
ConstantBuffer<PushConstants> push_constants;

struct VSInput {
  float3 position;
  float3 normal;
  float2 uv;
};

struct VertexOutput {
  float4 position : SV_Position;
  float3 normal;
  float2 uv;
};

[shader("vertex")]
VertexOutput vertMain(VSInput vertex, uint vertex_id: SV_VertexID) {
  VertexOutput output;
  output.position = float4(vertex.position, 1);
  output.position = mul(push_constants.view_projection_matrix, output.position);
  output.normal   = vertex.normal;
  output.uv       = vertex.uv;
  // if(pushConstants.culled) {
  //     output.color *= float3(1.0,0.3,0.2);
  // }
  // if(instances[instanceID].culled) {
  //     output.color *= float3(1.0F,0.1F,0.1F);
  // }
  // output.position = mul(pushConstants.object_matrix, float4(input.position, 1));
  // output.position = mul(pushConstants.view_projection_matrix, output.position);

  return output;
}

[shader("fragment")]
float4 fragMain(VertexOutput inVert) : SV_Target {
  return float4(float3(inVert.position.z / inVert.position.w), 1);
  return float4(inVert.uv, 0.5 + 0.5 * sin(3.14 * push_constants.time / 2), 1);
}
