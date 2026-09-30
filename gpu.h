#ifndef H_GPU
#define H_GPU

// opengl
#include <GLES2/gl2.h>

// send the model vertices to gpu
void gpu_send_model_vertices(float* model_vertices, int vertex_count, GLuint* vertex_buffer) {

  glGenBuffers(1, vertex_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, *vertex_buffer);
  glBufferData(GL_ARRAY_BUFFER, vertex_count * 14 * sizeof(float),
	       model_vertices, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, *vertex_buffer);

  // position
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void *)0);
  // normal
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void *)(3 * sizeof(float)));
  // joints
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void *)(6 * sizeof(float)));  
  // weights
  glEnableVertexAttribArray(3);
  glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void *)(10 * sizeof(float)));  
  
}

// compile a shader program
GLuint gpu_compile_shader(GLenum type, const char *source) {

  GLuint shader = glCreateShader(type);

  glShaderSource(shader, 1, &source, NULL);
  glCompileShader(shader);

  GLint success;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

  if (!success) {
    char log[512];

    glGetShaderInfoLog(shader, sizeof(log), NULL, log);

    fprintf(stderr, "Shader compilation failed:\n%s\n", log);

    glDeleteShader(shader);
    return 0;
  }

  return shader;
}


#endif
