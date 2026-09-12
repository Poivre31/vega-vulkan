Texture2D<float> depth;
RWTexture2D<float3> source;
RWTexture2D<float3> destination;

struct post_processing_data {
  float time;
  uint frame;

  float dithering_stength;

  int blur_kernel_radius;
  float blur_strength;

  float exposure;
  uint srgb_transform;
  float srgb_gamma;
  float srgb_offset;

  float znear;
  // float zfar;

  uint32_t enable_fog;
  float3 fog_color;
  float fog_falloff;
}

[vk::push_constant]
ConstantBuffer<post_processing_data, ScalarDataLayout> pp;

const static int N = 16;

float depth_nd_to_linear(float d) {
  return pp.znear / d;
  // return 2 * pp.znear * pp.zfar / (pp.zfar + pp.znear - d * (pp.zfar - pp.znear));
}

uint seed2Dto1D(uint2 v) {
  v    = v * 1664525u + 1013904223u;
  v.x += v.y * 1664525u;
  v.y += v.x * 1664525u;
  v   ^= v >> 16u;
  v.x += v.y * 1664525u;
  v.y += v.x * 1664525u;
  v   ^= v >> 16u;
  return v.x;
}

uint randIntU(uint* seed) {
  *seed = *seed * 747796405u + 2891336453u;
  return *seed;
}

float randFloat(float a, float b, uint* seed) {
  uint randI = randIntU(seed);
  float x    = float(randI) / (-1U);
  return a + (b - a) * x;
}

float3 randFloat3(float a, float b, uint* seed) {
  return float3(randFloat(a, b, seed), randFloat(a, b, seed), randFloat(a, b, seed));
}

float3 linear_to_srgb(float3 x) {
  if (pp.srgb_gamma < 1.01) {
    return x;
  }
  float c = pow(
      pp.srgb_gamma * pp.srgb_offset / ((pp.srgb_gamma - 1) * (1 + pp.srgb_offset)), pp.srgb_gamma
  );
  float beta = pp.srgb_offset / (c * (pp.srgb_gamma - 1));
  return select(
      x <= c, beta * x, (1 + pp.srgb_offset) * pow(x, 1. / pp.srgb_gamma) - pp.srgb_offset
  );
  // return select(x <= 0.0031308, 12.92 * x, 1.055 * pow(x, 1. / 2.4) - 0.055);
}
float4 linear_to_srgb(float4 x) {
  return float4(linear_to_srgb(x.rgb), x.a);
}

[shader("compute")]
[numthreads(8, 8, 1)]
void computeMain(uint3 threadId: SV_DispatchThreadID) {
  uint width, height;
  destination.GetDimensions(width, height);

  if (threadId.x >= width || threadId.y >= height) {
    return;
  }

  float3 color = float3(0);
  float var    = pp.blur_strength * pp.blur_strength;
  if (var > 0.1) {
    float total_weight = 0;
    float exp_factor   = 1. / (2 * var);
    for (int i = -pp.blur_kernel_radius; i <= pp.blur_kernel_radius; i++) {
      for (int j = -pp.blur_kernel_radius; j <= pp.blur_kernel_radius; j++) {
        float weight = exp(-float(i * i + j * j) * exp_factor);

        color +=
            weight
            * source[clamp(int2(threadId.xy) + int2(j, i), int2(0), int2(width, height) - int2(1))]
                  .rgb;
        total_weight += weight;
      }
    }
    color /= total_weight;
  } else {
    color = source[threadId.xy];
  }

  if (bool(pp.enable_fog)) {
    float3 fog = pp.fog_color * (1 - exp(-depth_nd_to_linear(depth[threadId.xy]) / pp.fog_falloff));
    color += fog;
  }

  if (bool(pp.srgb_transform)) {
    color = linear_to_srgb(pp.exposure * color);
  }

  uint seed  = seed2Dto1D(threadId.xy) + 1375423u * pp.frame;
  color     += 0.02 * pp.dithering_stength * randFloat3(-1, 1, &seed);

  destination[threadId.xy] = color;

  //   float4 fog = 0.9*float4(0.2,0.4,1.,1);
  // float3 dir = normalize(inVert.world_position-push_constants.camera_position);
  // return albedo * float4(light, 1)+fog*(1-exp(-depth/50))+0.01*float4(light_color *
  // pow(max(-dot(dir,light_dir),0),8),1);
}
