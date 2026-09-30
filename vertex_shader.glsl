attribute vec3 position;
attribute vec3 normal;
attribute vec4 joints;
attribute vec4 weights;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

uniform mat4 jointMatrices[21];

varying vec3 vertexNormal;

void main()
{
    vec4 skinnedPosition =
          weights.x * jointMatrices[int(joints.x)] * vec4(position, 1.0)
        + weights.y * jointMatrices[int(joints.y)] * vec4(position, 1.0)
        + weights.z * jointMatrices[int(joints.z)] * vec4(position, 1.0)
        + weights.w * jointMatrices[int(joints.w)] * vec4(position, 1.0);

    vec3 skinnedNormal = 
         weights.x * mat3(jointMatrices[int(joints.x)]) * normal
        + weights.y * mat3(jointMatrices[int(joints.y)]) * normal
        + weights.z * mat3(jointMatrices[int(joints.z)]) * normal
        + weights.w * mat3(jointMatrices[int(joints.w)]) * normal;

    vertexNormal = normalize(skinnedNormal);

    vertexNormal = normal;

    gl_Position =
        projectionMatrix *
        viewMatrix *
        modelMatrix *
        skinnedPosition;
}

/*
attribute vec3 position;
attribute vec3 normal;
attribute vec4 joints;
attribute vec4 weights;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

varying vec3 vertexNormal;

void main() {

       gl_Position =
        projectionMatrix *
        viewMatrix *
        modelMatrix *
        vec4(position, 1.0);

    vertexNormal = normal;
}
*/