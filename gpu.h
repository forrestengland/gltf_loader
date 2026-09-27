#ifndef H_GPU
#define H_GPU

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

#endif
