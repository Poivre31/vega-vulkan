[vk::image_format("rgba8")]
RWTexture2D<float4> source;

[vk::image_format("rgba8")]
RWTexture2D<float4> destination;

const static int N = 32;

[shader("compute")]
[numthreads(8, 8, 1)]
void computeMain(uint3 threadId: SV_DispatchThreadID) {
  uint width, height;
  destination.GetDimensions(width, height);

  if(threadId.x >= width || threadId.y >= height) {
    return;
  }

  float4 acc = float4(0);
  float fac = 0;
  float sigma = 20;
  for(int i = -N; i <=N; i++) {
    [unroll]
    for(int j = -N; j <=N; j++) {
      float weight = exp(-(i*i+j*j)/(2*sigma*sigma));
      acc += weight*source[threadId.xy+int2(j,i)];
      fac += weight;
    } 
  }
  acc /= fac;

  // int N = 8;
  // float4 acc = float4(0);
  // float sigma = 1.5;
  // for(int i = 0; i <N; i++) {
  //   for(int j = 0; j <N; j++) {
  //     acc += source[threadId.xy/int(N)*int(N)+int2(i,j)];
  //   } 
  // }
  // acc /= N*N;

  // float factor = exp(-((int(threadId.x%8)-4)*(int(threadId.x%8)-4)));
  // float total = WaveActiveSum(factor);
  // float4 color = factor * source[threadId.xy];
  // float4 acc = WaveActiveSum(color)/total;

  destination[threadId.xy] = acc;
}
