#version 330 core

layout(std140) uniform blk_camera {
	mat4 u_projection_matrix;
	mat4 u_view_matrix;
};

layout(std140) uniform blk_object {
	mat4 u_vertex_matrix;
};

layout (location=0) in vec3 a_pos;
layout (location=1) in vec3 a_normal;
layout (location=3) in vec2 a_texcoords;

out vec3 v_normal;
out vec2 v_texcoords;

void main() {
	v_normal = normalize(mat3(transpose(inverse(u_vertex_matrix))) * a_normal);
	v_texcoords = a_texcoords;
	gl_Position = u_projection_matrix * u_view_matrix * u_vertex_matrix * vec4(a_pos, 1.0);
}
