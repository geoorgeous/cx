#version 330 core

layout(std140) uniform blk_material_properties {
	vec4  u_color;
};

out vec4 f_color;

void main() {
	f_color = u_color;
}
