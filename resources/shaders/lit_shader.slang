
struct PushConstants {
  //   float4x4 object_matrix;
  float4x4 view_projection_matrix;
  Material* materials;
  uint32_t material_count;
  float time;
};
[[vk::push_constant]]
ConstantBuffer<PushConstants> push_constants;

uniform Sampler2D textures[];

struct Material {
  float4 albedo;
  uint texture_id;
};

// uniform StructuredBuffer<Material>.Handle materials;

static float3 light_dir     = float3(-0.2, -0.3, -1);
static float4 light_color   = float4(1., 0.8, 0.6,1);
static float4 ambient_color = float4(0.03, 0.08, 0.2,1);

struct VSInput {
  float3 position;
  float3 normal;
  float2 uv;
  uint32_t material_id;
};

struct VertexOutput {
  float4 position : SV_Position;
  float3 normal;
  float2 uv;
  nointerpolation Material material;
};

uint randIntU(uint seed) {
  // LCG values from Numerical Recipes
  seed = 1664525 * seed + 1013904223;
  return seed;
}

float randFloat(float a, float b, uint seed) {
  uint randI = randIntU(seed);
  float x    = float(randI) / (-1U);
  return a + (b - a) * x;
}

float3 randFloat3(float a, float b, uint seed) {
  return float3(
      randFloat(a, b, 1242 * seed + 7971),
      randFloat(a, b, 23027 * seed + 23),
      randFloat(a, b, 23910 * seed + 24829)
  );
}

[shader("vertex")]
VertexOutput vertMain(VSInput vertex) {
  VertexOutput output;
  output.position    = float4(vertex.position, 1);
  output.position    = mul(push_constants.view_projection_matrix, output.position);
  output.normal      = vertex.normal;
  output.uv          = vertex.uv;
  // if(vertex.material_id<push_constants.material_count) {
  //   output.material = Material(float3(0,0,1),0);
  // } else {
  //   output.material = Material(1,0);
  // }
  output.material = push_constants.materials[vertex.material_id];
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
  float4 light    = intensity * light_color + ambient_color;

  if(inVert.material.texture_id>=push_constants.material_count && inVert.material.texture_id!=-1U) {
    return float4(0,1,0,1);
  }

  float4 albedo = inVert.material.albedo;
  if(inVert.material.texture_id!=-1U) { 
    albedo = textures[NonUniformResourceIndex(inVert.material.texture_id)]
                        .Sample(float2(inVert.uv.x, 1-inVert.uv.y));
  }
  albedo.a = 1;

  return albedo*light;
  // return float4(inVert.uv, 0.5 + 0.5 * sin(3.14 * push_constants.time / 2), 1);
}
