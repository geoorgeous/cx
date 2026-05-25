#version 330 core

layout(std140) uniform blk_material_properties {
	vec3 u_color;
};

uniform sampler2D u_texture_albedo;

in vec3 v_normal;
in vec2 v_texcoords;

out vec4 f_color;

void main() {
	const vec3 light_dir = normalize(-vec3(1, 1, 1));
	const float ka = 0.3;

	vec3 texture_rgb = texture(u_texture_albedo, v_texcoords).rgb;
	vec3 albedo = texture_rgb * u_color;

	float kd = max(dot(v_normal, -light_dir), 0);

	f_color = vec4(min((ka + kd), 1) * albedo, 1);
}
