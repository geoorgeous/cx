#version 330 core

layout(std140) uniform blk_camera {
	mat4 u_view_projection_matrix;
	mat4 u_inv_view_projection_matrix;
};

in vec2 v_texcoords;

out vec4 f_color;

float grid(vec2 pos, float scale) {
	vec2 coord = pos / scale;

	vec2 d = fwidth(coord);

	vec2 g = abs(fract(coord - 0.5) - 0.5) / max(d, vec2(1e-6));

	return 1.0 - min(min(g.x, g.y), 1.0);
}

float compute_fade(vec3 frag_pos, vec3 view_pos, float fade_start, float fade_size) {
	vec3 diff = frag_pos - view_pos;
	float dist_sqr = dot(diff, diff);
	float fade_end = fade_start + fade_size;
	float fade = 1.0 - smoothstep(fade_start * fade_start, fade_end * fade_end, dist_sqr);
	return fade * fade;
}

void main() {
	vec2 ndc = v_texcoords * 2.0 - 1.0;

	vec4 near = u_inv_view_projection_matrix * vec4(ndc, -1.0, 1.0);
	vec4 far = u_inv_view_projection_matrix * vec4(ndc, 1.0, 1.0);

	near /= near.w;
	far /= far.w;

	vec3 ray_origin = near.xyz;
	vec3 ray_dir = far.xyz - ray_origin;

	float t = -ray_origin.y / ray_dir.y;

	if (t <= 0.0) {
		discard;
	}

	vec3 world_pos = ray_origin + ray_dir * t;

	float minor = grid(world_pos.xz, 1.0);
	float major = grid(world_pos.xz, 10.0);
	float line = max(minor, major);

	vec3 color = vec3(0.2) * minor;
	color += vec3(0.2) * major;

	float axis_x = 1.0 - min(abs(world_pos.x) / fwidth(world_pos.x), 1.0);
	float axis_z = 1.0 - min(abs(world_pos.z) / fwidth(world_pos.z), 1.0);

	color = mix(color, vec3(0.243, 0.478, 1.000), axis_x);
	color = mix(color, vec3(0.961, 0.306, 0.306), axis_z);

	vec4 clip = u_view_projection_matrix * vec4(world_pos, 1.0);

	float ndc_depth = clip.z / clip.w;
	gl_FragDepth = ndc_depth * 0.5 + 0.5;

	float alpha = max(minor, major) * compute_fade(world_pos, ray_origin, 450.0, 550.0);

	f_color = vec4(color, alpha);
}
