#version 330 core

uniform sampler2D u_texture;

in vec4 v_color;
in vec2 v_texcoords;

out vec4 f_color;

void main() {
	vec4 texture_rgba = texture(u_texture, v_texcoords).rgba;
	f_color = texture_rgba * v_color;
}
