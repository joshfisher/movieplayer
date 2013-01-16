#version 150

uniform mat4 viewMatrix, projectionMatrix;

in vec2 position;
in vec2 texCoords;

out vec2 coords;

void main() {
	coords = texCoords;
	gl_Position = projectionMatrix * viewMatrix * vec4(position,0.0,1.0);
}