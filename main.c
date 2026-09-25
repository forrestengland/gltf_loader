#include <stdio.h>
#include <stdlib.h>
#include "cJSON.h"

char* read_file(const char* filename) {

  FILE *file = fopen(filename, "rb");

  if (!file) {
    perror(filename);
    return NULL;
  }

  /* Find file size */
  fseek(file, 0, SEEK_END);
  long fileSize = ftell(file);
  rewind(file);

  /* Read entire file */
  char *jsonText = malloc(fileSize + 1);

  if (!jsonText) {
    printf("error allocating buffer for read_file\n");
    fclose(file);
    return NULL;
  }

  size_t bytes_read = fread(jsonText, 1, fileSize, file);
  jsonText[fileSize] = '\0';

  fclose(file);

  printf("bytes read: %d\n", bytes_read);

  return jsonText;
}

cJSON* parse_gltf(const char* filename) {

  char* jsonText = read_file(filename);

  /* Parse JSON */
  cJSON *root = cJSON_Parse(jsonText);

  if (!root) {
    fprintf(stderr, "Failed to parse JSON\n");
    free(jsonText);
    return NULL;
  }

  printf("JSON loaded successfully!\n");

  /* We're done with the original text */
  free(jsonText);

  return root;
}

const char* process_json(cJSON* root, int* byte_offset, int* vertex_count) {

  cJSON *meshes = cJSON_GetObjectItem(root, "meshes");

  if (!meshes || !cJSON_IsArray(meshes)) {
    fprintf(stderr, "meshes not found or isn't an array\n");
    return NULL;
  }

  printf("Number of meshes: %d\n", cJSON_GetArraySize(meshes));

  cJSON *mesh = cJSON_GetArrayItem(meshes, 0);

  if (!mesh) {
    fprintf(stderr, "No first mesh\n");
    return NULL;
  }

  // get the first mesh name
  cJSON *meshName = cJSON_GetObjectItem(mesh, "name");
  char *meshNameStr = meshName->valuestring;
  printf("found mesh 0 name '%s'\n", meshNameStr);

  cJSON *primitives = cJSON_GetObjectItem(mesh, "primitives");

  if (!primitives || !cJSON_IsArray(primitives)) {
    fprintf(stderr, "primitives not found\n");
    return NULL;
  }

  cJSON *primitive = cJSON_GetArrayItem(primitives, 0);

  if (!primitive) {
    fprintf(stderr, "No first primitive\n");
    return NULL;
  }

  cJSON *attributes = cJSON_GetObjectItem(primitive, "attributes");

  if (!attributes) {
    fprintf(stderr, "attributes not found\n");
    return NULL;
  }

  cJSON *position = cJSON_GetObjectItem(attributes, "POSITION");

  if (!position || !cJSON_IsNumber(position)) {
    fprintf(stderr, "POSITION not found or isn't a number\n");
    return NULL;
  }

  printf("POSITION accessor: %d\n", position->valueint);
  int position_accessor = position->valueint;

  cJSON *accessors = cJSON_GetObjectItem(root, "accessors");

  if (!accessors || !cJSON_IsArray(accessors)) {
    fprintf(stderr, "accessors not found or isn't an array\n");
    return NULL;
  }

  printf("Number of accessors: %d\n", cJSON_GetArraySize(accessors));

  cJSON *accessor = cJSON_GetArrayItem(accessors, 0);

  if (!accessor) {
    fprintf(stderr, "No first accessor\n");
    return NULL;
  }

  // get relevant data from accessor
  int bufferViewIndex = cJSON_GetObjectItem(accessor, "bufferView")->valueint; // index to buffer view to find binary file offset
  int componentType = cJSON_GetObjectItem(accessor, "componentType")->valueint; // 5126 - float
  int count = cJSON_GetObjectItem(accessor, "count")->valueint; // number of vertices
  const char *type = cJSON_GetObjectItem(accessor, "type")->valuestring; // "VEC3"

  printf("accessor 0 has a count of %d, %d bytes\n", count, count * 12);

  cJSON *bufferViews = cJSON_GetObjectItem(root, "bufferViews");

  if (!bufferViews || !cJSON_IsArray(bufferViews)) {
    fprintf(stderr, "bufferViews not found or isn't an array\n");
    return NULL;
  }

  printf("Number of bufferViews: %d\n", cJSON_GetArraySize(bufferViews));

  cJSON *bufferView = cJSON_GetArrayItem(bufferViews, 0);

  if (!bufferView) {
    fprintf(stderr, "No first bufferView\n");
    return NULL;
  }

  int bufferIndex = cJSON_GetObjectItem(bufferView, "buffer")->valueint;
  int byteOffset = 0;
  cJSON *offset = cJSON_GetObjectItem(bufferView, "byteOffset");
  if (offset) byteOffset = offset->valueint;
  int byteLength = cJSON_GetObjectItem(bufferView, "byteLength")->valueint;

  printf("bufferView 0 has index %d, byteOffset %d, byteLength %d\n", bufferIndex, byteOffset, byteLength);

  *byte_offset = byteOffset;
  *vertex_count = count;

  cJSON *buffers = cJSON_GetObjectItem(root, "buffers");

  if (!buffers || !cJSON_IsArray(buffers)) {
    fprintf(stderr, "buffers not found or isn't an array\n");
    return NULL;
  }

  printf("Number of buffers: %d\n", cJSON_GetArraySize(buffers));

  cJSON *buffer = cJSON_GetArrayItem(buffers, bufferIndex); // use buffer index from above accessor

  if (!buffer) {
    fprintf(stderr, "No buffer at index %d\n", bufferIndex);
    return NULL;
  }

  const char *uri = cJSON_GetObjectItem(buffer, "uri")->valuestring;
  printf("Binary file: %s\n", uri);

  return uri;
}

float *load_positions(const char *filename, int byteOffset, int count) {

  FILE *file = fopen(filename, "rb");

  if (!file) {
    perror(filename);
    return NULL;
  }

  /* Move to the beginning of the position data */
  if (fseek(file, byteOffset, SEEK_SET) != 0) {
    fprintf(stderr, "Failed to seek in %s\n", filename);
    fclose(file);
    return NULL;
  }

  /* Three floats per vertex: X, Y, Z */
  float *positions = malloc(count * 3 * sizeof(float));

  if (!positions) {
    fprintf(stderr, "Failed to allocate positions\n");
    fclose(file);
    return NULL;
  }

  size_t numFloats = count * 3;

  size_t readCount =
    fread(positions, sizeof(float), numFloats, file);

  fclose(file);

  if (readCount != numFloats) {
    fprintf(stderr,
	    "Expected %zu floats, but only read %zu\n",
	    numFloats,
	    readCount);

    free(positions);
    return NULL;
  }

  return positions;
}

int main(int argc, char* argv[]) {
  
  printf("it works\n");

  cJSON* root = parse_gltf("robot.gltf");

  int byte_offset, count;
  const char* uri = process_json(root, &byte_offset, &count);

  // load vertecies from binary file
  float* positions = load_positions(uri, byte_offset, count);

  for (int i=0; i<count; i+=3) {
    printf("position %d x:%f, y:%f z:%f\n", i/3, positions[i], positions[i+1], positions[i+2]);
  }

  free(positions);

  cJSON_Delete(root);
  
  return 0;
}
