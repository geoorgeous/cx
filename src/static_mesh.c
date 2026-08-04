#include <stdlib.h>

#include "cx_alloc.h"
#include "cx_gfx_mesh.h"
#include "cx_mesh_data.h"
#include "cx_stream_serialization.h"
#include "static_mesh.h"

void static_mesh_free(struct static_mesh* p_static_mesh) {
	for (size_t i = 0; i < p_static_mesh->num_primitives; ++i) {
		for (size_t j = 0; j < p_static_mesh->p_primitives[i].layout.num_vertex_buffers; ++j) {
			CX_FREE(p_static_mesh->p_primitives[i].p_vertex_buffers[j].p_bytes);
		}

		CX_FREE(p_static_mesh->p_primitives[i].p_vertex_buffers);
		free(p_static_mesh->p_primitives[i].index_buffer.p_bytes);
		CX_FREE(p_static_mesh->p_primitives[i].layout.p_attributes);
	}

	CX_FREE(p_static_mesh->p_primitives);
	CX_FREE(p_static_mesh->p_primitives_material_asset_refs);

	static_mesh_unload_device_meshes(p_static_mesh);
	*p_static_mesh = (struct static_mesh){0};
}

void static_mesh_load_device_meshes(struct static_mesh* p_static_mesh) {
	p_static_mesh->p_gfx_meshes = CX_MALLOC(sizeof(*p_static_mesh->p_gfx_meshes) * p_static_mesh->num_primitives);

	for (size_t i = 0; i < p_static_mesh->num_primitives; ++i) {
		const struct cx_mesh_data* p_primitive = &p_static_mesh->p_primitives[i];
		cx_gfx_mesh_create(p_primitive, CX_GFX_BUFFER_USAGE_static, &p_static_mesh->p_gfx_meshes[i]);
	}

	p_static_mesh->b_loaded_device_meshes = 1;
}

void static_mesh_unload_device_meshes(struct static_mesh* p_static_mesh) {
	for (size_t i = 0; i < p_static_mesh->num_primitives; ++i) {
		cx_gfx_mesh_destroy(&p_static_mesh->p_gfx_meshes[i]);
	}
	free(p_static_mesh->p_gfx_meshes);
	p_static_mesh->b_loaded_device_meshes = 0;
}

int static_mesh_serialize(const struct static_mesh* p_static_mesh, struct cx_stream* p_stream) {
	cx_stream_serialize_uint16(p_stream, p_static_mesh->num_primitives);

	for (uint16_t i = 0; i < p_static_mesh->num_primitives; ++i) {
		const struct cx_mesh_data* p_mesh_data = &p_static_mesh->p_primitives[i];

		// Material asset ref

		cx_asset_ref_serialize(&p_static_mesh->p_primitives_material_asset_refs[i], p_stream);

		// Layout metrics

		cx_stream_serialize_uint16(p_stream, p_mesh_data->layout.num_vertex_buffers);
		cx_stream_serialize_uint16(p_stream, p_mesh_data->layout.num_attributes);
		cx_stream_serialize_uint8(p_stream, (uint8_t)p_mesh_data->layout.index_type);
		cx_stream_serialize_uint8(p_stream, (uint8_t)p_mesh_data->layout.draw_mode);

		// Layout attributes

		for (uint16_t j = 0; j < p_mesh_data->layout.num_attributes; ++j) {
			const struct cx_mesh_vertex_attribute* p_attribute = &p_mesh_data->layout.p_attributes[j];

			cx_stream_serialize_uint16(p_stream, p_attribute->index);
			cx_stream_serialize_uint16(p_stream, p_attribute->vertex_buffer_index);
			cx_stream_serialize_uint8(p_stream, (uint8_t)p_attribute->format.type);
			cx_stream_serialize_uint16(p_stream, p_attribute->format.count);
			cx_stream_serialize_uint64(p_stream, p_attribute->layout.offset);
			cx_stream_serialize_uint64(p_stream, p_attribute->layout.stride);
		}


		// Vertex metrics

		cx_stream_serialize_uint32(p_stream, p_mesh_data->vertex_count);

		cx_stream_serialize_bytes(p_stream, sizeof(p_mesh_data->bounds_min), p_mesh_data->bounds_min);
		cx_stream_serialize_bytes(p_stream, sizeof(p_mesh_data->bounds_max), p_mesh_data->bounds_max);

		// Vertex buffers

		for (uint16_t j = 0; j < p_mesh_data->layout.num_vertex_buffers; ++j) {
			const struct cx_mesh_vertex_buffer* p_vertex_buffer = &p_mesh_data->p_vertex_buffers[j];

			cx_stream_serialize_uint64(p_stream, p_vertex_buffer->size);
			cx_stream_serialize_bytes(p_stream, p_vertex_buffer->size, p_vertex_buffer->p_bytes);
		}

		// Index buffer

		cx_stream_serialize_uint32(p_stream, p_mesh_data->index_buffer.count);

		if (p_mesh_data->index_buffer.count > 0) {
			cx_stream_serialize_bytes(
				p_stream,
				cx_mesh_vertex_index_type_size(p_mesh_data->layout.index_type) * p_mesh_data->index_buffer.count,
				p_mesh_data->index_buffer.p_bytes);
		}
	}

	return CX_TRUE;
}

int static_mesh_deserialize(struct cx_stream* p_stream, struct static_mesh* p_out_static_mesh) {
	cx_stream_deserialize_uint16(p_stream, &p_out_static_mesh->num_primitives);

	p_out_static_mesh->p_primitives =
		CX_MALLOC(sizeof(*p_out_static_mesh->p_primitives) * p_out_static_mesh->num_primitives);
	p_out_static_mesh->p_primitives_material_asset_refs =
		CX_MALLOC(sizeof(*p_out_static_mesh->p_primitives_material_asset_refs) * p_out_static_mesh->num_primitives);

	for (uint16_t i = 0; i < p_out_static_mesh->num_primitives; ++i) {
		struct cx_mesh_data* p_mesh_data = &p_out_static_mesh->p_primitives[i];

		// Material asset ref

		cx_asset_ref_deserialize(p_stream, &p_out_static_mesh->p_primitives_material_asset_refs[i]);

		// Layout metrics

		cx_stream_deserialize_uint16(p_stream, &p_mesh_data->layout.num_vertex_buffers);
		cx_stream_deserialize_uint16(p_stream, &p_mesh_data->layout.num_attributes);

		uint8_t temp;

		cx_stream_deserialize_uint8(p_stream, &temp);
		p_mesh_data->layout.index_type = temp;

		cx_stream_deserialize_uint8(p_stream, &temp);
		p_mesh_data->layout.draw_mode = temp;

		// Layout attributes

		p_mesh_data->layout.p_attributes =
			CX_MALLOC(sizeof(*p_mesh_data->layout.p_attributes) * p_mesh_data->layout.num_attributes);

		for (uint16_t j = 0; j < p_mesh_data->layout.num_attributes; ++j) {
			struct cx_mesh_vertex_attribute* p_attribute = &p_mesh_data->layout.p_attributes[j];

			cx_stream_deserialize_uint16(p_stream, &p_attribute->index);
			cx_stream_deserialize_uint16(p_stream, &p_attribute->vertex_buffer_index);

			cx_stream_deserialize_uint8(p_stream, &temp);
			p_attribute->format.type = temp;

			cx_stream_deserialize_uint16(p_stream, &p_attribute->format.count);

			cx_stream_deserialize_uint64(p_stream, &p_attribute->layout.offset);
			cx_stream_deserialize_uint64(p_stream, &p_attribute->layout.stride);
		}

		// Vertex metrics

		cx_stream_deserialize_uint32(p_stream, &p_mesh_data->vertex_count);

		cx_stream_deserialize_bytes(p_stream, sizeof(p_mesh_data->bounds_min), p_mesh_data->bounds_min);
		cx_stream_deserialize_bytes(p_stream, sizeof(p_mesh_data->bounds_max), p_mesh_data->bounds_max);

		// Vertex buffers

		p_mesh_data->p_vertex_buffers =
			CX_MALLOC(sizeof(*p_mesh_data->p_vertex_buffers) * p_mesh_data->layout.num_vertex_buffers);

		for (uint16_t j = 0; j < p_mesh_data->layout.num_vertex_buffers; ++j) {
			struct cx_mesh_vertex_buffer* p_vertex_buffer = &p_mesh_data->p_vertex_buffers[j];

			cx_stream_deserialize_uint64(p_stream, &p_vertex_buffer->size);

			p_vertex_buffer->p_bytes = CX_MALLOC(p_vertex_buffer->size);
			cx_stream_deserialize_bytes(p_stream, p_vertex_buffer->size, p_vertex_buffer->p_bytes);
		}

		// Index buffer
		
		cx_stream_deserialize_uint32(p_stream, &p_mesh_data->index_buffer.count);

		if (p_mesh_data->index_buffer.count > 0) {
			const size_t index_buffer_size =
				cx_mesh_vertex_index_type_size(p_mesh_data->layout.index_type) * p_mesh_data->index_buffer.count;

			p_mesh_data->index_buffer.p_bytes = CX_MALLOC(index_buffer_size);

			cx_stream_deserialize_bytes(p_stream, index_buffer_size, p_mesh_data->index_buffer.p_bytes);
		}
	}

	return CX_TRUE;
}
