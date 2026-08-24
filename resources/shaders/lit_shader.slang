
struct PushConstants {
  //   float4x4 object_matrix;
  float4x4 view_projection_matrix;
  float time;
};
[[vk::push_constant]]
ConstantBuffer<PushConstants> push_constants;

static float3 positions[3] =
    float3[](float3(1.0, 0.0, 0.25), float3(1.0, 0.5, -0.25), float3(1.0, -0.5, -0.25));
static float2 uvs[3] = float2[](float2(0., 0.), float2(1., 0.), float2(0., 1.));

// struct VSInput {
//   float3 position;
//   float3 normal;
//   float2 uv;
// };

struct VertexOutput {
  float4 position : SV_Position;
  // float3 normal;
  float2 uv;
};

[shader("vertex")]
VertexOutput vertMain(uint vertex_id: SV_VertexID) {
  VertexOutput output;
  output.position = float4(positions[vertex_id], 1);
  output.position = mul(push_constants.view_projection_matrix, output.position);
  // output.normal   = input.normal;
  output.uv       = uvs[vertex_id];
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
  return float4(inVert.uv, 0.5 + 0.5 * sin(3.14 * push_constants.time / 2), 1);
}
