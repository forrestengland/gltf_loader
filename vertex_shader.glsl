attribute vec3 position;
attribute vec3 normal;

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
