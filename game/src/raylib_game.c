#include "raylib.h"
#include "nbody.h"
#include "screens.h"    // NOTE: Declares global (extern) variables and screens functions
// #include "vector3.h"
#include "raymath.h"

#include <omp.h>
#include <pthread.h>

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

#define FPS 100

#define NUM_THREADS 8

// extern "C"
// {
//   void init_nbody();
//   void update_nbody();
//   void draw_nbody();
//   void close_nbody();
// }

static const int g_radius = 1;
static const float g_force = 1000.0f;
static const float g_vel = 10.0f;
static float g_zoom = 1.0f;
pthread_t g_threads[NUM_THREADS];

static GPUGameObjects g_gameObjects;

static Camera2D g_Camera = { { 0.0f, 0.0f }, { 0.0f, 0.0f }, 1.0f, 1.0f };
static RenderTexture2D g_Renderer;

void
InitGameObjects(GameObjects *gameObjects, int numObjs)
{
  gameObjects->numObjs = numObjs;
  gameObjects->positions = (Vector2 *)MemAlloc(sizeof(Vector2) * numObjs);
  gameObjects->velocities = (Vector2 *)MemAlloc(sizeof(Vector2) * numObjs);
  gameObjects->abilities = (float *)MemAlloc(gameObjects->numObjs * sizeof(float));
  for (int i = 0; i < gameObjects->numObjs; i++)
  {
    gameObjects->positions[i] = (Vector2)
    {
      GetRandomValue(0, GetScreenWidth()), GetRandomValue(0, GetScreenHeight())
    };
    gameObjects->velocities[i] = (Vector2)
    {
      GetRandomValue(-1, 1), GetRandomValue(-1, 1)
    };
    gameObjects->abilities[i] = GetRandomValue(0.0f, 1.0f);
  }
}

void
InitGPUGameObjects(GPUGameObjects *gameObjects, int numObjs)
{
  gameObjects->numObjs = numObjs;
  gameObjects->pos = (vec2 *)MemAlloc(sizeof(double) * numObjs);
  gameObjects->vel = (vec2 *)MemAlloc(sizeof(double) * numObjs);
  for (int i = 0; i < gameObjects->numObjs; i++)
  {
    gameObjects->pos[i].x = GetScreenWidth() / 2.0f;
    gameObjects->pos[i].y = GetScreenHeight() / 2.0f;
    float angle = M_PI * 2.0f * (float)i / (float)gameObjects->numObjs;
    gameObjects->vel[i].x = cos(angle) * g_vel;
    gameObjects->vel[i].y = sin(angle) * g_vel;
  }
}

void
CloseGameObjects(GPUGameObjects *gameObjects)
{
  MemFree(gameObjects->pos);
  MemFree(gameObjects->vel);
}

void
GPUUpdate()
{
  for (int i = 0; i < g_gameObjects.numObjs; ++i)
  {
    // TraceLog(LOG_INFO, "Thread %d", omp_get_thread_num());
    g_gameObjects.pos[i].x += g_gameObjects.vel[i].x;
    g_gameObjects.pos[i].y += g_gameObjects.vel[i].y;
    if (g_gameObjects.pos[i].x < 0)
    {
      g_gameObjects.pos[i].x = GetScreenWidth();
    }
    else if (g_gameObjects.pos[i].x > GetScreenWidth())
    {
      g_gameObjects.pos[i].x = 0;
    }
    if (g_gameObjects.pos[i].y < 0)
    {
      g_gameObjects.pos[i].y = GetScreenHeight();
    }
    else if (g_gameObjects.pos[i].y > GetScreenHeight())
    {
      g_gameObjects.pos[i].y = 0;
    }

    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON))
    {
      // Vector2 mousePos = (Vector2){ GetMouseX(), -GetMouseY() };
      Vector2 mousePos = (Vector2)
      {
        GetMouseX(), GetMouseY()
      };
      Vector2 direction = Vector2Subtract(mousePos, (Vector2)
      {
        g_gameObjects.pos[i].x, g_gameObjects.pos[i].y
      });
      float distance = Vector2Length(direction);
      if (distance > 0)
      {
        direction = Vector2Normalize(direction);
        g_gameObjects.vel[i].x = direction.x * g_force / (distance * distance);
        g_gameObjects.vel[i].y = direction.y * g_force / (distance * distance);
      }
    }

    if (GetMouseWheelMove() > 0)
    {
      // Zoom in
      g_Camera.target.x += g_Camera.offset.x * 0.1f;
      g_Camera.target.y += g_Camera.offset.y * 0.1f;
      g_Camera.offset.x *= 0.9f;
      g_Camera.offset.y *= 0.9f;
      g_zoom *= 0.9f;
    }
    else if (GetMouseWheelMove() < 0)
    {
      // Zoom out
      g_Camera.target.x -= g_Camera.offset.x * 0.1f;
      g_Camera.target.y -= g_Camera.offset.y * 0.1f;
      g_Camera.offset.x *= 1.1f;
      g_Camera.offset.y *= 1.1f;
      g_zoom *= 1.1f;
    }

    // Slow down
    g_gameObjects.vel[i].x *= 0.99f;
    g_gameObjects.vel[i].y *= 0.99f;
  }

  gpu_nbody(&g_gameObjects, g_gameObjects.numObjs, GetFrameTime() * 0.01f);
}

#if 0
void
Update()
{
  #pragma omp parallel for
  for (int i = 0; i < g_gameObjects.numObjs; ++i)
  {
    // TraceLog(LOG_INFO, "Thread %d", omp_get_thread_num());
    g_gameObjects.positions[i].x += g_gameObjects.velocities[i].x;
    g_gameObjects.positions[i].y += g_gameObjects.velocities[i].y;
    if (g_gameObjects.positions[i].x < 0)
    {
      g_gameObjects.positions[i].x = GetScreenWidth();
    }
    else if (g_gameObjects.positions[i].x > GetScreenWidth())
    {
      g_gameObjects.positions[i].x = 0;
    }
    if (g_gameObjects.positions[i].y < 0)
    {
      g_gameObjects.positions[i].y = GetScreenHeight();
    }
    else if (g_gameObjects.positions[i].y > GetScreenHeight())
    {
      g_gameObjects.positions[i].y = 0;
    }

    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON))
    {
      Vector2 mousePos = GetMousePosition();
      Vector2 direction = Vector2Subtract(mousePos, g_gameObjects.positions[i]);
      float distance = Vector2Length(direction);
      if (distance > 0)
      {
        direction = Vector2Normalize(direction);
        g_gameObjects.velocities[i] = Vector2Add(g_gameObjects.velocities[i], Vector2Scale(direction,
                                                                                           g_force / (distance * distance)));
      }
    }

    if (IsMouseButtonDown(MOUSE_RIGHT_BUTTON))
    {
      Vector2 mousePos = GetMousePosition();
      Vector2 direction = Vector2Subtract(mousePos, g_gameObjects.positions[i]);
      float distance = Vector2Length(direction);
      if (distance > 0)
      {
        direction = Vector2Normalize(direction);
        g_gameObjects.velocities[i] = Vector2Subtract(g_gameObjects.velocities[i], Vector2Scale(direction,
                                                                                                g_force / (distance * distance)));
      }
    }

#ifndef CUDA_FOUND
    // Attraction to all other objects
    for (int j = 0; j < g_gameObjects.numObjs; ++j)
    {
      if (i != j)
      {
        Vector2 direction = Vector2Subtract(g_gameObjects.positions[j], g_gameObjects.positions[i]);
        float distance = Vector2Length(direction);
        if (distance > 0)
        {
          direction = Vector2Normalize(direction);
          g_gameObjects.velocities[i] = Vector2Add(g_gameObjects.velocities[i], Vector2Scale(direction, 0.001f));
        }
      }
    }
#endif

    // Slow down
    g_gameObjects.velocities[i] = Vector2Scale(g_gameObjects.velocities[i], 0.999f);
  }

#ifdef CUDA_FOUND
  gpu_nbody(&g_gameObjects, g_gameObjects.numObjs);
#endif
}
#endif

#if 0
void *
UpdatePthread(void *thread_id)
{
  // ignore pointer-to-int cast
  //
  while (!WindowShouldClose())
  {
    long tid = (long)thread_id;
    int stride = g_gameObjects.numObjs / NUM_THREADS;
    // Use parallelism to update game objects
    // with pthreads since it's faster than OpenMP
    // and it's not supported on WebGL

    for (int i = tid; i < g_gameObjects.numObjs; i += stride)
    {
      // TraceLog(LOG_INFO, "Thread %d", omp_get_thread_num());
      g_gameObjects.positions[i].x += g_gameObjects.velocities[i].x;
      g_gameObjects.positions[i].y += g_gameObjects.velocities[i].y;
      if (g_gameObjects.positions[i].x < 0)
      {
        g_gameObjects.positions[i].x = GetScreenWidth();
      }
      else if (g_gameObjects.positions[i].x > GetScreenWidth())
      {
        g_gameObjects.positions[i].x = 0;
      }
      if (g_gameObjects.positions[i].y < 0)
      {
        g_gameObjects.positions[i].y = GetScreenHeight();
      }
      else if (g_gameObjects.positions[i].y > GetScreenHeight())
      {
        g_gameObjects.positions[i].y = 0;
      }

      if (IsMouseButtonDown(MOUSE_LEFT_BUTTON))
      {
        // Vector2 mousePos = GetMousePosition();
        // if (CheckCollisionPointCircle(mousePos, g_gameObjects.positions[i], g_radius))
        // {
        //   g_gameObjects.abilities[i] = 1.0f;
        // }
        // Attract to mouse position
        Vector2 mousePos = GetMousePosition();
        Vector2 direction = Vector2Subtract(mousePos, g_gameObjects.positions[i]);
        float distance = Vector2Length(direction);
        if (distance > 0)
        {
          direction = Vector2Normalize(direction);
          g_gameObjects.velocities[i] = Vector2Add(g_gameObjects.velocities[i], Vector2Scale(direction,
                                                                                             1.0f / (distance * distance)));
        }
      }

      // Attraction to all other objects
      for (int j = 0; j < g_gameObjects.numObjs; ++j)
      {
        if (i != j)
        {
          Vector2 direction = Vector2Subtract(g_gameObjects.positions[j], g_gameObjects.positions[i]);
          float distance = Vector2Length(direction);
          if (distance > 0)
          {
            direction = Vector2Normalize(direction);
            g_gameObjects.velocities[i] = Vector2Add(g_gameObjects.velocities[i], Vector2Scale(direction, 0.001f));
          }
        }
      }

      // Slow down
      g_gameObjects.velocities[i] = Vector2Scale(g_gameObjects.velocities[i], 0.999f);
    }
  }

  pthread_exit(NULL);
}
#endif

void
Render()
{
  // BeginTextureMode(g_Renderer);
  // ClearBackground(RAYWHITE);
  // for (int i = 0; i < g_gameObjects.numObjs; ++i)
  // {
  //   DrawCircle(g_gameObjects.pos[i].x, g_gameObjects.pos[i].y, g_radius * g_zoom, RED);
  // }
  // EndTextureMode();
}

void
Draw()
{
  Rectangle src = {0, 0, (float)g_Renderer.texture.width, -(float)g_Renderer.texture.height};
  Rectangle dst = {0, 0, GetScreenWidth(), GetScreenHeight()};
  Vector2 origin = {0.0f, 0.0f};
  BeginDrawing();
  ClearBackground(RAYWHITE);
  BeginMode2D(g_Camera);
  {
    for (int i = 0; i < g_gameObjects.numObjs; ++i)
    {
      DrawCircle(g_gameObjects.pos[i].x, g_gameObjects.pos[i].y, g_radius * g_zoom, RED);
    }
    // DrawTexturePro(g_Renderer.texture, src, dst, origin, 0.0f, WHITE);
    DrawText(TextFormat("Number of objects: %d", g_gameObjects.numObjs), 10, 10, 20, BLACK);
    DrawText(TextFormat("FPS: %d", GetFPS()), 10, 30, 20, BLACK);
  }
  EndMode2D();
  EndDrawing();
}

int
main(void)
{
  const int screenWidth = 800;
  const int screenHeight = 450;

  // Check if openmp is supported
  // TraceLog(LOG_WARNING, "OpenMP procs: %d", omp_get_num_procs());

  InitWindow(screenWidth, screenHeight, "battlesim");

  // Initialize renderer
  g_Renderer = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());

  SetTargetFPS(FPS);

  InitGPUGameObjects(&g_gameObjects, 30000);

  // Initialize threads
  // for (long i = 0; i < NUM_THREADS; ++i)
  // {
  //   pthread_create(&g_threads[i], NULL, &UpdatePthread, (void *)i);
  // }

  TraceLog(LOG_INFO, "Main thread: %ld", pthread_self());

  while (!WindowShouldClose())
  {
    // Update
    GPUUpdate();

    // Draw
    Render();
    Draw();
  }

  CloseGameObjects(&g_gameObjects);

  CloseWindow();

  return 0;
}
