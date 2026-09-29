// standard includes
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>

// sdl
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

// custom headers
#include "gltf_loader.h"
#include "gpu.h"

// requested screen size for desktop
#define SCREEN_W 640
#define SCREEN_H 480

#define GLTF_FILE "robot.gltf"
// #define GLTF_FILE "cube.gltf"

#define PI 3.1415926535

#define VERTEX_SHADER_FILE "vertex_shader.glsl"
#define FRAGMENT_SHADER_FILE "fragment_shader.glsl"

GLint matrixUniform;
GLint projectionUniform;
GLint viewUniform;

float aspect;

SDL_Window *window;
SDL_GLContext context;
GLuint program;

int mouseDragging = 0;
int lastMouseX = 0;
int lastMouseY = 0;

// Time
double getTime() {
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);

    return ts.tv_sec +
           ts.tv_nsec / 1000000000.0;
}

void createProgram(void) {

  char *pgmTxt = read_file(VERTEX_SHADER_FILE);

  if (!pgmTxt) {
    printf("error reading glsl file '%s'\n", VERTEX_SHADER_FILE);
    return;
  }

  GLuint vertexShader = gpu_compile_shader(GL_VERTEX_SHADER, pgmTxt);
  free(pgmTxt);

  pgmTxt = read_file(FRAGMENT_SHADER_FILE);
  if (!pgmTxt) {
    printf("error reading glsl file '%s'\n", FRAGMENT_SHADER_FILE);
    return;
  }

  GLuint fragmentShader = gpu_compile_shader(GL_FRAGMENT_SHADER, pgmTxt);
  free(pgmTxt);

  if (!vertexShader || !fragmentShader)
    return;

  program = glCreateProgram();

  glAttachShader(program, vertexShader);
  glAttachShader(program, fragmentShader);

  glBindAttribLocation(program, 0, "position");
  glBindAttribLocation(program, 1, "normal");    

  glLinkProgram(program);

  GLint success;

  glGetProgramiv(program, GL_LINK_STATUS, &success);

  if (!success) {
      
    char log[512];

    glGetProgramInfoLog(program, sizeof(log), NULL, log);

    fprintf(stderr, "background program linking failed:\n%s\n", log);

    glDeleteProgram(program);
    program = 0;
  }

  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);
}

int setup_screen(void) {

  // init sdl2
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
    fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
    return 0;
  }

  // Ask SDL for an OpenGL ES 2.0 context.
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

  window = SDL_CreateWindow("SDL2 OpenGL ES", 0, 0,	SCREEN_W, SCREEN_H,
					SDL_WINDOW_OPENGL);

  if (!window) {
    fprintf(stderr, "SDL_CreateWindow failed: %s\n",SDL_GetError());
    SDL_Quit();
    return 0;
  }


  context = SDL_GL_CreateContext(window);
  if (!context) {
    fprintf(stderr, "SDL_GL_CreateContext failed: %s\n", SDL_GetError());
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
  }

  printf("GL renderer: %s\n", glGetString(GL_RENDERER));
  printf("GL version: %s\n", glGetString(GL_VERSION));

  // enable depth testing
  glEnable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);  
  glDepthFunc(GL_LESS);
  glClearDepthf(1.0f);
  int depthBits;
  SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE, &depthBits);
  printf("Depth buffer: %d bits\n", depthBits);

  // create the gpu program
  createProgram();

  if (!program) {
    fprintf(stderr, "Failed to create shader program\n");
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
  }

  // handle to communicate with the shader program
  matrixUniform = glGetUniformLocation(program, "modelMatrix");
  projectionUniform = glGetUniformLocation(program, "projectionMatrix");
  viewUniform = glGetUniformLocation(program, "viewMatrix");

  // get actual size of window (fullscreen if on console)
  int width, height;
  SDL_GL_GetDrawableSize(window, &width, &height);
  glViewport(0, 0, width, height);
  aspect = (float)width / (float)height;

  return 1;
}

// Draw OBJ model
void draw_model(Mat4 *modelm, Mat4 *view, Mat4 *projection, Model* m, GLuint program, GLuint vertexBuffer) {

  /*  GLuint cubeBuffer;

  float cube[] = {
    // front
    -1, -1,  1,
    1, -1,  1,
    1,  1,  1,

    -1, -1,  1,
    1,  1,  1,
    -1,  1,  1,

    // back
    -1, -1, -1,
    1,  1, -1,
    1, -1, -1,

    -1, -1, -1,
    -1,  1, -1,
    1,  1, -1,

    // left
    -1, -1, -1,
    -1, -1,  1,
    -1,  1,  1,

    -1, -1, -1,
    -1,  1,  1,
    -1,  1, -1,

    // right
    1, -1,  1,
    1, -1, -1,
    1,  1, -1,

    1, -1,  1,
    1,  1, -1,
    1,  1,  1,

    // top
    -1,  1,  1,
    1,  1,  1,
    1,  1, -1,

    -1,  1,  1,
    1,  1, -1,
    -1,  1, -1,

    // bottom
    -1, -1, -1,
    1, -1, -1,
    1, -1,  1,

    -1, -1, -1,
    1, -1,  1,
    -1, -1,  1
  };

  glGenBuffers(1, &cubeBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, cubeBuffer);

  glBufferData(GL_ARRAY_BUFFER, sizeof(cube), cube, GL_STATIC_DRAW);

  glUseProgram(program);

  glUniformMatrix4fv(
		     matrixUniform,
		     1,
		     GL_FALSE,
		     modelm->m
		     );

  glUniformMatrix4fv(
		     projectionUniform,
		     1,
		     GL_FALSE,
		     projection->m
		     );

  glUniformMatrix4fv(
		     viewUniform,
		     1,
		     GL_FALSE,
		     view->m
		     );

  glDisable(GL_CULL_FACE);
  glDisable(GL_DEPTH_TEST);

  glEnableVertexAttribArray(0);

  glVertexAttribPointer(
			0,
			3,
			GL_FLOAT,
			GL_FALSE,
			3 * sizeof(float),
			cube
			);

  glDrawArrays(GL_TRIANGLES, 0, 36);

  glDisableVertexAttribArray(0); */

  glUseProgram(program);
  glUniformMatrix4fv(matrixUniform, 1, GL_FALSE, modelm->m);
  glUniformMatrix4fv(projectionUniform, 1, GL_FALSE, projection->m);
  glUniformMatrix4fv(viewUniform, 1, GL_FALSE, view->m);    
  glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
  // Position
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
  // Normal
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
  glDrawArrays(GL_TRIANGLES, 0, m->info->index_count);

  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1); 
}

int main(int argc, char* argv[]) {
  
  Model model;

  if (!setup_screen()) {
    printf("setup_screen failed\n");
    return 1;
  }

  int running = 1;
  
  load_gltf(GLTF_FILE, &model);
  float* modelVertices = model_vertices_gltf(&model);

  /*  printf("joint count = %d\n", model.skin->joint_count);
  printf("node count = %d\n", model.node_count);  

  for (int j=0; j<model.skin->joint_count; j++) {

    printf("inspecting joint %d\n", j);

    int i = model.skin->joints[j];

    printf("joint node index is %d\n", i);

    Mat4 m = get_node_world_matrix(&model, i);
    Mat4 bind = model.skin->inverse_bind_matrices[j];
    Mat4 joint_matrix = mat4_multiply(m, bind);
    
    printf("got joint matrix for joint %d, node %d '%s':\n", j, i, model.nodes[i].name);
    printf("[%f][%f][%f][%f]\n", joint_matrix.m[0], joint_matrix.m[4], joint_matrix.m[8], joint_matrix.m[12]);
    printf("[%f][%f][%f][%f]\n", joint_matrix.m[1], joint_matrix.m[5], joint_matrix.m[9], joint_matrix.m[13]);
    printf("[%f][%f][%f][%f]\n", joint_matrix.m[2], joint_matrix.m[6], joint_matrix.m[10], joint_matrix.m[14]);
    printf("[%f][%f][%f][%f]\n", joint_matrix.m[3], joint_matrix.m[7], joint_matrix.m[11], joint_matrix.m[15]);
    } */

  GLuint vertexBuffer;
  gpu_send_model_vertices(modelVertices, model.info->index_count, &vertexBuffer);

  GLint bufferSize;

  glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
  glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &bufferSize);

  printf("VBO size = %d bytes\n", bufferSize);
  printf("Expected = %d bytes\n",
	 model.info->index_count * 6 * sizeof(float));

  free(modelVertices);

  // projection matrix
  Mat4 projection = mat4_perspective(60.0f * PI / 180.0f, aspect, 0.1f, 100.0f);

  // Timing
  double previousTime = getTime();
  double angleX = 0.0;
  double angleY = 0.0;  
  int frameCount = 0;
  double fpsTimer = 0.0;
  int fps = 0;

  // Main loop
  while (running) {

    double currentTime = getTime();
    double deltaTime = currentTime - previousTime;
    previousTime = currentTime;

    // FPS
    frameCount++;
    fpsTimer += deltaTime;
    if (fpsTimer >= 1.0) {
      //      printf("FPS: %d\n", frameCount);
      fps = frameCount;
      frameCount = 0;
      fpsTimer = 0.0;
    }

    // Events
    SDL_Event event;
    while (SDL_PollEvent(&event)) {

      if (event.type == SDL_QUIT)
	running = 0;

      if (event.type == SDL_KEYDOWN) {
        printf("Key pressed: %s\n",
               SDL_GetKeyName(event.key.keysym.sym));
	if (event.key.keysym.sym == SDLK_ESCAPE) {
	  running = 0;
	}
      }

      if (event.type == SDL_KEYUP) {
        printf("Key released: %s\n",
               SDL_GetKeyName(event.key.keysym.sym));
      }

      if (event.type == SDL_MOUSEBUTTONDOWN) {
	if (event.button.button == SDL_BUTTON_LEFT) {
	  mouseDragging = 1;
	  lastMouseX = event.button.x;
	  lastMouseY = event.button.y;
	}
      }

      if (event.type == SDL_MOUSEBUTTONUP) {
	if (event.button.button == SDL_BUTTON_LEFT) {
	  mouseDragging = 0;
	}
      }

      if (event.type == SDL_MOUSEMOTION && mouseDragging) {
	int dx = event.motion.x - lastMouseX;
	int dy = event.motion.y - lastMouseY;
	angleY += dx * 0.05;
	angleX += dy * 0.05;
      }
    }
    
    // update camera based on player
    Vec3 cameraPosition = {0.0, 0.0, 8.0f};
    Vec3 cameraTarget = {0.0, 0.0, 0.0};
    Vec3 cameraUp = {0.0f, 1.0f, 0.0f};
    Mat4 view = mat4_look_at(cameraPosition, cameraTarget, cameraUp);

    // calculate rotation matrices for player
    Mat4 rotationX = mat4_rotation_x((float)(angleX * PI / 180.0));
    Mat4 rotationY = mat4_rotation_y((float)(angleY * PI / 180.0));
    Mat4 rotation = mat4_multiply(rotationY, rotationX);

    // translation for player
    Mat4 translation = mat4_translation(0.0, 0.0, 0.0);    
    Mat4 modelm = mat4_multiply(translation, rotation);

    // Clear the frame
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // draw player model
    draw_model(&modelm, &view, &projection, &model, program, vertexBuffer);
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
      printf("GL error: 0x%x\n", err);
    }
    //    printf("vertexBuffer = %u\n", vertexBuffer);
    //    printf("vertex_count = %d\n", model.info->vertex_count);    

    // Display frame
    SDL_GL_SwapWindow(window);
  }

  // Cleanup
  cleanup_gltf(&model);  

  glDeleteBuffers(1, &vertexBuffer);
  glDeleteProgram(program);
 
  SDL_GL_DeleteContext(context);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
