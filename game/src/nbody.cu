#include <cuda_runtime.h>
#include <raylib.h>
#include "nbody.h"

inline __device__ Vector2
vec2_add(Vector2 a, Vector2 b)
{
  return (Vector2)
  {
    a.x + b.x, a.y + b.y
  };
}

inline __device__ Vector2
vec2_sub(Vector2 a, Vector2 b)
{
  return (Vector2)
  {
    a.x - b.x, a.y - b.y
  };
}

inline __device__ float
vec2_len(Vector2 a)
{
  return sqrt(a.x * a.x + a.y * a.y);
}

inline __device__ Vector2
vec2_norm(Vector2 a)
{
  float len = vec2_len(a);
  return (Vector2)
  {
    a.x / len, a.y / len
  };
}

inline __device__ Vector2
vec2_scale(Vector2 a, float s)
{
  return (Vector2)
  {
    a.x *s, a.y *s
  };
}

__global__ void
nbody_kernel(float *px, float *py, float *vx, float *vy, int numObjs, float dt)
{
  extern __shared__ float shared_x[];
  extern __shared__ float shared_y[];
  const float epsilon = 0.00001f;
  int idx = threadIdx.x + blockIdx.x * blockDim.x;
  int stride = blockDim.x * gridDim.x;

  if (idx < numObjs)
  {
    shared_x[threadIdx.x] = px[idx];
    shared_y[threadIdx.x] = py[idx];
  }
  __syncthreads();

  for (int i = idx; i < numObjs; i += stride)
  {
    for (int j = 0; j < numObjs; ++j)
    {
      if (i != j)
      {
        float dir_x = px[j] - px[i];
        float dir_y = py[j] - py[i];
        float norm = 1 / sqrt(dir_x * dir_x + dir_y * dir_y + epsilon);
        dir_x *= norm;
        dir_y *= norm;
        vx[i] += dir_x * dt;
        vy[i] += dir_y * dt;
      }
    }
  }
}

__global__ void
nbody_kernel2(vec2 *pos, vec2 *vel, int numObjs, float dt)
{
  extern __shared__ vec2 sdata[];
  const float epsilon = 0.00001f;
  int idx = threadIdx.x + blockIdx.x * blockDim.x;
  int stride = blockDim.x * gridDim.x;

  if (idx < numObjs)
  {
    sdata[threadIdx.x] = pos[idx];
  }
  __syncthreads();

  for (int i = idx; i < numObjs; i += stride)
  {
    for (int j = 0; j < idx; ++j)
    {
      float dir_x = pos[j].x - pos[i].x;
      float dir_y = pos[j].y - pos[i].y;
      float norm = 1 / sqrt(dir_x * dir_x + dir_y * dir_y + epsilon);
      dir_x *= norm;
      dir_y *= norm;
      vel[i].x += dir_x * dt;
      vel[i].y += dir_y * dt;
    }

    for (int j = idx; j < idx + 32; ++j)
    {
      if (i != j)
      {
        float dir_x = sdata[j - idx].x - sdata[i - idx].x;
        float dir_y = sdata[j - idx].y - sdata[i - idx].y;
        float norm = 1 / sqrt(dir_x * dir_x + dir_y * dir_y + epsilon);
        dir_x *= norm;
        dir_y *= norm;
        vel[i].x += dir_x * dt;
        vel[i].y += dir_y * dt;
      }
    }

    for (int j = idx + 32; j < numObjs; ++j)
    {
      if (i != j)
      {
        float dir_x = pos[j].x - pos[i].x;
        float dir_y = pos[j].y - pos[i].y;
        float norm = 1 / sqrt(dir_x * dir_x + dir_y * dir_y + epsilon);
        dir_x *= norm;
        dir_y *= norm;
        vel[i].x += dir_x * dt;
        vel[i].y += dir_y * dt;
      }
    }
  }
}

void
gpu_nbody(GPUGameObjects *objects, int numObjs, double dt)
{
  vec2 *d_pxy, *d_vxy;
  cudaMalloc(&d_pxy, numObjs * sizeof(vec2));
  cudaMalloc(&d_vxy, numObjs * sizeof(vec2));
  cudaMemcpy(d_pxy, objects->pos, numObjs * sizeof(vec2), cudaMemcpyHostToDevice);
  cudaMemcpy(d_vxy, objects->vel, numObjs * sizeof(vec2), cudaMemcpyHostToDevice);

  const int blockSize = 512;
  const int numBlocks = (numObjs + blockSize - 1) / blockSize;

  nbody_kernel2 <<< numBlocks, blockSize, sizeof(vec2) * numObjs / numBlocks >>> (d_pxy, d_vxy, numObjs, dt);
  cudaMemcpy(objects->pos, d_pxy, numObjs * sizeof(vec2), cudaMemcpyDeviceToHost);
  cudaMemcpy(objects->vel, d_vxy, numObjs * sizeof(vec2), cudaMemcpyDeviceToHost);
  cudaFree(d_pxy);
  cudaFree(d_vxy);
}
