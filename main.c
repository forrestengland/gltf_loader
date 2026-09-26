#include <stdio.h>
#include <stdlib.h>
#include "cJSON.h"

typedef struct {
  
  const char *uri;

  int position_byte_offset;
  int vertex_count;

  int index_byte_offset;
  int index_count;
  int index_component_type;

  int normal_byte_offset;
  int normal_count;
  int normal_component_type;
  
} MeshInfo;

// read a text file and return the contents
// remember to free the returned string
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

// parse a gltf file and return the cJSON root object
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

// process the cJSON object. returns the path to the binary file
// sets the binary file byte offset and vertex count for the position vertices
//const char* process_json(cJSON* root, int* byte_offset, int* vertex_count, int* index_byte_offset, int* index_count, int* index_component_type) {
const char* process_json(cJSON* root, MeshInfo* info) {

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

  cJSON *normal = cJSON_GetObjectItem(attributes, "NORMAL");

  if (!normal || !cJSON_IsNumber(normal)) {
    fprintf(stderr, "NORMAL not found or isn't a number\n");
    return NULL;
  }

  printf("NORMAL accessor: %d\n", normal->valueint);
  int normal_accessor = normal->valueint;

  cJSON *indices = cJSON_GetObjectItem(primitive, "indices");

  if (!indices || !cJSON_IsNumber(indices)) {
    fprintf(stderr, "indices not found or isn't a number\n");
    return NULL;
  }

  int index_accessor = indices->valueint;

  printf("INDEX accessor: %d\n", index_accessor);

  cJSON *accessors = cJSON_GetObjectItem(root, "accessors");

  if (!accessors || !cJSON_IsArray(accessors)) {
    fprintf(stderr, "accessors not found or isn't an array\n");
    return NULL;
  }

  printf("Number of accessors: %d\n", cJSON_GetArraySize(accessors));

  cJSON *accessor = cJSON_GetArrayItem(accessors, position_accessor);

  if (!accessor) {
    fprintf(stderr, "No position accessor\n");
    return NULL;
  }

  // get relevant data from accessor
  int bufferViewIndex = cJSON_GetObjectItem(accessor, "bufferView")->valueint; // index to buffer view to find binary file offset
  int componentType = cJSON_GetObjectItem(accessor, "componentType")->valueint; // 5126 - float
  int count = cJSON_GetObjectItem(accessor, "count")->valueint; // number of vertices
  const char *type = cJSON_GetObjectItem(accessor, "type")->valuestring; // "VEC3"

  printf("accessor %d has a count of %d, %d bytes\n", position_accessor, count, count * 12);

  accessor = cJSON_GetArrayItem(accessors, normal_accessor);

  if (!accessor) {
    fprintf(stderr, "No normal accessor\n");
    return NULL;
  }

  // get relevant data from accessor
  int normalBufferViewIndex = cJSON_GetObjectItem(accessor, "bufferView")->valueint; // index to buffer view to find binary file offset
  int normalComponentType = cJSON_GetObjectItem(accessor, "componentType")->valueint; // 5126 - float
  int normalCount = cJSON_GetObjectItem(accessor, "count")->valueint; // number of vertices
  const char *normalType = cJSON_GetObjectItem(accessor, "type")->valuestring; // "VEC3"

  printf("normal accessor %d has normalBufferViewIndex %d, normalComponentType %d, normalCount %d\n", normal_accessor, normalBufferViewIndex, normalComponentType, normalCount);
  info->normal_count = normalCount;
  info->normal_component_type = normalComponentType;

  accessor = cJSON_GetArrayItem(accessors, index_accessor);

  if (!accessor) {
    fprintf(stderr, "No index accessor\n");
    return NULL;
  }

  int indexBufferViewIndex = cJSON_GetObjectItem(accessor, "bufferView")->valueint;
  int indexComponentType = cJSON_GetObjectItem(accessor, "componentType")->valueint;
  printf("index component type: %d\n", indexComponentType);
  int indexCount = cJSON_GetObjectItem(accessor, "count")->valueint;

  info->index_component_type = indexComponentType;
  info->index_count = indexCount;

  cJSON *bufferViews = cJSON_GetObjectItem(root, "bufferViews");

  if (!bufferViews || !cJSON_IsArray(bufferViews)) {
    fprintf(stderr, "bufferViews not found or isn't an array\n");
    return NULL;
  }

  printf("Number of bufferViews: %d\n", cJSON_GetArraySize(bufferViews));

  cJSON *bufferView = cJSON_GetArrayItem(bufferViews, bufferViewIndex);

  if (!bufferView) {
    fprintf(stderr, "No position bufferView\n");
    return NULL;
  }

  int bufferIndex = cJSON_GetObjectItem(bufferView, "buffer")->valueint;
  int byteOffset = 0;
  cJSON *offset = cJSON_GetObjectItem(bufferView, "byteOffset");
  if (offset) byteOffset = offset->valueint;
  int byteLength = cJSON_GetObjectItem(bufferView, "byteLength")->valueint;

  printf("position bufferView has index %d, byteOffset %d, byteLength %d\n", bufferIndex, byteOffset, byteLength);

  info->position_byte_offset = byteOffset;
  info->vertex_count = count;

  bufferView = cJSON_GetArrayItem(bufferViews, indexBufferViewIndex);
  if (!bufferView) {
    fprintf(stderr, "No index bufferView\n");
    return NULL;
  }

  bufferIndex = cJSON_GetObjectItem(bufferView, "buffer")->valueint;
  byteOffset = 0;
  offset = cJSON_GetObjectItem(bufferView, "byteOffset");
  if (offset) byteOffset = offset->valueint;
  byteLength = cJSON_GetObjectItem(bufferView, "byteLength")->valueint;

  printf("index bufferView has index %d, byteOffset %d, byteLength %d\n", bufferIndex, byteOffset, byteLength);

  info->index_byte_offset = byteOffset;

  bufferView = cJSON_GetArrayItem(bufferViews, normalBufferViewIndex);
  if (!bufferView) {
    fprintf(stderr, "No normal bufferView\n");
    return NULL;
  }

  bufferIndex = cJSON_GetObjectItem(bufferView, "buffer")->valueint;
  byteOffset = 0;
  offset = cJSON_GetObjectItem(bufferView, "byteOffset");
  if (offset) byteOffset = offset->valueint;
  byteLength = cJSON_GetObjectItem(bufferView, "byteLength")->valueint;

  printf("normal bufferView has index %d, byteOffset %d, byteLength %d\n", bufferIndex, byteOffset, byteLength);

  info->normal_byte_offset = byteOffset;

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

// load position vertices from a binary file. returns the loaded position float array
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

#include <stdint.h>

unsigned int *load_indices(const char *filename, int byteOffset, int count, int componentType) {
  
  FILE *file = fopen(filename, "rb");

  if (!file) {
    perror(filename);
    return NULL;
  }

  if (fseek(file, byteOffset, SEEK_SET) != 0) {
    fprintf(stderr, "Failed to seek in %s\n", filename);
    fclose(file);
    return NULL;
  }

  unsigned int *indices =
    malloc(count * sizeof(unsigned int));

  if (!indices) {
    fprintf(stderr, "Failed to allocate indices\n");
    fclose(file);
    return NULL;
  }

  if (componentType == 5123) {
    /* UNSIGNED_SHORT */

    uint16_t *temp =
      malloc(count * sizeof(uint16_t));

    if (!temp) {
      fprintf(stderr, "Failed to allocate temporary indices\n");
      free(indices);
      fclose(file);
      return NULL;
    }

    size_t readCount =
      fread(temp, sizeof(uint16_t), count, file);

    if (readCount != (size_t)count) {
      fprintf(stderr,
	      "Expected %d indices, but only read %zu\n",
	      count,
	      readCount);

      free(temp);
      free(indices);
      fclose(file);
      return NULL;
    }

    for (int i = 0; i < count; i++) {
      indices[i] = temp[i];
    }

    free(temp);
  }
  else if (componentType == 5125) {
    /* UNSIGNED_INT */

    size_t readCount =
      fread(indices, sizeof(unsigned int), count, file);

    if (readCount != (size_t)count) {
      fprintf(stderr,
	      "Expected %d indices, but only read %zu\n",
	      count,
	      readCount);

      free(indices);
      fclose(file);
      return NULL;
    }
  }
  else {
    fprintf(stderr,
	    "Unsupported index component type: %d\n",
	    componentType);

    free(indices);
    fclose(file);
    return NULL;
  }

  fclose(file);

  return indices;
}

int main(int argc, char* argv[]) {
  
  printf("it works\n");

  cJSON* root = parse_gltf("robot.gltf");

  // int byte_offset, count, index_byte_offset, index_count, index_component_type;
  MeshInfo info;
  info.uri = process_json(root, &info);

  // load vertecies from binary file
  float* positions = load_positions(info.uri, info.position_byte_offset, info.vertex_count);

  for (int i=0; i<info.vertex_count; i+=3) {
    printf("position %d x:%f, y:%f z:%f\n", i/3, positions[i], positions[i+1], positions[i+2]);
  }

  unsigned int* indices = load_indices(info.uri, info.index_byte_offset, info.index_count, info.index_component_type);

  for (int i=0; i<info.index_count; i+=3) {
    printf("triangle %d indices: %d, %d %d\n", i/3, indices[i], indices[i+1], indices[i+2]);
  }    

  free(positions);
  free(indices);

  cJSON_Delete(root);
  
  return 0;
}
