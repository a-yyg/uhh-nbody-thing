#ifndef NBODY_H
#define NBODY_H

#include "raylib.h"

typedef struct
{
  int numObjs;
  Vector2 *positions;
  Vector2 *velocities;
  float *abilities;
} GameObjects;

typedef struct
{
  union
  {
    struct
    {
      float x;
      float y;
    };
    double _data;
  };
} vec2;

float getx(vec2 v);
float gety(vec2 v);

typedef struct
{
  int numObjs;
  vec2 *pos;
  vec2 *vel;
} GPUGameObjects;

#ifdef __cplusplus
extern "C"
{
#endif
void gpu_nbody(GPUGameObjects *objects, int numObjs, double dt);
#ifdef __cplusplus
}
#endif

#endif
