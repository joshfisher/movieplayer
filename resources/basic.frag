#version 150

uniform sampler2D texUnit;

in vec2 coords;

out vec4 color;

void main() {
	color = texture(texUnit,coords);
}