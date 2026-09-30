#ifndef H_GLTF_LOADER
#define H_GLTF_LOADER

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "f3_mat.h"
#include "cJSON.h"

typedef struct {

  int input_accessor;
  int input_buffer_view_index;
  int input_component_type;
  int input_buffer_count;
  char* input_buffer_type;
  int input_byte_offset;
  
  int output_accessor;
  int output_buffer_view_index;
  int output_component_type;
  int output_buffer_count;
  char* output_buffer_type;
  int output_byte_offset;
  
  float *times;
  int keyframe_count;

  float *values;
  int value_components;

  char *interpolation;
  
} AnimationSampler;

typedef struct {
  int sampler;
  int node;
  char *path;
} AnimationChannel;

typedef struct {
  char *name;

  AnimationSampler *samplers;
  int sampler_count;

  AnimationChannel *channels;
  int channel_count;
} Animation;

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

  int skin_byte_offset;
  int skin_count;
  int skin_component_type;

  int joints_0_component_type;
  int joints_0_count;
  int joints_0_byte_offset;

  int weights_0_component_type;
  int weights_0_count;
  int weights_0_byte_offset;

  //  int anim_sample_input_byte_offset;
  //  char* anim_sample_input_type;
  //  int anim_sample_input_count;

  //  int anim_sample_output_byte_offset;
  //  char* anim_sample_output_type;
  //  int anim_sample_output_count;

} MeshInfo;

typedef struct {

  float position[3];
  float normal[3];

  unsigned short joints[4];
  float weights[4];
  
} Vertex;

typedef struct {
  unsigned int vertex[3];
} Face;

typedef struct {
  int joint_count;
  int* joints;
  int inverse_bind_accessor;
  Mat4* inverse_bind_matrices;
} Skin;

typedef struct {

  char* name;

  int parent;
  int* children;
  int child_count;

  Vec3 translation;
  Vec3 scale;
  Quaternion rotation;

  int mesh;
  int skin;
  
} Node;

typedef struct {

  MeshInfo* info;
  Vertex* vertices;
  Face* faces;
  Skin* skin;

  Node* nodes;
  int node_count;

  float* anim_outputs;
  Animation* anim;
  
} Model;

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

  //  printf("bytes read: %d\n", bytes_read);

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

  //  printf("JSON loaded successfully!\n");

  /* We're done with the original text */
  free(jsonText);

  return root;
}

// process the cJSON object. returns the path to the binary file. sets the values in the MeshInfo
const char* process_json(cJSON* root, Model* model) {

  MeshInfo* info = model->info;
  Skin* skin = model->skin;

  cJSON *meshes = cJSON_GetObjectItem(root, "meshes");

  if (!meshes || !cJSON_IsArray(meshes)) {
    fprintf(stderr, "meshes not found or isn't an array\n");
    return NULL;
  }

  //  printf("Number of meshes: %d\n", cJSON_GetArraySize(meshes));

  cJSON *mesh = cJSON_GetArrayItem(meshes, 0);

  if (!mesh) {
    fprintf(stderr, "No first mesh\n");
    return NULL;
  }

  // get the first mesh name
  cJSON *meshName = cJSON_GetObjectItem(mesh, "name");
  char *meshNameStr = meshName->valuestring;
  //  printf("found mesh 0 name '%s'\n", meshNameStr);

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

  //  printf("POSITION accessor: %d\n", position->valueint);
  int position_accessor = position->valueint;

  cJSON *normal = cJSON_GetObjectItem(attributes, "NORMAL");

  if (!normal || !cJSON_IsNumber(normal)) {
    fprintf(stderr, "NORMAL not found or isn't a number\n");
    return NULL;
  }

  //  printf("NORMAL accessor: %d\n", normal->valueint);
  int normal_accessor = normal->valueint;

  // get JOINTS_0 accessor index
  cJSON *joints0 = cJSON_GetObjectItem(attributes, "JOINTS_0");

  if (!joints0 || !cJSON_IsNumber(joints0)) {
    fprintf(stderr, "JOINTS_0 not found or isn't a number\n");
    return NULL;
  }

  //  printf("JOINTS_0 accessor: %d\n", joints0->valueint);
  int joints_0_accessor = joints0->valueint;
  // -----

  // get WEIGHTS_0 accessor index
  cJSON *weights0 = cJSON_GetObjectItem(attributes, "WEIGHTS_0");

  if (!weights0 || !cJSON_IsNumber(weights0)) {
    fprintf(stderr, "WEIGHTS_0 not found or isn't a number\n");
    return NULL;
  }

  //  printf("WEIGHTS_0 accessor: %d\n", weights0->valueint);
  int weights_0_accessor = weights0->valueint;
  // -----

  cJSON *indices = cJSON_GetObjectItem(primitive, "indices");

  if (!indices || !cJSON_IsNumber(indices)) {
    fprintf(stderr, "indices not found or isn't a number\n");
    return NULL;
  }

  int index_accessor = indices->valueint;

  //  printf("INDEX accessor: %d\n", index_accessor);

  // load skin
  cJSON *skins = cJSON_GetObjectItem(root, "skins");
  if (!skins || !cJSON_IsArray(skins)) {
    fprintf(stderr, "skins not found or isn't an array\n");
    return NULL;
  }
  // get first skin
  //  printf("Number of skins: %d\n", cJSON_GetArraySize(skins));
  cJSON* skinJSON = cJSON_GetArrayItem(skins, 0);
  int inverseBindMatrices = cJSON_GetObjectItem(skinJSON, "inverseBindMatrices")->valueint;
  //  printf("got skin inverseBindMatrices: %d\n", inverseBindMatrices);
  skin->inverse_bind_accessor = inverseBindMatrices;
  // load joint node indices
  cJSON* joints = cJSON_GetObjectItem(skinJSON, "joints");
  int jointCount = cJSON_GetArraySize(joints);
  //  printf("got %d joints\n", jointCount);
  skin->joint_count = jointCount;
  skin->joints = malloc(sizeof(int) * skin->joint_count);
  for (int i=0; i<skin->joint_count; i++) {
    skin->joints[i] = cJSON_GetArrayItem(joints, i)->valueint;
    //    printf("got joint %d: %d\n", i, skin->joints[i]);
  }

  cJSON* nodes = cJSON_GetObjectItem(root, "nodes");
  int nodeCount = cJSON_GetArraySize(nodes);
  model->node_count = nodeCount;
  //  printf("got %d nodes\n", model->node_count);
  model->nodes = malloc(sizeof(Node) * model->node_count);
  for (int i=0; i<model->node_count; i++) {

    // load each node data. start with defaults
    model->nodes[i].translation.x = 0.0f;
    model->nodes[i].translation.y = 0.0f;
    model->nodes[i].translation.z = 0.0f;
    model->nodes[i].rotation.x = 0.0f;
    model->nodes[i].rotation.y = 0.0f;
    model->nodes[i].rotation.z = 0.0f;
    model->nodes[i].rotation.w = 1.0f;
    model->nodes[i].scale.x = 1.0f;
    model->nodes[i].scale.y = 1.0f;    
    model->nodes[i].scale.z = 1.0f;
    model->nodes[i].mesh = -1;
    model->nodes[i].skin = -1;
    model->nodes[i].parent = -1;
    model->nodes[i].children = NULL;
    model->nodes[i].child_count = 0;
    model->nodes[i].name = NULL;

    // get node name
    cJSON* node = cJSON_GetArrayItem(nodes, i);
    char* name = cJSON_GetObjectItem(node, "name")->valuestring;
    model->nodes[i].name = malloc(strlen(name) + 1);
    strcpy(model->nodes[i].name, name);
    //    printf("node %d name '%s'\n", i, model->nodes[i].name);
    
    // get node rotation if provided
    cJSON* rotation = cJSON_GetObjectItem(node, "rotation");
    if (rotation) {
      float rotx = cJSON_GetArrayItem(rotation, 0)->valuedouble;
      float roty = cJSON_GetArrayItem(rotation, 1)->valuedouble;
      float rotz = cJSON_GetArrayItem(rotation, 2)->valuedouble;
      float rotw = cJSON_GetArrayItem(rotation, 3)->valuedouble;
      //      printf("found node rotation %f, %f, %f, %f\n", rotx, roty, rotz, rotw);
      model->nodes[i].rotation.x = rotx;
      model->nodes[i].rotation.y = roty;
      model->nodes[i].rotation.z = rotz;
      model->nodes[i].rotation.w = rotw;      
    }

    // get node translation if provided
    cJSON* translation = cJSON_GetObjectItem(node, "translation");
    if (translation) {
      float tx = cJSON_GetArrayItem(translation, 0)->valuedouble;
      float ty = cJSON_GetArrayItem(translation, 1)->valuedouble;
      float tz = cJSON_GetArrayItem(translation, 2)->valuedouble;
      //      printf("found node translation %f, %f, %f\n", tx, ty, tz);
      model->nodes[i].translation.x = tx;
      model->nodes[i].translation.y = ty;
      model->nodes[i].translation.z = tz;      
    }

    // get node scale if provided
    cJSON* scale = cJSON_GetObjectItem(node, "scale");
    if (scale) {
      float sx = cJSON_GetArrayItem(scale, 0)->valuedouble;
      float sy = cJSON_GetArrayItem(scale, 1)->valuedouble;
      float sz = cJSON_GetArrayItem(scale, 2)->valuedouble;
      //      printf("found node scale %f, %f, %f\n", sx, sy, sz);
      model->nodes[i].scale.x = sx;
      model->nodes[i].scale.y = sy;
      model->nodes[i].scale.z = sz;      
    }

    // get node children if provided
    cJSON* children = cJSON_GetObjectItem(node, "children");
    if (children) {
      int childCount = cJSON_GetArraySize(children);
      //      printf("number of children: %d\n", childCount);
      model->nodes[i].child_count = childCount;
      model->nodes[i].children = malloc(sizeof(int) * childCount);
      for (int j=0; j<childCount; j++) {
	model->nodes[i].children[j] = cJSON_GetArrayItem(children, j)->valueint;
	//	printf("got child node index %d\n", model->nodes[i].children[j]);
      }
    }

    // get mesh if provided
    cJSON* nodeMesh = cJSON_GetObjectItem(node, "mesh");
    if (nodeMesh) {
      model->nodes[i].mesh = nodeMesh->valueint;
      //      printf("found node mesh index %d\n", model->nodes[i].mesh);
    }

    // get skin if provided
    cJSON* nodeSkin = cJSON_GetObjectItem(node, "skin");
    if (nodeSkin) {
      model->nodes[i].skin = nodeSkin->valueint;
      //      printf("found node skin index %d\n", model->nodes[i].skin);
    }
    
  }

  // nodes are set up, now we can set up the parent indexes
  for (int i=0; i<model->node_count; i++) {
    Node* node = &model->nodes[i];
    for (int c=0; c<node->child_count; c++) {
      int ci = node->children[c];
      Node* child = &model->nodes[ci];
      child->parent = i;
      //      printf("setting node %d parent to %d\n", ci, i);
    }
  }

  // ------- load animations --------------
  cJSON* animations = cJSON_GetObjectItem(root, "animations");
  int animationCount = cJSON_GetArraySize(animations);
  printf("got %d animations\n", animationCount);
  
  // get first animation
  cJSON* animation = cJSON_GetArrayItem(animations, 0);
  
  char* animName = cJSON_GetObjectItem(animation, "name")->valuestring;
  printf("got animation '%s'\n", animName);
  model->anim->name = malloc(strlen(animName) + 1);
  strcpy(model->anim->name, animName);
    
  cJSON* channels = cJSON_GetObjectItem(animation, "channels");


  model->anim->channel_count = cJSON_GetArraySize(channels);
  model->anim->channels = malloc(sizeof(AnimationChannel) * model->anim->channel_count);

  for (int c=0; c<model->anim->channel_count; c++) {
  
    // get a channel
    cJSON* channel = cJSON_GetArrayItem(channels, c);

    int samplerNum = cJSON_GetObjectItem(channel, "sampler")->valueint;
    printf("channel 0 sampler: %d\n", samplerNum);
    model->anim->channels[c].sampler = samplerNum;
    //    model->anim->channels[0].sampler = 0;
  
    cJSON* target = cJSON_GetObjectItem(channel, "target");

    int targetVal = cJSON_GetObjectItem(target, "node")->valueint;
    model->anim->channels[c].node = targetVal;

    char* pathVal = cJSON_GetObjectItem(target, "path")->valuestring;
    model->anim->channels[c].path = malloc(strlen(pathVal) + 1);
    strcpy(model->anim->channels[c].path, pathVal);
    printf("channel %d target node: %d, path: %s\n", c, targetVal, pathVal);
  }

  // get the sampler for this channel
  cJSON* samplers = cJSON_GetObjectItem(animation, "samplers");

  model->anim->sampler_count = cJSON_GetArraySize(samplers);
  model->anim->samplers = malloc(sizeof(AnimationSampler) * model->anim->sampler_count);

  for (int s=0; s<model->anim->sampler_count; s++) {

    cJSON* sampler = cJSON_GetArrayItem(samplers, s);

    char* interpVal = cJSON_GetObjectItem(sampler, "interpolation")->valuestring;
    model->anim->samplers[s].interpolation = malloc(strlen(interpVal) + 1);
    strcpy(model->anim->samplers[s].interpolation, interpVal);
  
    model->anim->samplers[s].input_accessor = cJSON_GetObjectItem(sampler, "input")->valueint;
    model->anim->samplers[s].output_accessor = cJSON_GetObjectItem(sampler, "output")->valueint;  

    printf("sampler %d input: %d, interpolation: %s, output: %d\n", s,
	   model->anim->samplers[s].input_accessor,
	   interpVal,
	   model->anim->samplers[s].output_accessor);
  }

  // ------------------------------------

  cJSON *accessors = cJSON_GetObjectItem(root, "accessors");

  if (!accessors || !cJSON_IsArray(accessors)) {
    fprintf(stderr, "accessors not found or isn't an array\n");
    return NULL;
  }

  //  printf("Number of accessors: %d\n", cJSON_GetArraySize(accessors));

  cJSON *accessor = cJSON_GetArrayItem(accessors, position_accessor);

  if (!accessor) {
    fprintf(stderr, "No position accessor\n");
    return NULL;
  }

  // get relevant data from accessor
  // index to buffer view to find binary file offset
  int bufferViewIndex = cJSON_GetObjectItem(accessor, "bufferView")->valueint; 
  int componentType = cJSON_GetObjectItem(accessor, "componentType")->valueint; // 5126 - float
  int count = cJSON_GetObjectItem(accessor, "count")->valueint; // number of vertices
  const char *type = cJSON_GetObjectItem(accessor, "type")->valuestring; // "VEC3"

  //  printf("accessor %d has a count of %d, %d bytes\n", position_accessor, count, count * 12);

  accessor = cJSON_GetArrayItem(accessors, normal_accessor);

  if (!accessor) {
    fprintf(stderr, "No normal accessor\n");
    return NULL;
  }

  // get relevant data from accessor
  // index to buffer view to find binary file offset
  int normalBufferViewIndex = cJSON_GetObjectItem(accessor, "bufferView")->valueint; 
  int normalComponentType = cJSON_GetObjectItem(accessor, "componentType")->valueint; // 5126 - float
  int normalCount = cJSON_GetObjectItem(accessor, "count")->valueint; // number of vertices
  const char *normalType = cJSON_GetObjectItem(accessor, "type")->valuestring; // "VEC3"

  //  printf("normal accessor %d has normalBufferViewIndex %d, normalComponentType %d, normalCount %d\n",
  //	 normal_accessor, normalBufferViewIndex, normalComponentType, normalCount);
  info->normal_count = normalCount;
  info->normal_component_type = normalComponentType;

  accessor = cJSON_GetArrayItem(accessors, index_accessor);

  if (!accessor) {
    fprintf(stderr, "No index accessor\n");
    return NULL;
  }

  int indexBufferViewIndex = cJSON_GetObjectItem(accessor, "bufferView")->valueint;
  int indexComponentType = cJSON_GetObjectItem(accessor, "componentType")->valueint;
  //  printf("index component type: %d\n", indexComponentType);
  int indexCount = cJSON_GetObjectItem(accessor, "count")->valueint;

  info->index_component_type = indexComponentType;
  info->index_count = indexCount;

  // skin accessor
  accessor = cJSON_GetArrayItem(accessors, model->skin->inverse_bind_accessor);
  if (!accessor) {
    fprintf(stderr, "no skin accessor\n");
    return NULL;
  }
  int skinBufferViewIndex = cJSON_GetObjectItem(accessor, "bufferView")->valueint;
  int skinComponentType = cJSON_GetObjectItem(accessor, "componentType")->valueint;
  int skinBufferCount = cJSON_GetObjectItem(accessor, "count")->valueint;
  //  printf("got skin accessor %d: bufferView: %d, componentType: %d, count: %d\n",
  //	 model->skin->inverse_bind_accessor, skinBufferViewIndex, skinComponentType, skinBufferCount);

  info->skin_component_type = skinComponentType;
  info->skin_count = skinBufferCount;

  // JOINTS_0 accessor
  accessor = cJSON_GetArrayItem(accessors, joints_0_accessor);
  if (!accessor) {
    fprintf(stderr, "no JOINTS_0 accessor\n");
    return NULL;
  }
  int joints0BufferViewIndex = cJSON_GetObjectItem(accessor, "bufferView")->valueint;
  int joints0ComponentType = cJSON_GetObjectItem(accessor, "componentType")->valueint;
  int joints0BufferCount = cJSON_GetObjectItem(accessor, "count")->valueint;
  //  printf("got JOINTS_0 accessor %d: bufferView: %d, componentType: %d, count: %d\n",
  //	 joints_0_accessor, joints0BufferViewIndex, joints0ComponentType, joints0BufferCount);

  info->joints_0_component_type = joints0ComponentType;
  info->joints_0_count = joints0BufferCount;
  //----

  // WEIGHTS_0 accessor
  accessor = cJSON_GetArrayItem(accessors, weights_0_accessor);
  if (!accessor) {
    fprintf(stderr, "no WEIGHTS_0 accessor\n");
    return NULL;
  }
  int weights0BufferViewIndex = cJSON_GetObjectItem(accessor, "bufferView")->valueint;
  int weights0ComponentType = cJSON_GetObjectItem(accessor, "componentType")->valueint;
  int weights0BufferCount = cJSON_GetObjectItem(accessor, "count")->valueint;
  //  printf("got WEIGHTS_0 accessor %d: bufferView: %d, componentType: %d, count: %d\n",
  //	 weights_0_accessor, weights0BufferViewIndex, weights0ComponentType, weights0BufferCount);

  info->weights_0_component_type = weights0ComponentType;
  info->weights_0_count = weights0BufferCount;
  //----

  // ----------------- animation accessors ----------------------------
  for (int i=0; i<model->anim->sampler_count; i++) {
    
    accessor = cJSON_GetArrayItem(accessors, model->anim->samplers[i].input_accessor);
    
    if (!accessor) {
      fprintf(stderr, "no sampler input accessor for %d\n", i);
      return NULL;
    }

    model->anim->samplers[i].input_buffer_view_index = cJSON_GetObjectItem(accessor, "bufferView")->valueint;
    model->anim->samplers[i].input_component_type = cJSON_GetObjectItem(accessor, "componentType")->valueint;
    model->anim->samplers[i].input_buffer_count = cJSON_GetObjectItem(accessor, "count")->valueint;
    type = cJSON_GetObjectItem(accessor, "type")->valuestring;
    model->anim->samplers[i].input_buffer_type = malloc(strlen(type) + 1);
    strcpy(model->anim->samplers[i].input_buffer_type, type);
    
    accessor = cJSON_GetArrayItem(accessors, model->anim->samplers[i].output_accessor);
    
    if (!accessor) {
      fprintf(stderr, "no sampler output accessor for %d\n", i);
      return NULL;
    }

    model->anim->samplers[i].output_buffer_view_index = cJSON_GetObjectItem(accessor, "bufferView")->valueint;
    model->anim->samplers[i].output_component_type = cJSON_GetObjectItem(accessor, "componentType")->valueint;
    model->anim->samplers[i].output_buffer_count = cJSON_GetObjectItem(accessor, "count")->valueint;
    type = cJSON_GetObjectItem(accessor, "type")->valuestring;
    model->anim->samplers[i].output_buffer_type = malloc(strlen(type) + 1);
    strcpy(model->anim->samplers[i].output_buffer_type, type);

  }

  // ---------------------------------------------------------

  cJSON *bufferViews = cJSON_GetObjectItem(root, "bufferViews");

  if (!bufferViews || !cJSON_IsArray(bufferViews)) {
    fprintf(stderr, "bufferViews not found or isn't an array\n");
    return NULL;
  }

  //  printf("Number of bufferViews: %d\n", cJSON_GetArraySize(bufferViews));

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

  //  printf("position bufferView has index %d, byteOffset %d, byteLength %d\n",
  //	 bufferIndex, byteOffset, byteLength);

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

  //  printf("index bufferView has index %d, byteOffset %d, byteLength %d\n",
  //	 bufferIndex, byteOffset, byteLength);

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

  //  printf("normal bufferView has index %d, byteOffset %d, byteLength %d\n",
  //	 bufferIndex, byteOffset, byteLength);

  info->normal_byte_offset = byteOffset;

  // skin buffer view
  bufferView = cJSON_GetArrayItem(bufferViews, skinBufferViewIndex);
  if (!bufferView) {
    fprintf(stderr, "no skin bufferView\n");
    return NULL;
  }
  bufferIndex = cJSON_GetObjectItem(bufferView, "buffer")->valueint;
  byteOffset = 0;
  offset = cJSON_GetObjectItem(bufferView, "byteOffset");
  if (offset) byteOffset = offset->valueint;
  byteLength = cJSON_GetObjectItem(bufferView, "byteLength")->valueint;
  //  printf("skin bufferView has index %d, byteOffset %d, byteLength %d\n",
  //	 bufferIndex, byteOffset, byteLength);

  info->skin_byte_offset = byteOffset;

  // JOINTS_0 buffer view
  bufferView = cJSON_GetArrayItem(bufferViews, joints0BufferViewIndex);
  if (!bufferView) {
    fprintf(stderr, "no JOINTS_0 bufferView\n");
    return NULL;
  }
  bufferIndex = cJSON_GetObjectItem(bufferView, "buffer")->valueint;
  byteOffset = 0;
  offset = cJSON_GetObjectItem(bufferView, "byteOffset");
  if (offset) byteOffset = offset->valueint;
  byteLength = cJSON_GetObjectItem(bufferView, "byteLength")->valueint;
  //  printf("JOINTS_0 bufferView has index %d, byteOffset %d, byteLength %d\n",
  //  	 bufferIndex, byteOffset, byteLength);
  info->joints_0_byte_offset = byteOffset;
  // --------------------

  // WEIGHTS_0 buffer view
  bufferView = cJSON_GetArrayItem(bufferViews, weights0BufferViewIndex);
  if (!bufferView) {
    fprintf(stderr, "no WEIGHTS_0 bufferView\n");
    return NULL;
  }
  bufferIndex = cJSON_GetObjectItem(bufferView, "buffer")->valueint;
  byteOffset = 0;
  offset = cJSON_GetObjectItem(bufferView, "byteOffset");
  if (offset) byteOffset = offset->valueint;
  byteLength = cJSON_GetObjectItem(bufferView, "byteLength")->valueint;
  //  printf("WEIGHTS_0 bufferView has index %d, byteOffset %d, byteLength %d\n",
  //  	 bufferIndex, byteOffset, byteLength);
  info->weights_0_byte_offset = byteOffset;
  // --------------------

  // ------------------ animation sampler bufferViews ---------------
  for (int i=0; i<model->anim->sampler_count; i++) {
    
    bufferView = cJSON_GetArrayItem(bufferViews, model->anim->samplers[i].input_buffer_view_index);
    if (!bufferView) {
      fprintf(stderr, "no animation sampler input bufferView for %d\n", i);
      return NULL;
    }
    bufferIndex = cJSON_GetObjectItem(bufferView, "buffer")->valueint;
    byteOffset = 0;
    offset = cJSON_GetObjectItem(bufferView, "byteOffset");
    if (offset) byteOffset = offset->valueint;
    byteLength = cJSON_GetObjectItem(bufferView, "byteLength")->valueint;
    printf("animation sampler input bufferView has index %d, byteOffset %d, byteLength %d\n",
	   bufferIndex, byteOffset, byteLength);
    //    info->anim_sample_input_byte_offset = byteOffset;
    model->anim->samplers[i].input_byte_offset = byteOffset;

    // get output byte offset
    bufferView = cJSON_GetArrayItem(bufferViews, model->anim->samplers[i].output_buffer_view_index);
    if (!bufferView) {
      fprintf(stderr, "no animation sampler output bufferView for %d\n", i);
      return NULL;
    }
    bufferIndex = cJSON_GetObjectItem(bufferView, "buffer")->valueint;
    byteOffset = 0;
    offset = cJSON_GetObjectItem(bufferView, "byteOffset");
    if (offset) byteOffset = offset->valueint;
    byteLength = cJSON_GetObjectItem(bufferView, "byteLength")->valueint;
    printf("animation sampler output bufferView has index %d, byteOffset %d, byteLength %d\n",
	   bufferIndex, byteOffset, byteLength);
    //    info->anim_sample_output_byte_offset = byteOffset;
    model->anim->samplers[i].output_byte_offset = byteOffset;
  }
  // ---------------------------------------------------------------------

  cJSON *buffers = cJSON_GetObjectItem(root, "buffers");

  if (!buffers || !cJSON_IsArray(buffers)) {
    fprintf(stderr, "buffers not found or isn't an array\n");
    return NULL;
  }

  //  printf("Number of buffers: %d\n", cJSON_GetArraySize(buffers));

  cJSON *buffer = cJSON_GetArrayItem(buffers, bufferIndex); // use buffer index from above accessor

  if (!buffer) {
    fprintf(stderr, "No buffer at index %d\n", bufferIndex);
    return NULL;
  }

  const char *uri = cJSON_GetObjectItem(buffer, "uri")->valuestring;
  //  printf("Binary file: %s\n", uri);

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

  size_t readCount = fread(positions, sizeof(float), numFloats, file);

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

float *load_normals(const char *filename, int byteOffset, int count, int componentType) {
  
  FILE *file = fopen(filename, "rb");

  if (!file) {
    perror(filename);
    return NULL;
  }

  //  printf("loading normals with byteOffset %d\n", byteOffset);
  if (fseek(file, byteOffset, SEEK_SET) != 0) {
    fprintf(stderr, "Failed to seek in %s\n", filename);
    fclose(file);
    return NULL;
  }

  float *normals = malloc(count * 3 * sizeof(float));

  if (!normals) {
    fprintf(stderr, "Failed to allocate normals\n");
    fclose(file);
    return NULL;
  }

  size_t numFloats = count * 3;

  size_t readCount = fread(normals, sizeof(float), numFloats, file);

  fclose(file);

  if (readCount != numFloats) {
    fprintf(stderr, "Expected %zu floats, but only read %zu\n",
	    numFloats, readCount);

    free(normals);
    return NULL;
  }

  return normals;
}

float* load_skin_inverse_matrices(const char* filename, int byteOffset, int count) {

  FILE *file = fopen(filename, "rb");

  if (!file) {
    perror(filename);
    return NULL;
  }

  //  printf("loading inverse bind matrices with byteOffset %d\n", byteOffset);
  if (fseek(file, byteOffset, SEEK_SET) != 0) {
    fprintf(stderr, "Failed to seek in %s\n", filename);
    fclose(file);
    return NULL;
  }

  float *matrices = malloc(count * 16 * sizeof(float));

  if (!matrices) {
    fprintf(stderr, "Failed to allocate inverse bind matrices\n");
    fclose(file);
    return NULL;
  }

  size_t numFloats = count * 16;
  size_t readCount = fread(matrices, sizeof(float), numFloats, file);

  fclose(file);

  if (readCount != numFloats) {
    fprintf(stderr, "Expected %zu floats reading inverse_bind_matrices, but only read %zu\n",
	    numFloats, readCount);

    free(matrices);
    return NULL;
  }

  return matrices;  
}

unsigned short *load_joints_0(const char *filename, int byteOffset, int count) {
  
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

  unsigned short *joints_0 = malloc(count * sizeof(unsigned short) * 4);

  if (!joints_0) {
    fprintf(stderr, "Failed to allocate joints_0\n");
    fclose(file);
    return NULL;
  }

  /* UNSIGNED_BYTE */

  uint8_t *temp = malloc(count * sizeof(uint8_t) * 4);

  if (!temp) {
    fprintf(stderr, "Failed to allocate temporary indices\n");
    free(joints_0);
    fclose(file);
    return NULL;
  }

  size_t readCount = fread(temp, sizeof(uint8_t), count * 4, file);

  if (readCount != (size_t)(count*4)) {
    fprintf(stderr,
	    "Expected %d joints_0, but only read %zu\n",
	    count,
	    readCount);

    free(temp);
    free(joints_0);
    fclose(file);
    return NULL;
  }

  for (int i = 0; i < count*4; i++) {
    joints_0[i] = temp[i];
  }

  free(temp);

  fclose(file);

  return joints_0;
}

float* load_weights_0(const char* filename, int byteOffset, int count) {

  FILE *file = fopen(filename, "rb");

  if (!file) {
    perror(filename);
    return NULL;
  }

  //  printf("loading inverse bind matrices with byteOffset %d\n", byteOffset);
  if (fseek(file, byteOffset, SEEK_SET) != 0) {
    fprintf(stderr, "Failed to seek in %s\n", filename);
    fclose(file);
    return NULL;
  }

  float *weights = malloc(count * 4 * sizeof(float));

  if (!weights) {
    fprintf(stderr, "Failed to allocate weights_0\n");
    fclose(file);
    return NULL;
  }

  size_t numFloats = count * 4;
  size_t readCount = fread(weights, sizeof(float), numFloats, file);

  fclose(file);

  if (readCount != numFloats) {
    fprintf(stderr, "Expected %zu floats reading weights_0, but only read %zu\n",
	    numFloats, readCount);

    free(weights);
    return NULL;
  }

  return weights;  
}

float *load_anim_sample(const char *filename, int byteOffset, int count, char* type) {

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

  float* inputs;
  size_t readCount;
  size_t numFloats;

  if (strcmp(type, "VEC3") == 0) {

    /* Three floats per vertex: X, Y, Z */
    inputs = malloc(count * 3 * sizeof(float));

    if (!inputs) {
      fprintf(stderr, "Failed to allocate inputs\n");
      fclose(file);
      return NULL;
    }

    numFloats = count * 3;

    readCount = fread(inputs, sizeof(float), numFloats, file);

  } else if (strcmp(type, "VEC4") == 0) {

    inputs = malloc(count * 4 * sizeof(float));

    if (!inputs) {
      fprintf(stderr, "Failed to allocate inputs\n");
      fclose(file);
      return NULL;
    }

    numFloats = count * 4;

    readCount = fread(inputs, sizeof(float), numFloats, file);
    
  } else if (strcmp(type, "SCALAR") == 0) {

    inputs = malloc(count * sizeof(float));
    if (!inputs) {
      fprintf(stderr, "Failed to allocate inputs\n");
      fclose(file);
      return NULL;
    }
    numFloats = count;
    readCount = fread(inputs, sizeof(float), numFloats, file);

  } else if (strcmp(type, "MAT4") == 0) {

    inputs = malloc(count * 16 * sizeof(float));
    if (!inputs) {
      fprintf(stderr, "Failed to allocate inputs\n");
      fclose(file);
      return NULL;
    }
    numFloats = count * 16;
    readCount = fread(inputs, sizeof(float), numFloats, file);

  } else {
    printf("got unknown type %s for animation sample input\n", type);
    fclose(file);
    return NULL;
  }

  fclose(file);

  if (readCount != numFloats) {
    fprintf(stderr,
	    "Expected %zu floats, but only read %zu\n",
	    numFloats,
	    readCount);

    //    free(inputs);
    return NULL;
  }

  return inputs;
}

void cleanup_gltf(Model* model) {
  
  for (int i=0; i<model->node_count; i++) {
    if (model->nodes[i].child_count) free(model->nodes[i].children);
    free(model->nodes[i].name);
  }
  
  free(model->nodes);
  free(model->skin->joints);
  free(model->skin);
  //  free(model->info->anim_sample_input_type);
  //  free(model->info->anim_sample_output_type);
  free(model->info);
  free(model->vertices);
  free(model->faces);
  //  free(model->anim_outputs);
  free(model->anim->name);

  // TODO: free any char* in each sampler / channel

  for (int s=0; s<model->anim->sampler_count; s++) {
    free(model->anim->samplers[s].input_buffer_type);
    free(model->anim->samplers[s].output_buffer_type);
    free(model->anim->samplers[s].times);
    free(model->anim->samplers[s].values);
    free(model->anim->samplers[s].interpolation);
  }
  free(model->anim->samplers);

  for (int c=0; c<model->anim->channel_count; c++) {
    free(model->anim->channels[c].path);
  }
  free(model->anim->channels);

  free(model->anim);
}

void load_gltf(const char* filename, Model* model) {

  cJSON* root = parse_gltf(filename);

  MeshInfo* info = malloc(sizeof(*info));
  model->info = info;
  Skin* skin = malloc(sizeof(*skin));
  model->skin = skin;
  Animation* anim = malloc(sizeof(*anim));
  model->anim = anim;
  info->uri = process_json(root, model);

  //  printf("position count = %d\n", info->vertex_count);
  //  printf("index count    = %d\n", info->index_count);
  //  printf("normal count   = %d\n", info->normal_count);

  Vertex* vertices = malloc(info->vertex_count * sizeof(Vertex));
  model->vertices = vertices;

  // load vertecies from binary file
  float* positions = load_positions(info->uri, info->position_byte_offset, info->vertex_count);

  unsigned int* indices = load_indices(info->uri, info->index_byte_offset,
				       info->index_count, info->index_component_type);

  //  printf("index_count = %d\n", info->index_count);

  model->faces = malloc(sizeof(Face) * info->index_count / 3);
  for (int i=0; i<info->index_count; i+=3) {
    int face = i/3;
    model->faces[face].vertex[0] = indices[i];
    model->faces[face].vertex[1] = indices[i+1];
    model->faces[face].vertex[2] = indices[i+2];    
  } 

  float* normals = load_normals(info->uri, info->normal_byte_offset,
				info->normal_count, info->normal_component_type);
  if (!normals) {
    printf("error loading normals\n");
    free(positions);
    free(indices);
    cJSON_Delete(root);
    return;
  }

  float* skin_inverse_bind_matrices = load_skin_inverse_matrices(info->uri, info->skin_byte_offset,
								 info->skin_count);
  if (!skin_inverse_bind_matrices) {
    printf("error loading skin inverse bind matrices\n");
    free(positions);
    free(indices);
    free(normals);
    cJSON_Delete(root);
    return;
  }

  for (int i = 0; i < info->vertex_count; i++) {
    vertices[i].position[0] = positions[i * 3 + 0];
    vertices[i].position[1] = positions[i * 3 + 1];
    vertices[i].position[2] = positions[i * 3 + 2];

    vertices[i].normal[0] = normals[i * 3 + 0];
    vertices[i].normal[1] = normals[i * 3 + 1];
    vertices[i].normal[2] = normals[i * 3 + 2];
  }

  // setup inverse bind matrices in our model
  model->skin->inverse_bind_matrices = malloc(sizeof(Mat4) * model->skin->joint_count);
  for (int i=0; i<model->skin->joint_count; i++) {
    for (int j=0; j<16; j++) {
      model->skin->inverse_bind_matrices[i].m[j] = skin_inverse_bind_matrices[i*16+j];
      //      printf("skin inverse bind matrix %d:%d:%f\n", i, j, skin_inverse_bind_matrices[i*16+j]);
    }
  }

  // load joints_0
  unsigned short *joints_0 = load_joints_0(info->uri, info->joints_0_byte_offset, info->joints_0_count);
  if (!joints_0) {
    printf("error loading joints 0\n");
    return;
  }
  for (int i=0; i<info->joints_0_count; i++) {
    for (int j=0; j<4; j++) {
      model->vertices[i].joints[j] = joints_0[i*4+j];
      //      printf("vertex %d joint %d: %d\n", i, j, model->vertices[i].joints[j]);
    }
  } 
  
  // load weights_0
  float *weights_0 = load_weights_0(info->uri, info->weights_0_byte_offset, info->weights_0_count);
  for (int i=0; i<info->weights_0_count; i++) {
    for (int j=0; j<4; j++) {
      model->vertices[i].weights[j] = weights_0[i*4+j];
      //      printf("vertex %d joint %d: %d, weight: %f\n", i, j, model->vertices[i].joints[j], model->vertices[i].weights[j]);
    }
  }
  if (!weights_0) {
    printf("error loading weights 0\n");
    return;
  }

  // load animation sample inputs
  for (int s=0; s<model->anim->sampler_count; s++) {
    
    float* inputs = load_anim_sample(info->uri, model->anim->samplers[s].input_byte_offset, model->anim->samplers[s].input_buffer_count, model->anim->samplers[s].input_buffer_type);
    if (!inputs) {
      printf("error loading inputs\n");
      return;
    }

    model->anim->samplers[s].keyframe_count = model->anim->samplers[s].input_buffer_count;
    model->anim->samplers[s].times = malloc(sizeof(float) * model->anim->samplers[s].keyframe_count);
  
    for (int i=0; i<model->anim->samplers[s].input_buffer_count; i++) {
      model->anim->samplers[s].times[i] = inputs[i];
    }

    float* outputs = load_anim_sample(info->uri, model->anim->samplers[s].output_byte_offset,
				      model->anim->samplers[s].output_buffer_count, model->anim->samplers[s].output_buffer_type);
    if (!outputs) {
      printf("error loading outputs\n");
      return;
    }

    //  model->anim->samplers[0].value_components = info->anim_sample_output_count;
    if (strcmp(model->anim->samplers[s].output_buffer_type, "VEC4") == 0) {

      model->anim->samplers[s].value_components = 4;
      model->anim->samplers[s].values = malloc(sizeof(float) * 4 * model->anim->samplers[s].output_buffer_count);
  
      for (int i=0; i<model->anim->samplers[s].output_buffer_count; i++) {
	model->anim->samplers[s].values[i*4] = outputs[i*4];
	model->anim->samplers[s].values[i*4+1] = outputs[i*4+1];
	model->anim->samplers[s].values[i*4+2] = outputs[i*4+2];
	model->anim->samplers[s].values[i*4+3] = outputs[i*4+3];
      }
    //    model->anim_outputs = outputs;
    } else if (strcmp(model->anim->samplers[s].output_buffer_type, "VEC3") == 0) {

      model->anim->samplers[s].value_components = 3;
      model->anim->samplers[s].values = malloc(sizeof(float) * 3 * model->anim->samplers[s].output_buffer_count);
  
      for (int i=0; i<model->anim->samplers[s].output_buffer_count; i++) {
	model->anim->samplers[s].values[i*3] = outputs[i*3];
	model->anim->samplers[s].values[i*3+1] = outputs[i*3+1];
	model->anim->samplers[s].values[i*3+2] = outputs[i*3+2];
      }

    } else {
      printf("unknown type %s\n", model->anim->samplers[s].output_buffer_type);
      return;
    }

    free(inputs);
    free(outputs);
  }

  free(positions);
  free(indices);
  free(normals);
  free(skin_inverse_bind_matrices);

  cJSON_Delete(root);
}

float* model_vertices_gltf(Model* model) {

  // put the data in the format opengl expects it
  
  int vertex_count = model->info->index_count;
  
  float *model_vertices = malloc(vertex_count * 14 * sizeof(float));
  
  for (int i = 0; i < model->info->index_count / 3; i++) {

    for (int j = 0; j < 3; j++) {

      unsigned int index = model->faces[i].vertex[j];

      model_vertices[(i * 3 + j) * 14 + 0] = model->vertices[index].position[0];
      model_vertices[(i * 3 + j) * 14 + 1] = model->vertices[index].position[1];
      model_vertices[(i * 3 + j) * 14 + 2] = model->vertices[index].position[2];
      
      model_vertices[(i * 3 + j) * 14 + 3] = model->vertices[index].normal[0];
      model_vertices[(i * 3 + j) * 14 + 4] = model->vertices[index].normal[1];
      model_vertices[(i * 3 + j) * 14 + 5] = model->vertices[index].normal[2];
      
      model_vertices[(i * 3 + j) * 14 + 6] = model->vertices[index].joints[0];
      model_vertices[(i * 3 + j) * 14 + 7] = model->vertices[index].joints[1];
      model_vertices[(i * 3 + j) * 14 + 8] = model->vertices[index].joints[2];
      model_vertices[(i * 3 + j) * 14 + 9] = model->vertices[index].joints[3];      
      
      model_vertices[(i * 3 + j) * 14 + 10] = model->vertices[index].weights[0];
      model_vertices[(i * 3 + j) * 14 + 11] = model->vertices[index].weights[1];
      model_vertices[(i * 3 + j) * 14 + 12] = model->vertices[index].weights[2];
      model_vertices[(i * 3 + j) * 14 + 13] = model->vertices[index].weights[3];      
    }

  }
  
  return model_vertices;
}

Mat4 node_local_matrix(Node * node) {

  Mat4 T = mat4_translation(node->translation.x,
			    node->translation.y,
			    node->translation.z);

  Mat4 R = quaternion_to_mat4(node->rotation);

  Mat4 S = mat4_scale(node->scale.x, node->scale.y, node->scale.z);

  return mat4_multiply(T, mat4_multiply(R, S));
}

Mat4 get_node_world_matrix(Model * model, int nodeIndex) {

  Node * node = &model->nodes[nodeIndex];
  Mat4 local = node_local_matrix(node);

  if (node->parent < 0) {
    return local;
  }
  Mat4 parent = get_node_world_matrix(model, node->parent);
  return mat4_multiply(parent, local);
}

#endif
