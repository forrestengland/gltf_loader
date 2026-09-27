// standard includes
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>

// sdl
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

// opengl
#include <GLES2/gl2.h>

// custom gltf loader
#include "gltf_loader.h"
#include "gpu.h"

// requested screen size for desktop
#define SCREEN_W 640
#define SCREEN_H 480

#define GLTF_FILE "robot.gltf"
// pi
#define PI 3.1415926535

#define VERTEX_SHADER_FILE "vertex_shader.glsl"
#define FRAGMENT_SHADER_FILE "fragment_shader.glsl"

int main(int argc, char* argv[]) {
  
  printf("it works\n");

  Model model;
  
  load_gltf(GLTF_FILE, &model);
  float* modelVertices = model_vertices_gltf(&model);

  GLuint vertexBuffer;
  gpu_send_model_vertices(modelVertices, model.info->vertex_count, &vertexBuffer);

  free(modelVertices);
  cleanup_gltf(&model);

  return 0;
}
