#include <stdlib.h>

#include "cx_gfx_mesh.h"
#include "cx_mesh_data.h"
#include "cx_stream_serialization.h"
#include "static_mesh.h"

void static_mesh_free(struct static_mesh* p_static_mesh) {
	for (size_t i = 0; i < p_static_mesh->num_primitives; ++i) {
		for (size_t j = 0; j < p_static_mesh->p_primitives[i].layout.num_vertex_buffers; ++j) {
			free(p_static_mesh->p_primitives[i].p_vertex_buffers[j].p_bytes);
		}

		free(p_static_mesh->p_primitives[i].p_vertex_buffers);
		free(p_static_mesh->p_primitives[i].index_buffer.p_bytes);
		free(p_static_mesh->p_primitives[i].layout.p_attributes);
	}

	free(p_static_mesh->p_primitives);
	free(p_static_mesh->p_materials);

	static_mesh_unload_device_meshes(p_static_mesh);
	*p_static_mesh = (struct static_mesh){0};
}

void static_mesh_load_device_meshes(struct static_mesh* p_static_mesh) {
	p_static_mesh->p_gfx_meshes = malloc(sizeof(*p_static_mesh->p_gfx_meshes) * p_static_mesh->num_primitives);

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

int static_mesh_serialize(const struct static_mesh* p_static_mesh, struct cx_stream_writer* p_writer) {
	cx_stream_serialize_uint16(p_writer, p_static_mesh->num_primitives);

	for (uint16_t i = 0; i < p_static_mesh->num_primitives; ++i) {
		const struct cx_mesh_data* p_mesh_data = &p_static_mesh->p_primitives[i];

		cx_stream_serialize_uint16(p_writer, p_mesh_data->layout.num_vertex_buffers);
		cx_stream_serialize_uint16(p_writer, p_mesh_data->layout.num_attributes);
		cx_stream_serialize_uint8(p_writer, (uint8_t)p_mesh_data->layout.index_type);
		cx_stream_serialize_uint8(p_writer, (uint8_t)p_mesh_data->layout.draw_mode);

		for (uint16_t j = 0; j < p_mesh_data->layout.num_attributes; ++j) {
			const struct cx_mesh_vertex_attribute* p_attribute = &p_mesh_data->layout.p_attributes[j];

			cx_stream_serialize_uint16(p_writer, p_attribute->index);
			cx_stream_serialize_uint16(p_writer, p_attribute->vertex_buffer_index);
			cx_stream_serialize_uint8(p_writer, (uint8_t)p_attribute->format.type);
			cx_stream_serialize_uint16(p_writer, p_attribute->format.count);
			cx_stream_serialize_uint64(p_writer, p_attribute->layout.offset);
			cx_stream_serialize_uint64(p_writer, p_attribute->layout.stride);
		}

		cx_stream_serialize_uint32(p_writer, p_mesh_data->vertex_count);

		cx_stream_serialize_bytes(p_writer, sizeof(p_mesh_data->bounds_min), p_mesh_data->bounds_min);
		cx_stream_serialize_bytes(p_writer, sizeof(p_mesh_data->bounds_max), p_mesh_data->bounds_max);

		cx_asset_serialize_handle(p_writer, p_static_mesh->p_materials[i]);

		for (uint16_t j = 0; j < p_mesh_data->layout.num_vertex_buffers; ++j) {
			const struct cx_mesh_vertex_buffer* p_vertex_buffer = &p_mesh_data->p_vertex_buffers[j];

			cx_stream_serialize_uint64(p_writer, p_vertex_buffer->size);
			cx_stream_serialize_bytes(p_writer, p_vertex_buffer->size, p_vertex_buffer->p_bytes);
		}
	}

	return CX_TRUE;
}

int static_mesh_deserialize(struct static_mesh* p_static_mesh, struct cx_stream_reader* p_reader) {
	cx_stream_deserialize_uint16(p_reader, &p_static_mesh->num_primitives);

	// todo: alloc primitives

	for (uint16_t i = 0; i < p_static_mesh->num_primitives; ++i) {
		struct cx_mesh_data* p_mesh_data = &p_static_mesh->p_primitives[i];

		cx_stream_deserialize_uint16(p_reader, &p_mesh_data->layout.num_vertex_buffers);
		cx_stream_deserialize_uint16(p_reader, &p_mesh_data->layout.num_attributes);

		// todo: alloc vertex buffers, attributes

		uint8_t temp;

		cx_stream_deserialize_uint8(p_reader, &temp);
		p_mesh_data->layout.index_type = temp;

		cx_stream_deserialize_uint8(p_reader, &temp);
		p_mesh_data->layout.draw_mode = temp;

		for (uint16_t j = 0; j < p_mesh_data->layout.num_attributes; ++j) {
			struct cx_mesh_vertex_attribute* p_attribute = &p_mesh_data->layout.p_attributes[j];

			cx_stream_deserialize_uint16(p_reader, &p_attribute->index);
			cx_stream_deserialize_uint16(p_reader, &p_attribute->vertex_buffer_index);

			cx_stream_deserialize_uint8(p_reader, &temp);
			p_attribute->format.type = temp;

			cx_stream_deserialize_uint16(p_reader, &p_attribute->format.count);

			cx_stream_deserialize_uint64(p_reader, &p_attribute->layout.offset);
			cx_stream_deserialize_uint64(p_reader, &p_attribute->layout.stride);
		}

		cx_stream_deserialize_uint32(p_reader, &p_mesh_data->vertex_count);

		cx_stream_deserialize_bytes(p_reader, sizeof(p_mesh_data->bounds_min), p_mesh_data->bounds_min);
		cx_stream_deserialize_bytes(p_reader, sizeof(p_mesh_data->bounds_max), p_mesh_data->bounds_max);

		cx_asset_deserialize_handle(p_reader, &p_static_mesh->p_materials[i]);

		for (uint16_t j = 0; j < p_mesh_data->layout.num_vertex_buffers; ++j) {
			struct cx_mesh_vertex_buffer* p_vertex_buffer = &p_mesh_data->p_vertex_buffers[j];

			cx_stream_deserialize_uint64(p_reader, &p_vertex_buffer->size);

			// todo: alloc
			cx_stream_deserialize_bytes(p_reader, p_vertex_buffer->size, p_vertex_buffer->p_bytes);
		}
	}

	return CX_TRUE;
}
