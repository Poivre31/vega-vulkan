RWTexture2D<float3> source;
RWTexture2D<float3> destination;

const static int N = 4;

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
  return select(x <= 0.0031308, 12.92 * x, 1.055 * pow(x, 1. / 2.4) - 0.055);
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

  float3 acc       = float3(0);
  float fac        = 0;
  float sigma      = 2;
  float exp_factor = 1. / (2 * sigma * sigma);
  for (int i = -N; i <= N; i++) {
    for (int j = -N; j <= N; j++) {
      float weight = exp(-float(i * i + j * j) * exp_factor);

      acc += weight
             * source[clamp(int2(threadId.xy) + int2(j, i), int2(0), int2(width, height) - int2(1))]
                   .rgb;
      fac += weight;
    }
  }
  acc /= fac;

  uint seed                = seed2Dto1D(threadId.xy);
  float3 noise             = 0.002 * randFloat3(-1, 1, &seed);
  destination[threadId.xy] = linear_to_srgb(acc) + noise;
}
