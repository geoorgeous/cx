#version 330 core

uniform sampler2D u_texture_albedo;

in vec2 v_texcoords;
in vec4 v_color;

out vec4 f_color;

void main() {
	float r = texture(u_texture_albedo, v_texcoords).r;
	f_color = vec4(v_color.r, v_color.g, v_color.b, v_color.a * r);
}
