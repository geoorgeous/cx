#version 330 core

uniform sampler2D u_texture;

in vec4 v_color;
in vec2 v_texcoords;

out vec4 f_color;

void main() {
	float r = texture(u_texture, v_texcoords).r;
	f_color = vec4(v_color.rgb, v_color.a * r);
}
