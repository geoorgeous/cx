#version 330 core

layout(std140) uniform blk_object {
	mat4 u_vertex_matrix;
	uint u_object_id;
};

out uint f_color;

void main() {
	f_color = u_object_id;
}
