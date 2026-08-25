
struct PushConstants {
  //   float4x4 object_matrix;
  float4x4 view_projection_matrix;
  float time;
};
[[vk::push_constant]]
ConstantBuffer<PushConstants> push_constants;

static float3 light_dir     = float3(0.2, -0.3, -1);
static float3 light_color   = float3(1., 0.8, 0.6);
static float3 ambient_color = float3(0.05, 0.1, 0.2);

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
VertexOutput vertMain(VSInput vertex) {
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
  float intensity = clamp(dot(inVert.normal, -light_dir) / dot(light_dir, light_dir), 0, 1);
  float3 color    = intensity * light_color + (1 - intensity) * ambient_color;
  return float4(color, 1);
  // return float4(inVert.uv, 0.5 + 0.5 * sin(3.14 * push_constants.time / 2), 1);
}
