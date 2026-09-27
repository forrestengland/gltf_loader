#ifndef H_GPU
#define H_GPU

// opengl
#include <GLES2/gl2.h>

void gpu_send_model_vertices(float* model_vertices, int vertex_count, GLuint* vertex_buffer) {

  glGenBuffers(1, vertex_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, *vertex_buffer);
  glBufferData(GL_ARRAY_BUFFER, vertex_count * 6 * sizeof(float),
	       model_vertices, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, *vertex_buffer);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
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
