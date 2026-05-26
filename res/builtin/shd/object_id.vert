#version 330 core

layout(std140) uniform blk_camera {
	mat4 u_projection_matrix;
	mat4 u_view_matrix;
};

layout(std140) uniform blk_object {
	mat4 u_vertex_matrix;
	uint u_object_id;
};

layout(location=0) in vec3 a_pos;

out uint v_object_id;

void main() {
	v_object_id = u_object_id;
	gl_Position = u_projection_matrix * u_view_matrix * u_vertex_matrix * vec4(a_pos, 1.0);
}
