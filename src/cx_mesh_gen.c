#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "cx_mesh_data.h"
#include "cx_mesh_gen.h"
#include "darr.h"
#include "half_edge.h"
#include "math_utils.h"
#include "vector.h"

void cx_mesh_gen_quad(
	float half_width,
	float half_height,
	const float* p_normal,
	struct cx_mesh_data* p_out) {

	float tangent[3];
	float bitangent[3];
	vec3_orthonormal_basis(p_normal, tangent, bitangent);

	const float tx = half_width * tangent[0];
	const float ty = half_width * tangent[1];
	const float tz = half_width * tangent[2];

	const float bx = half_height * bitangent[0];
	const float by = half_height * bitangent[1];
	const float bz = half_height * bitangent[2];

	const float vertices[] = {
		-tx - bx, -ty - by, -tz - bz, p_normal[0], p_normal[1], p_normal[2],
		 tx - bx,  ty - by,  tz - bz, p_normal[0], p_normal[1], p_normal[2],
		 tx + bx,  ty + by,  tz + bz, p_normal[0], p_normal[1], p_normal[2],

		 tx + bx,  ty + by,  tz + bz, p_normal[0], p_normal[1], p_normal[2],
		-tx + bx, -ty + by, -tz + bz, p_normal[0], p_normal[1], p_normal[2],
		-tx - bx, -ty - by, -tz - bz, p_normal[0], p_normal[1], p_normal[2],
	};
	
	const size_t num_vertices = 6;
	const size_t vertex_size = sizeof(float) * 6;
	const size_t vertices_size = num_vertices * vertex_size;

	float* p_vertices = malloc(vertices_size);
	memcpy(p_vertices, vertices, vertices_size);

	*p_out = (struct cx_mesh_data) {
		.layout = {
			.num_vertex_buffers = 1,
			.p_attributes = malloc(sizeof(*p_out->layout.p_attributes) * 2),
			.num_attributes = 2,
			.draw_mode = CX_MESH_DRAW_MODE_triangles
		},
		.p_vertex_buffers = malloc(sizeof(*p_out->p_vertex_buffers)),
		.vertex_count = num_vertices,
		.bounds_min = {
			-(fabsf(tx) + fabsf(bx)),
			-(fabsf(ty) + fabsf(by)),
			-(fabsf(tz) + fabsf(bz))
		},
		.bounds_max = {
			 (fabsf(tx) + fabsf(bx)),
			 (fabsf(ty) + fabsf(by)),
			 (fabsf(tz) + fabsf(bz))
		}
	};

	p_out->bounds_max[0] = -p_out->bounds_min[0];
	p_out->bounds_max[1] = -p_out->bounds_min[1];
	p_out->bounds_max[2] = -p_out->bounds_min[2];

	*p_out->p_vertex_buffers = (struct cx_mesh_vertex_buffer) {
		.p_bytes = p_vertices,
		.size = vertices_size
	};

	p_out->layout.p_attributes[0] = (struct cx_mesh_vertex_attribute) {
		.index = 0,
		.vertex_buffer_index = 0,
		.format = {
			.type  = CX_MESH_VERTEX_ATTRIBUTE_TYPE_f32,
			.count = 3
		},
		.layout = {
			.stride = vertex_size
		}
	};

	p_out->layout.p_attributes[1] = (struct cx_mesh_vertex_attribute) {
		.index = 1,
		.vertex_buffer_index = 0,
		.format = {
			.type  = CX_MESH_VERTEX_ATTRIBUTE_TYPE_f32,
			.count = 3
		},
		.layout = {
			.offset = sizeof(float) * 3,
			.stride = vertex_size
		}
	};
}

void cx_mesh_gen_box(const float p_half_size[3], struct cx_mesh_data* p_out) {
	const float vertices[] = {
		-p_half_size[0], -p_half_size[1], -p_half_size[2], -1,  0,  0,
		-p_half_size[0],  p_half_size[1],  p_half_size[2], -1,  0,  0,
		-p_half_size[0],  p_half_size[1], -p_half_size[2], -1,  0,  0,

		-p_half_size[0],  p_half_size[1],  p_half_size[2], -1,  0,  0,
		-p_half_size[0], -p_half_size[1], -p_half_size[2], -1,  0,  0,
		-p_half_size[0], -p_half_size[1],  p_half_size[2], -1,  0,  0,

		 p_half_size[0], -p_half_size[1],  p_half_size[2],  1,  0,  0,
		 p_half_size[0],  p_half_size[1], -p_half_size[2],  1,  0,  0,
		 p_half_size[0],  p_half_size[1],  p_half_size[2],  1,  0,  0,

		 p_half_size[0],  p_half_size[1], -p_half_size[2],  1,  0,  0,
		 p_half_size[0], -p_half_size[1],  p_half_size[2],  1,  0,  0,
		 p_half_size[0], -p_half_size[1], -p_half_size[2],  1,  0,  0,

		-p_half_size[0], -p_half_size[1], -p_half_size[2],  0, -1,  0,
		 p_half_size[0], -p_half_size[1],  p_half_size[2],  0, -1,  0,
		-p_half_size[0], -p_half_size[1],  p_half_size[2],  0, -1,  0,

		 p_half_size[0], -p_half_size[1],  p_half_size[2],  0, -1,  0,
		-p_half_size[0], -p_half_size[1], -p_half_size[2],  0, -1,  0,
		 p_half_size[0], -p_half_size[1], -p_half_size[2],  0, -1,  0,

		 p_half_size[0],  p_half_size[1], -p_half_size[2],  0,  1,  0,
		-p_half_size[0],  p_half_size[1],  p_half_size[2],  0,  1,  0,
		 p_half_size[0],  p_half_size[1],  p_half_size[2],  0,  1,  0,

		-p_half_size[0],  p_half_size[1],  p_half_size[2],  0,  1,  0,
		 p_half_size[0],  p_half_size[1], -p_half_size[2],  0,  1,  0,
		-p_half_size[0],  p_half_size[1], -p_half_size[2],  0,  1,  0,

		-p_half_size[0], -p_half_size[1], -p_half_size[2],  0,  0, -1,
		 p_half_size[0],  p_half_size[1], -p_half_size[2],  0,  0, -1,
		 p_half_size[0], -p_half_size[1], -p_half_size[2],  0,  0, -1,

		 p_half_size[0],  p_half_size[1], -p_half_size[2],  0,  0, -1,
		-p_half_size[0], -p_half_size[1], -p_half_size[2],  0,  0, -1,
		-p_half_size[0],  p_half_size[1], -p_half_size[2],  0,  0, -1,

		-p_half_size[0], -p_half_size[1],  p_half_size[2],  0,  0,  1,
		 p_half_size[0],  p_half_size[1],  p_half_size[2],  0,  0,  1,
		-p_half_size[0],  p_half_size[1],  p_half_size[2],  0,  0,  1,

		 p_half_size[0],  p_half_size[1],  p_half_size[2],  0,  0,  1,
		-p_half_size[0], -p_half_size[1],  p_half_size[2],  0,  0,  1,
		 p_half_size[0], -p_half_size[1],  p_half_size[2],  0,  0,  1,
	};
	
	const size_t num_vertices = 36;
	const size_t vertex_size = sizeof(float) * 6;
	const size_t vertices_size = num_vertices * vertex_size;

	float* p_vertices = malloc(vertices_size);
	memcpy(p_vertices, vertices, vertices_size);

	*p_out = (struct cx_mesh_data) {
		.layout = {
			.num_vertex_buffers = 1,
			.p_attributes = malloc(sizeof(*p_out->layout.p_attributes) * 2),
			.num_attributes = 2,
			.draw_mode = CX_MESH_DRAW_MODE_triangles
		},
		.p_vertex_buffers = malloc(sizeof(*p_out->p_vertex_buffers)),
		.vertex_count = num_vertices,
		.bounds_min = { -p_half_size[0], -p_half_size[1], -p_half_size[2] },
		.bounds_max = {  p_half_size[0],  p_half_size[1],  p_half_size[2] }
	};

	*p_out->p_vertex_buffers = (struct cx_mesh_vertex_buffer) {
		.p_bytes = p_vertices,
		.size = vertices_size
	};

	p_out->layout.p_attributes[0] = (struct cx_mesh_vertex_attribute) {
		.index = 0,
		.vertex_buffer_index = 0,
		.format = {
			.type = CX_MESH_VERTEX_ATTRIBUTE_TYPE_f32,
			.count = 3
		},
		.layout = {
			.stride = vertex_size,
		}
	};

	p_out->layout.p_attributes[1] = (struct cx_mesh_vertex_attribute) {
		.index = 1,
		.vertex_buffer_index = 0,
		.format = {
			.type = CX_MESH_VERTEX_ATTRIBUTE_TYPE_f32,
			.count = 3
		},
		.layout = {
			.offset = sizeof(float) * 3,
			.stride = vertex_size,
		}
	};
}

void cx_mesh_gen_sphere(
	const float radius,
	const uint16_t rings,
	const uint16_t segments,
	struct cx_mesh_data* p_out) {
	
	const uint16_t num_vertices = (uint16_t)((rings - 1) * segments + 2);
	const size_t vertex_size = sizeof(float) * 6;
	const size_t vertices_size = num_vertices * vertex_size;
	float* const p_vertices = malloc(vertices_size);
	float* p_v = p_vertices;

	const float ring_theta = CX_MATH_TAU / rings;
	const float sgmt_theta = CX_MATH_TAU / segments;

	*p_v++ = 0;
	*p_v++ = radius;
	*p_v++ = 0;

	*p_v++ = 0;
	*p_v++ = 1;
	*p_v++ = 0;

	for (uint16_t i_ring = 1; i_ring < rings; ++i_ring) {
		const float i_ring_half_theta = i_ring * ring_theta * 0.5f;
		const float c = cosf(i_ring_half_theta);
		const float s = sinf(i_ring_half_theta);
		for (uint16_t i_sgmt = 0; i_sgmt < segments; ++i_sgmt) {
			const float i_sgmt_theta = i_sgmt * sgmt_theta;
			
			const float x = cosf(i_sgmt_theta) * s;
			const float z = sinf(i_sgmt_theta) * s;

			*p_v++ = radius * x;
			*p_v++ = radius * c;
			*p_v++ = radius * z;

			*p_v++ = x;
			*p_v++ = c;
			*p_v++ = z;
		}
	}

	*p_v++ = 0;
	*p_v++ = -radius;
	*p_v++ = 0;

	*p_v++ = 0;
	*p_v++ = -1;
	*p_v++ = 0;

	const uint16_t num_indices = (uint16_t)(segments * (rings - 1) * 6);
	uint16_t* const p_indices = malloc(num_indices * sizeof(uint16_t));
	uint16_t* p_i = p_indices;

	// top pole faces
	for (uint16_t i = 0; i < segments - 1; ++i) {
		*p_i++ = 0;
		*p_i++ = i + 2;
		*p_i++ = i + 1;
	}
	*p_i++ = 0;
	*p_i++ = 1;
	*p_i++ = segments;

	// ring quad strip faces
	for (uint16_t i_ring = 0; i_ring < rings - 2; ++i_ring) {
		for (uint16_t i_sgmt = 0; i_sgmt < segments - 1; ++i_sgmt) {
			const uint16_t i_vert = (uint16_t)((segments * i_ring) + i_sgmt + 1);
			
			*p_i++ = i_vert;
			*p_i++ = i_vert + 1;
			*p_i++ = i_vert + segments;
			
			*p_i++ = i_vert + segments;
			*p_i++ = i_vert + 1;
			*p_i++ = (uint16_t)(i_vert + segments + 1);
		}

		*p_i++ = (uint16_t)(segments * (i_ring + 1));
		*p_i++ = (uint16_t)(segments * i_ring + 1);
		*p_i++ = (uint16_t)(segments * (i_ring + 2));

		*p_i++ = (uint16_t)(segments * (i_ring + 2));
		*p_i++ = (uint16_t)(segments * i_ring + 1);
		*p_i++ = (uint16_t)(segments * (i_ring + 1) + 1);
	}

	// bottom pole faces
	const uint16_t last = num_vertices - 1;
	for (uint16_t i = 0; i < segments - 1; ++i) {
		*p_i++ = last;
		*p_i++ = (uint16_t)(last - (i + 2));
		*p_i++ = (uint16_t)(last - (i + 1));
	}
	*p_i++ = last;
	*p_i++ = last - 1;
	*p_i++ = last - segments;

	*p_out = (struct cx_mesh_data) {
		.layout = {
			.num_vertex_buffers = 1,
			.p_attributes = malloc(sizeof(*p_out->layout.p_attributes) * 2),
			.num_attributes = 2,
			.index_type = CX_MESH_VERTEX_INDEX_TYPE_u16,
			.draw_mode = CX_MESH_DRAW_MODE_triangles

		},
		.p_vertex_buffers = malloc(sizeof(*p_out->p_vertex_buffers)),
		.vertex_count = num_vertices,
		.index_buffer = {
			.p_bytes = p_indices,
			.count = num_indices,
		},
		.bounds_min = { -radius, -radius, -radius },
		.bounds_max = {  radius,  radius,  radius }
	};

	*p_out->p_vertex_buffers = (struct cx_mesh_vertex_buffer) {
		.p_bytes = p_vertices,
		.size = vertices_size
	};

	p_out->layout.p_attributes[0] = (struct cx_mesh_vertex_attribute) {
		.index = 0,
		.vertex_buffer_index = 0,
		.format = {
			.type = CX_MESH_VERTEX_ATTRIBUTE_TYPE_f32,
			.count = 3
		},
		.layout = {
			.stride = vertex_size,
		}
	};

	p_out->layout.p_attributes[1] = (struct cx_mesh_vertex_attribute) {
		.index = 1,
		.vertex_buffer_index = 0,
		.format = {
			.type = CX_MESH_VERTEX_ATTRIBUTE_TYPE_f32,
			.count = 3
		},
		.layout = {
			.offset = sizeof(float) * 3,
			.stride = vertex_size,
		}
	};
}

void cx_mesh_gen_hemisphere(
	const float radius,
	const uint16_t rings, const uint16_t segments,
	struct cx_mesh_data* p_out) {

	const size_t num_vertices = (size_t)(segments * rings + 1);
	const size_t vertex_size = sizeof(float) * 6;
	const size_t vertices_size = num_vertices * vertex_size;
	float* const p_vertices = malloc(vertices_size);
	float* p_v = p_vertices;

	const float ring_theta = CX_MATH_TAU / rings;
	const float sgmt_theta = CX_MATH_TAU / segments;

	*p_v++ = 0;
	*p_v++ = radius;
	*p_v++ = 0;

	*p_v++ = 0;
	*p_v++ = 1;
	*p_v++ = 0;

	for (uint16_t i_ring = 1; i_ring <= rings; ++i_ring) {
		const float i_ring_half_theta = i_ring * ring_theta * 0.25f;
		const float c = cosf(i_ring_half_theta);
		const float s = sinf(i_ring_half_theta);

		for (uint16_t i_sgmt = 0; i_sgmt < segments; ++i_sgmt) {
			const float i_sgmt_theta = i_sgmt * sgmt_theta;		
			const float x = cosf(i_sgmt_theta) * s;
			const float z = sinf(i_sgmt_theta) * s;

			*p_v++ = radius * x;
			*p_v++ = radius * c;
			*p_v++ = radius * z;

			*p_v++ = x;
			*p_v++ = c;
			*p_v++ = z;
		}
	}

	const size_t num_indices = (size_t)(segments * 3 + (rings - 1) * segments * 6);
	const size_t indices_size = num_indices * sizeof(uint16_t);
	uint16_t* const p_indices = malloc(indices_size);
	uint16_t* p_i = p_indices;

	// cap faces
	for (uint16_t i = 0; i < segments - 1; ++i) {
		*p_i++ = 0;
		*p_i++ = i + 2;
		*p_i++ = i + 1;
	}

	*p_i++ = 0;
	*p_i++ = 1;
	*p_i++ = segments;

	// ring quad strip faces
	for (uint16_t i_ring = 0; i_ring < rings - 1; ++i_ring) {
		for (uint16_t i_sgmt = 0; i_sgmt < segments - 1; ++i_sgmt) {
			const uint16_t i_vert = (uint16_t)((segments * i_ring) + i_sgmt + 1);
			
			*p_i++ = i_vert;
			*p_i++ = i_vert + 1;
			*p_i++ = i_vert + segments;
			
			*p_i++ = i_vert + segments;
			*p_i++ = i_vert + 1;
			*p_i++ = (uint16_t)(i_vert + segments + 1);
		}

		*p_i++ = (uint16_t)(segments * (i_ring + 1));
		*p_i++ = (uint16_t)(segments * i_ring + 1);
		*p_i++ = (uint16_t)(segments * (i_ring + 2));

		*p_i++ = (uint16_t)(segments * (i_ring + 2));
		*p_i++ = (uint16_t)(segments * i_ring + 1);
		*p_i++ = (uint16_t)(segments * (i_ring + 1) + 1);
	}
	
	*p_out = (struct cx_mesh_data) {
		.layout = {
			.num_vertex_buffers = 1,
			.p_attributes = malloc(sizeof(*p_out->layout.p_attributes) * 2),
			.num_attributes = 2,
			.index_type = CX_MESH_VERTEX_INDEX_TYPE_u16,
			.draw_mode = CX_MESH_DRAW_MODE_triangles
		},
		.p_vertex_buffers = malloc(sizeof(*p_out->p_vertex_buffers)),
		.vertex_count = num_vertices,
		.index_buffer = {
			.p_bytes = p_indices,
			.count = num_indices,
		},
		.bounds_min = { -radius, -radius, -radius },
		.bounds_max = {  radius,  radius,  radius }
	};

	*p_out->p_vertex_buffers = (struct cx_mesh_vertex_buffer) {
		.p_bytes = p_vertices,
		.size = vertices_size
	};

	p_out->layout.p_attributes[0] = (struct cx_mesh_vertex_attribute) {
		.index               = 0,
		.vertex_buffer_index = 0,
		.format = {
			.type = CX_MESH_VERTEX_ATTRIBUTE_TYPE_f32,
			.count = 3
		},
		.layout = {
			.stride          = vertex_size,
		}
	};

	p_out->layout.p_attributes[1] = (struct cx_mesh_vertex_attribute) {
		.index               = 1,
		.vertex_buffer_index = 0,
		.format = {
			.type = CX_MESH_VERTEX_ATTRIBUTE_TYPE_f32,
			.count = 3,
		},
		.layout = {
			.offset          = sizeof(float) * 3,
			.stride          = vertex_size,
		}
	};
}

void cx_mesh_gen_cylinder(
	const float radius_a, const float radius_b, const float half_length,
	const uint16_t segments,
	const int b_cap_a, const int b_cap_b,
	struct cx_mesh_data* p_out) {
	
	const uint16_t sleeve_num_vertices = segments * 2;
	const uint16_t cap_num_vertices = segments + 1;
	const uint16_t num_vertices =
		(uint16_t)(sleeve_num_vertices + (!!b_cap_a) * cap_num_vertices + (!!b_cap_b) * cap_num_vertices);
	const size_t vertex_size = sizeof(float) * 6;
	const size_t vertices_size = vertex_size * num_vertices;
	float* const p_vertices = malloc(vertices_size);
	float*       p_v = p_vertices;

	const float ymin = -half_length;
	const float ymax = half_length;
	const float theta = CX_MATH_TAU / segments;

	for (uint16_t i = 0; i < segments; ++i) {
		const float x = cosf(theta * i);
		const float z = sinf(theta * i);

		*p_v++ = x * radius_a;
		*p_v++ = ymax;
		*p_v++ = z * radius_a;

		*p_v++ = x;
		*p_v++ = 0;
		*p_v++ = z;

		*p_v++ = x * radius_b;
		*p_v++ = ymin;
		*p_v++ = z * radius_b;

		*p_v++ = x;
		*p_v++ = 0;
		*p_v++ = z;
	}

#define GENERATE_CAP_VERTICES(P_V, Y, R, NY, N, T)\
	*P_V++ = 0;\
	*P_V++ = Y;\
	*P_V++ = 0;\
	for (uint16_t i = 0; i < N; ++i) {\
		*P_V++ = cosf(T * i) * R;\
		*P_V++ = Y;\
		*P_V++ = sinf(T * i) * R;\
		*P_V++ = 0;\
		*P_V++ = NY;\
		*P_V++ = 0;\
	}

	if (b_cap_a) {
		GENERATE_CAP_VERTICES(p_v, ymax, radius_a,  1, segments, theta);
	}

	if (b_cap_b) {
		GENERATE_CAP_VERTICES(p_v, ymin, radius_b, -1, segments, theta);
	}

#undef GENERATE_CAP

	const uint16_t cap_num_indices = segments * 3;
	const uint16_t num_indices =
		(uint16_t)((6 * segments) + (!!b_cap_a) * cap_num_indices + (!!b_cap_b) * cap_num_indices);
	const size_t indices_size = sizeof(uint16_t) * num_indices;
	uint16_t* const p_indices = malloc(indices_size);
	uint16_t* p_i = p_indices;

	for (uint16_t i = 0; i < segments - 1; ++i) {
		*p_i++ = i * 2;
		*p_i++ = (uint16_t)(i * 2 + 2);
		*p_i++ = (uint16_t)(i * 2 + 1);

		*p_i++ = (uint16_t)(i * 2 + 1);
		*p_i++ = (uint16_t)(i * 2 + 2);
		*p_i++ = (uint16_t)(i * 2 + 3);
	}

	*p_i++ = (uint16_t)(segments * 2 - 2);
	*p_i++ = 0;
	*p_i++ = (uint16_t)(segments * 2 - 1);

	*p_i++ = (uint16_t)(segments * 2 - 1);
	*p_i++ = 0;
	*p_i++ = 1;

#define GENERATE_CAP_INDICES(P_I, N, OFFSET)\
		for (uint16_t i = 0; i < (N) - 1; ++i) {\
			*(P_I)++ = (uint16_t)((OFFSET) + 0);\
			*(P_I)++ = (uint16_t)((OFFSET) + i + 1);\
			*(P_I)++ = (uint16_t)((OFFSET) + i + 2);\
		}\
		*(P_I)++ = (uint16_t)((OFFSET) + 0);\
		*(P_I)++ = (uint16_t)((OFFSET) + (N) + 1);\
		*(P_I)++ = (uint16_t)((OFFSET) + 1);

	if (b_cap_a) {
		GENERATE_CAP_INDICES(p_i, segments, segments * 2);
	}

	if (b_cap_b) {
		GENERATE_CAP_INDICES(p_i, segments, segments * 2 + (uint16_t)(!!b_cap_a) * (segments + 1));
	}

#undef GENERATE_CAP_INDICES

	const float radius_max = radius_a > radius_b ? radius_a : radius_b;

	*p_out = (struct cx_mesh_data) {
		.layout = {
			.num_vertex_buffers = 1,
			.p_attributes = malloc(sizeof(*p_out->layout.p_attributes) * 2),
			.num_attributes = 2,
			.index_type = CX_MESH_VERTEX_INDEX_TYPE_u16,
			.draw_mode = CX_MESH_DRAW_MODE_triangles
		},
		.p_vertex_buffers = malloc(sizeof(*p_out->p_vertex_buffers)),
		.vertex_count = num_vertices,
		.index_buffer = {
			.p_bytes = p_indices,
			.count = num_indices,
		},
		.bounds_min = { -radius_max, ymin, -radius_max },
		.bounds_max = {  radius_max, ymax,  radius_max }
	};

	*p_out->p_vertex_buffers = (struct cx_mesh_vertex_buffer) {
		.p_bytes = p_vertices,
		.size = vertices_size
	};

	p_out->layout.p_attributes[0] = (struct cx_mesh_vertex_attribute) {
		.index               = 0,
		.vertex_buffer_index = 0,
		.format = {
			.type = CX_MESH_VERTEX_ATTRIBUTE_TYPE_f32,
			.count = 3
		},
		.layout = {
			.stride          = vertex_size,
		}
	};

	p_out->layout.p_attributes[1] = (struct cx_mesh_vertex_attribute) {
		.index               = 1,
		.vertex_buffer_index = 0,
		.format = {
			.type = CX_MESH_VERTEX_ATTRIBUTE_TYPE_f32,
			.count = 3,
		},
		.layout = {
			.offset          = sizeof(float) * 3,
			.stride          = vertex_size,
		}
	};
}

void cx_mesh_gen_from_halfedge_mesh(
	const struct he_mesh* p_he_mesh,
	struct cx_mesh_data* p_out) {
	
	struct darr vertices;
	darr_init(&vertices, sizeof(float) * 6);

	struct he_face* p_face = p_he_mesh->p_faces;

	while (p_face) {
		size_t num_vertices = 0;

		struct he_edge* p_edge = p_face->p_edges;

		float p_ab[3];
		float p_ca[3];

		vec3_sub(p_edge->p_next->p_tail->position, p_edge->p_tail->position, p_ab);
		vec3_sub(p_edge->p_tail->position, p_edge->p_prev->p_tail->position, p_ca);

		float p_normal[3];
		vec3_cross(p_ca, p_ab, p_normal);

		do {
			float* p_vertex = darr_push(&vertices);
			vec3_copy(p_edge->p_tail->position, p_vertex);
			vec3_copy(p_normal, &p_vertex[3]);

			++num_vertices;

			if (num_vertices > 3) {
				p_vertex = darr_push(&vertices);
				vec3_copy(p_face->p_edges->p_tail->position, p_vertex);
				vec3_copy(p_normal, &p_vertex[3]);
				
				p_vertex = darr_push(&vertices);
				vec3_copy(p_edge->p_prev->p_tail->position, p_vertex);
				vec3_copy(p_normal, &p_vertex[3]);
			}

			p_edge = p_edge->p_next;
		} while (p_edge != p_face->p_edges);

		p_face = p_face->p_next;
	};

	darr_shrink(&vertices);

	*p_out = (struct cx_mesh_data) {
		.layout = {
			.num_vertex_buffers = 1,
			.p_attributes = malloc(sizeof(*p_out->layout.p_attributes) * 2),
			.num_attributes = 2,
			.draw_mode = CX_MESH_DRAW_MODE_triangles
		},
		.p_vertex_buffers   = malloc(sizeof(*p_out->p_vertex_buffers)),
		.vertex_count       = vertices.length_,
	};

	*p_out->p_vertex_buffers = (struct cx_mesh_vertex_buffer) {
		.p_bytes = vertices.p_buffer_,
		.size = vertices.capacity_ * vertices.element_size_
	};

	p_out->layout.p_attributes[0] = (struct cx_mesh_vertex_attribute) {
		.index               = 0,
		.vertex_buffer_index = 0,
		.format = {
			.type = CX_MESH_VERTEX_ATTRIBUTE_TYPE_f32,
			.count = 3
		},
		.layout = {
			.stride          = vertices.element_size_,
		}
	};

	p_out->layout.p_attributes[1] = (struct cx_mesh_vertex_attribute) {
		.index               = 1,
		.vertex_buffer_index = 0,
		.format = {
			.type = CX_MESH_VERTEX_ATTRIBUTE_TYPE_f32,
			.count = 3
		},
		.layout = {
			.offset          = sizeof(float) * 3,
			.stride          = vertices.element_size_,
		}
	};
}

void cx_mesh_gen_free(struct cx_mesh_data* p_out) {
	free(p_out->p_vertex_buffers[0].p_bytes);
	free(p_out->p_vertex_buffers);
	free(p_out->layout.p_attributes);
	free(p_out->index_buffer.p_bytes);
	*p_out = (struct cx_mesh_data) {0};
}
