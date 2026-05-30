#include "cx_text_mesher.h"

#include <stdint.h>
#include <stdlib.h>

#include "cx_font.h"
#include "cx_texture_atlas_layout.h"
#include "matrix.h"
#include "mesh.h"

#define CX_TAB_SPACE_COUNT 4

void cx_text_mesher_measure(
	const char* s,
	size_t n,
	const struct cx_font_render_data* p_font_render_data,
	float scale,
	float* p_out_x, float* p_out_y) {
	
	float x = 0;
	float y = (float)p_font_render_data->p_font->line_height_;

	float line_x = x;

	for(const char* p = s; *p && n; p++, --n) {
		const uint32_t codepoint = (uint32_t)*p;

		if (codepoint >= CX_FONT_NUM_GLYPHS) {
			continue;
		}

		if (*p == ' ') {
			line_x += (float)p_font_render_data->p_font->max_glyph_width_;
			continue;	
		}

		if (*p == '\t') {
			line_x += (float)p_font_render_data->p_font->max_glyph_width_ * CX_TAB_SPACE_COUNT;
			continue;
		}

		if (*p == '\n') {
			if (line_x > x) {
				x = line_x;
			}
			y += (float)p_font_render_data->p_font->line_height_;
			line_x = 0;
			continue;
		}

		const struct cx_font_glyph* p_glyph = &p_font_render_data->p_font->glyphs_[codepoint];

		if (!p_glyph->codepoint_) {
			// missing glyph
			continue;
		}

		line_x += (float)p_glyph->metrics_.adv_x;
	}

	if (line_x > x) {
		x = line_x;
	}
			
	*p_out_x = x * scale;
	*p_out_y = y * scale;
}

void cx_text_mesher_generate(
	const struct cx_text_mesher_input* p_input,
	size_t max_outputs,
	struct cx_text_mesher_output* p_out_outputs,
	size_t* p_out_num_outputs) {

	(void)max_outputs;

	size_t num_quads = 0;

	for(const char* p = p_input->s_text; *p; p++) {
		const uint32_t codepoint = (uint32_t)*p;

		// Check for a character we can render
		if (codepoint >= CX_FONT_NUM_GLYPHS || *p == ' ' || *p == '\t' || *p == '\n') {
			continue;
		}

		// Check if we have a glyph for that character
		const struct cx_font_glyph* p_glyph = &p_input->style.p_font_render_data->p_font->glyphs_[codepoint];
		if (!p_glyph->codepoint_) {
			continue;
		}

		++num_quads;
	}

	float* p_vertices;
	uint16_t* p_indices;

	const size_t primitive_vertex_buffers_size = sizeof(*p_out_outputs->primitive.p_vertex_buffers);
	const size_t primitive_vertex_attributes_size = sizeof(*p_out_outputs->primitive.p_attributes) * 3;
	const size_t primitive_data_size = primitive_vertex_buffers_size + primitive_vertex_attributes_size;

	const size_t num_vertices = num_quads * 4;
	const size_t num_indices = num_quads * 6;

	const size_t vertex_size = sizeof(*p_vertices) * 9;
	const size_t vertices_size = num_vertices * vertex_size;
	const size_t indices_size = num_indices * sizeof(*p_indices);

	uint8_t* p_buffer = malloc(primitive_data_size + vertices_size + indices_size);

	p_vertices = (void*)(p_buffer + primitive_data_size);
	p_indices = (void*)(p_buffer + primitive_data_size + vertices_size);

	float* p_v = p_vertices;
	uint16_t* p_i = p_indices;

	uint16_t vertex_index = 0;

	const float s = p_input->style.scale;
	const float tx = p_input->position[0];
	const float ty = p_input->position[1];
	const float tz = p_input->position[2];
	float transform[16] = {
		 s,  0,  0, 0,
		 0,  s,  0, 0,
		 0,  0,  s, 0,
		tx, ty, tz, 1
	};

	int32_t pen_x = 0;
	int32_t pen_baseline = 0;

	const uint32_t space_width = (uint32_t)abs(p_input->style.p_font_render_data->p_font->space_adv_);
	const uint32_t line_height = p_input->style.p_font_render_data->p_font->line_height_;

	for(const char* p = p_input->s_text; *p; p++) {
		const uint32_t codepoint = (uint32_t)*p;

		if (codepoint >= CX_FONT_NUM_GLYPHS) {
			continue;
		}

		if (*p == ' ') {
			pen_x += (int32_t)space_width;
			continue;	
		}

		if (*p == '\t') {
			pen_x += (int32_t)space_width * CX_TAB_SPACE_COUNT;
			continue;
		}

		if (*p == '\n') {
			pen_x = 0;
			pen_baseline -= (int32_t)line_height;
			continue;
		}

		const struct cx_font_glyph* p_glyph = &p_input->style.p_font_render_data->p_font->glyphs_[codepoint];

		if (!p_glyph->codepoint_) {
			// missing glyph
			continue;
		}

		const size_t glyph_index = (size_t)(p_glyph - p_input->style.p_font_render_data->p_font->glyphs_);
		const struct cx_texture_atlas_entry* p_atlas_entry =
			&p_input->style.p_font_render_data->p_glyph_atlas_layout->p_entries[glyph_index];

		float x = (float)(pen_x + p_glyph->metrics_.off_x);
		float y = (float)(pen_baseline + p_glyph->metrics_.off_y);
		
		// topleft

		p_v[0] = x;
		p_v[1] = y + (float)p_glyph->metrics_.height;
		p_v[2] = 0;
		p_v[3] = 1;
		matrix_multiply_vec4(transform, p_v, p_v);

		p_v[3] = p_atlas_entry->u0;
		p_v[4] = p_atlas_entry->v1;

		p_v[5] = CX_COLOR_R(p_input->style.color);
		p_v[6] = CX_COLOR_G(p_input->style.color);
		p_v[7] = CX_COLOR_B(p_input->style.color);
		p_v[8] = CX_COLOR_A(p_input->style.color);

		// bottomleft

		p_v[ 9] = x;
		p_v[10] = y;
		p_v[11] = 0;
		p_v[12] = 1;
		matrix_multiply_vec4(transform, p_v + 9, p_v + 9);

		p_v[12] = p_atlas_entry->u0;
		p_v[13] = p_atlas_entry->v0;

		p_v[14] = CX_COLOR_R(p_input->style.color);
		p_v[15] = CX_COLOR_G(p_input->style.color);
		p_v[16] = CX_COLOR_B(p_input->style.color);
		p_v[17] = CX_COLOR_A(p_input->style.color);

		// top right

		p_v[18] = x + (float)p_glyph->metrics_.width;
		p_v[19] = y + (float)p_glyph->metrics_.height;
		p_v[20] = 0;
		p_v[21] = 1;
		matrix_multiply_vec4(transform, p_v + 18, p_v + 18);

		p_v[21] = p_atlas_entry->u1;
		p_v[22] = p_atlas_entry->v1;

		p_v[23] = CX_COLOR_R(p_input->style.color);
		p_v[24] = CX_COLOR_G(p_input->style.color);
		p_v[25] = CX_COLOR_B(p_input->style.color);
		p_v[26] = CX_COLOR_A(p_input->style.color);

		// bottom right

		p_v[27] = x + (float)p_glyph->metrics_.width;
		p_v[28] = y;
		p_v[29] = 0;
		p_v[30] = 1;
		matrix_multiply_vec4(transform, p_v + 27, p_v + 27);

		p_v[30] = p_atlas_entry->u1;
		p_v[31] = p_atlas_entry->v0;

		p_v[32] = CX_COLOR_R(p_input->style.color);
		p_v[33] = CX_COLOR_G(p_input->style.color);
		p_v[34] = CX_COLOR_B(p_input->style.color);
		p_v[35] = CX_COLOR_A(p_input->style.color);

		p_v += 36;

		pen_x += p_glyph->metrics_.adv_x;

		p_i[0] = vertex_index + 0;
		p_i[1] = vertex_index + 1;
		p_i[2] = vertex_index + 2;
		p_i[3] = vertex_index + 1;
		p_i[4] = vertex_index + 3;
		p_i[5] = vertex_index + 2;
		p_i += 6;
		vertex_index += 4;
	}

	struct mesh_primitive* p_prim = &p_out_outputs->primitive;

    *p_prim = (struct mesh_primitive) {
        .p_vertex_buffers   = (void*)p_buffer,
        .num_vertex_buffers = 1,
        .p_attributes       = (void*)(p_buffer + primitive_vertex_buffers_size),
        .num_attributes     = 3,
        .vertex_count       = num_vertices,
        .index_buffer = {
            .p_bytes = p_indices,
            .count   = num_indices,
            .type    = VERTEX_INDEX_TYPE_u16
        },
        .draw_mode  = MESH_PRIMITIVE_DRAW_MODE_triangles,
        .bounds_min = { 0, 0, 0 },
        .bounds_max = { 0, 0, 0 }
    };

    *p_prim->p_vertex_buffers = (struct vertex_buffer) {
        .p_bytes = p_vertices,
        .size    = vertices_size
    };

    p_prim->p_attributes[0] = (struct vertex_attribute) {
        .index               = 0,
        .vertex_buffer_index = 0,
        .layout = {
            .stride          = vertex_size,
            .component_count = 3,
            .component_type  = VERTEX_ATTRIBUTE_TYPE_f32
        }
    };

    p_prim->p_attributes[1] = (struct vertex_attribute) {
        .index               = 1,
        .vertex_buffer_index = 0,
        .layout = {
            .offset          = sizeof(float) * 3,
            .stride          = vertex_size,
            .component_count = 2,
            .component_type  = VERTEX_ATTRIBUTE_TYPE_f32
        }
    };

    p_prim->p_attributes[2] = (struct vertex_attribute) {
        .index               = 2,
        .vertex_buffer_index = 0,
        .layout = {
            .offset          = sizeof(float) * 5,
            .stride          = vertex_size,
            .component_count = 4,
            .component_type  = VERTEX_ATTRIBUTE_TYPE_f32
        }
    };

	*p_out_num_outputs = 1;
}

void cx_text_mesher_free(struct cx_text_mesher_output* p_text_meshes, size_t n) {
	for (size_t i = 0; i < n; ++i) {
		free(p_text_meshes[i].primitive.p_vertex_buffers);
		p_text_meshes[i] = (struct cx_text_mesher_output){0};
	}
}
