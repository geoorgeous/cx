#include <string.h>

#include "cx_alloc.h"
#include "cx_array.h"
#include "cx_blueprint.h"
#include "cx_cmp_static_mesh.h"
#include "cx_ed_asset_library.h"
#include "cx_ed_import_gltf.h"
#include "cx_ed_import_image.h"
#include "cx_image.h"
#include "cx_mesh_data.h"
#include "cx_texture_sampler_settings.h"
#include "gltf.h"
#include "cx_logging.h"
#include "material.h"
#include "matrix.h"
#include "skeleton.h"
#include "skeletal_animation.h"
#include "static_mesh.h"
#include "cx_texture.h"
#include "transform_animation.h"

struct cx_ed_import_gltf_context {
	const struct gltf* p_gltf;
	struct cx_array image_assets;
	struct cx_array texture_assets;
	struct cx_array material_assets;
	struct cx_array static_mesh_assets;
	struct cx_asset_ref asset_ref;
};

static void cx_ed_import_gltf_image(struct cx_ed_import_gltf_context* p_context, size_t gltf_image_index);
static void cx_ed_import_gltf_texture(struct cx_ed_import_gltf_context* p_context, size_t gltf_texture_index);
static void cx_ed_import_gltf_material(struct cx_ed_import_gltf_context* p_context, size_t gltf_material_index);
static void cx_ed_import_gltf_mesh(struct cx_ed_import_gltf_context* p_context, size_t gltf_mesh_index);
static void cx_ed_import_gltf_mesh_primitive(
	struct cx_ed_import_gltf_context* p_context,
	struct static_mesh* p_mesh,
	size_t gltf_mesh_index,
	size_t mesh_primitive_index);
static void cx_ed_import_gltf_skin(struct cx_ed_import_gltf_context* p_context, size_t gltf_skin_index);
static void cx_ed_import_gltf_animation(struct cx_ed_import_gltf_context* p_context, size_t gltf_animation_index);
static void cx_ed_import_gltf_scene(struct cx_ed_import_gltf_context* p_context, size_t gltf_scene_index);

static const struct gltf_accessor* cx_ed_import_gltf_get_mesh_primitive_vertex_attribute_accessor(
	const struct gltf* p_gltf,
	const struct gltf_mesh_primitive* p_gltf_mesh_primitive,
	enum gltf_mesh_vertex_attribute type);

static void cx_ed_import_gltf_discover_joint_hierarchy(
	const struct gltf* p_gltf,
	const struct gltf_skin* p_gltf_skin,
	struct skeleton* p_skeleton,
	size_t joint_index);

static uint16_t cx_ed_import_gltf_process_scene_node(
	struct cx_ed_import_gltf_context* p_context,
	const struct gltf_node* p_gltf_node,
	struct cx_blueprint* p_blueprint);

static size_t cx_ed_import_gltf_compute_accessor_element_size(const struct gltf_accessor* p_gltf_accessor);

static void cx_ed_import_gltf_copy_accessor(
	const struct gltf* p_gltf,
	const struct gltf_accessor* p_gltf_accessor,
	void* p_dst,
	size_t dst_stride);

static void cx_ed_import_gtlf_copy_accessor_to_vertex_buffer(
	const struct gltf* p_gltf,
	const struct gltf_accessor* p_gltf_accessor,
	const struct cx_mesh_vertex_buffer* p_dst,
	const struct cx_mesh_vertex_attribute_layout* p_dst_layout);

static enum cx_texture_min_filter_mode gltf_enum_to_texture_filter_min(enum gltf_sampler_min_filter en);
static enum cx_texture_mag_filter_mode gltf_enum_to_texture_filter_mag(enum gltf_sampler_mag_filter en);
static enum cx_texture_address_mode gltf_enum_to_texture_address_mode(enum gltf_sampler_wrap en);
static enum cx_mesh_vertex_index_type gltf_enum_to_vertex_index_type(enum gltf_accessor_component_type type);
static enum cx_mesh_draw_mode gltf_enum_to_mesh_primitive_draw_mode(enum gltf_mesh_primitive_mode mode);

int cx_ed_import_gltf(const struct gltf* p_gltf, struct cx_asset_ref* p_out) {
	struct cx_ed_import_gltf_context context = {
		.p_gltf = p_gltf
	};

	cx_array_init(sizeof(struct cx_asset_ref), &context.image_assets);
	cx_array_init(sizeof(struct cx_asset_ref), &context.texture_assets);
	cx_array_init(sizeof(struct cx_asset_ref), &context.material_assets);
	cx_array_init(sizeof(struct cx_asset_ref), &context.static_mesh_assets);

	for (size_t i = 0; i < p_gltf->num_images; ++i) {
		cx_ed_import_gltf_image(&context, i);
	}

	for (size_t i = 0; i < p_gltf->num_textures; ++i) {
		cx_ed_import_gltf_texture(&context, i);
	}

	for (size_t i = 0; i < p_gltf->num_materials; ++i) {
		cx_ed_import_gltf_material(&context, i);
	}

	for (size_t i = 0; i < p_gltf->num_meshes; ++i) {
		cx_ed_import_gltf_mesh(&context, i);
	}

	//for (size_t i = 0; i < p_gltf->num_skins; ++i) {
	//	cx_ed_import_gltf_skin(&context, i);
	//}

	//// todo
	//for (size_t i = 0; i < p_gltf->num_animations; ++i) {
	//	cx_ed_import_gltf_animation(&context, i);
	//}

	for (size_t i = 0; i < p_gltf->num_scenes; ++i) {
		cx_ed_import_gltf_scene(&context, i);
	}

	*p_out = context.asset_ref;

	return CX_TRUE;
}

int cx_ed_import_gltf_file(const char* s_filepath, struct cx_asset_ref* p_out) {

	struct gltf gltf;
	if (gltf_load_from_file(s_filepath, &gltf) != GLTF_SUCCESS) {
		return 0;
	}

	const int result = cx_ed_import_gltf(&gltf, p_out);

	gltf_free(&gltf);

	return result;
}

void cx_ed_import_gltf_image(struct cx_ed_import_gltf_context* p_context, size_t gltf_image_index) {
	const struct gltf_image* p_gltf_image = &p_context->p_gltf->p_images[gltf_image_index];

	const void* p_bytes;
	size_t size;

	if (p_gltf_image->b_uri_source) {
		p_bytes = p_gltf_image->source.uri.p_data;
		size = p_gltf_image->source.uri.size;
	} else {
		const struct gltf_buffer_view* p_gltf_buffer_view =
			&p_context->p_gltf->p_buffer_views[p_gltf_image->source.buffer_view_index];
		const struct gltf_buffer* p_gltf_buffer = &p_context->p_gltf->p_buffers[p_gltf_buffer_view->buffer_index];
		p_bytes = (unsigned char*)p_gltf_buffer->p_bytes + p_gltf_buffer_view->byte_offset;
		size = p_gltf_buffer_view->byte_length;
	}

	struct cx_asset_ref image_asset_ref;
	cx_ed_import_image(p_bytes, size, &image_asset_ref);

	(void)cx_array_push(&p_context->image_assets, &image_asset_ref);
}

void cx_ed_import_gltf_texture(struct cx_ed_import_gltf_context* p_context, size_t gltf_texture_index) {
	const struct gltf_texture* p_gltf_texture = &p_context->p_gltf->p_textures[gltf_texture_index];

	struct cx_texture* p_texture = CX_MALLOC(cx_asset_type_size(CX_ASSET_TYPE_TEXTURE));

	const struct cx_asset_ref* p_source_image_asset_ref =
		cx_array_at(&p_context->image_assets, p_gltf_texture->source_image_index);

	*p_texture = (struct cx_texture){
		.source_image_asset_ref = *p_source_image_asset_ref,
		.sampler_settings = {
			.mag_filter_mode = gltf_enum_to_texture_filter_mag(p_gltf_texture->sampler_mag_filter),
			.min_filter_mode = gltf_enum_to_texture_filter_min(p_gltf_texture->sampler_min_filter),
			.address_mode_u = gltf_enum_to_texture_address_mode(p_gltf_texture->sampler_wrap_s),
			.address_mode_v = gltf_enum_to_texture_address_mode(p_gltf_texture->sampler_wrap_t)
		},
		.gfx_texture_format =
			((const struct cx_image*)cx_asset_ref_get(p_source_image_asset_ref))->pixel_data_format.pixel_format
	};

	struct cx_asset_ref texture_asset_ref;
	cx_ed_asset_library_new(CX_ASSET_TYPE_TEXTURE, p_texture, &texture_asset_ref);

	(void)cx_array_push(&p_context->texture_assets, &texture_asset_ref);
}

void cx_ed_import_gltf_material(struct cx_ed_import_gltf_context* p_context, size_t gltf_material_index) {
	const struct gltf_material* p_gltf_material = &p_context->p_gltf->p_materials[gltf_material_index];

	struct material* p_material = CX_MALLOC(cx_asset_type_size(CX_ASSET_TYPE_MATERIAL));

	const struct cx_asset_ref* p_material_texture_asset_ref = 
		cx_array_at(&p_context->texture_assets, p_gltf_material->pbr_base_color_texture.source_texture_index);

	*p_material = (struct material) {
		.texture_asset_ref = *p_material_texture_asset_ref,
		.color = { 1, 1, 1, 1 }
	};
	
	struct cx_asset_ref material_asset_ref;
	cx_ed_asset_library_new(CX_ASSET_TYPE_MATERIAL, p_material, &material_asset_ref);

	(void)cx_array_push(&p_context->material_assets, &material_asset_ref);
}

void cx_ed_import_gltf_mesh(struct cx_ed_import_gltf_context* p_context, size_t gltf_mesh_index) {
	const struct gltf_mesh* p_gltf_mesh = &p_context->p_gltf->p_meshes[gltf_mesh_index];

	struct static_mesh* p_static_mesh = CX_MALLOC(cx_asset_type_size(CX_ASSET_TYPE_STATIC_MESH));
	struct cx_mesh_data* p_static_mesh_primitives =
		CX_MALLOC(p_gltf_mesh->num_primitives * sizeof(*p_static_mesh->p_primitives));
	struct cx_asset_ref* p_static_mesh_primitive_materials =
		CX_MALLOC(p_gltf_mesh->num_primitives * sizeof(*p_static_mesh->p_primitives_material_asset_refs));

	*p_static_mesh = (struct static_mesh) {
		.num_primitives = (uint16_t)p_gltf_mesh->num_primitives,
		.p_primitives = p_static_mesh_primitives,
		.p_primitives_material_asset_refs = p_static_mesh_primitive_materials
	};
	
	for (size_t i = 0; i < p_gltf_mesh->num_primitives; ++i) {
		cx_ed_import_gltf_mesh_primitive(p_context, p_static_mesh, gltf_mesh_index, i);
	}

	struct cx_asset_ref static_mesh_asset_ref;
	cx_ed_asset_library_new(CX_ASSET_TYPE_STATIC_MESH, p_static_mesh, &static_mesh_asset_ref);

	(void)cx_array_push(&p_context->static_mesh_assets, &static_mesh_asset_ref);
}

void cx_ed_import_gltf_mesh_primitive(
	struct cx_ed_import_gltf_context* p_context,
	struct static_mesh* p_mesh,
	size_t gltf_mesh_index,
	size_t mesh_primitive_index) {

	const struct gltf_mesh_primitive* p_gltf_mesh_primitive = 
		&p_context->p_gltf->p_meshes[gltf_mesh_index].p_primitives[mesh_primitive_index];
	
	const int b_generate_normals = 1;
	
	const struct gltf_accessor* p_gltf_positions_accessor =
		cx_ed_import_gltf_get_mesh_primitive_vertex_attribute_accessor(
			p_context->p_gltf, p_gltf_mesh_primitive, GLTF_MESH_VERTEX_ATTRIBUTE_position);
	const struct gltf_accessor* p_gltf_normals_accessor =
		cx_ed_import_gltf_get_mesh_primitive_vertex_attribute_accessor(
			p_context->p_gltf, p_gltf_mesh_primitive, GLTF_MESH_VERTEX_ATTRIBUTE_normal);
	const struct gltf_accessor* p_gltf_tangents_accessor =
		cx_ed_import_gltf_get_mesh_primitive_vertex_attribute_accessor(
				p_context->p_gltf, p_gltf_mesh_primitive, GLTF_MESH_VERTEX_ATTRIBUTE_tangent);
	const struct gltf_accessor* p_gltf_texcoords_0_accessor =
		cx_ed_import_gltf_get_mesh_primitive_vertex_attribute_accessor(
			p_context->p_gltf, p_gltf_mesh_primitive, GLTF_MESH_VERTEX_ATTRIBUTE_texcoord_0);
	const struct gltf_accessor* p_gltf_texcoords_1_accessor =
		cx_ed_import_gltf_get_mesh_primitive_vertex_attribute_accessor(
			p_context->p_gltf, p_gltf_mesh_primitive, GLTF_MESH_VERTEX_ATTRIBUTE_texcoord_1);
	const struct gltf_accessor* p_gltf_colors_accessor =
		cx_ed_import_gltf_get_mesh_primitive_vertex_attribute_accessor(
			p_context->p_gltf, p_gltf_mesh_primitive, GLTF_MESH_VERTEX_ATTRIBUTE_color);
	const struct gltf_accessor* p_gltf_joints_accessor =
		cx_ed_import_gltf_get_mesh_primitive_vertex_attribute_accessor(
			p_context->p_gltf, p_gltf_mesh_primitive, GLTF_MESH_VERTEX_ATTRIBUTE_joints);
	const struct gltf_accessor* p_gltf_weights_accessor =
		cx_ed_import_gltf_get_mesh_primitive_vertex_attribute_accessor(
			p_context->p_gltf, p_gltf_mesh_primitive, GLTF_MESH_VERTEX_ATTRIBUTE_weights);
	
	const struct gltf_accessor* p_attribute_gltf_accessors[8] = {0};
	struct cx_mesh_vertex_attribute attributes[8] = {0};
	uint16_t num_attributes = 0;

	size_t offset = 0;

	if (!!p_gltf_positions_accessor) {
		p_attribute_gltf_accessors[num_attributes] = p_gltf_positions_accessor;
		attributes[num_attributes].index = 0;
		attributes[num_attributes].vertex_buffer_index = 0;
		attributes[num_attributes].layout.offset = offset;
		attributes[num_attributes].format.type = CX_MESH_VERTEX_ATTRIBUTE_TYPE_f32;
		attributes[num_attributes].format.count = 3;
		offset += cx_mesh_vertex_attribute_format_size(&attributes[num_attributes].format);
		++num_attributes;
	} else {
		CX_LOG(ERROR, IMPORT_GLTF, "Mesh import failed: missing position data\n");
		return;
	}

	if (!!p_gltf_normals_accessor || b_generate_normals) {
		p_attribute_gltf_accessors[num_attributes] = p_gltf_normals_accessor;
		attributes[num_attributes].index = 1;
		attributes[num_attributes].vertex_buffer_index = 0;
		attributes[num_attributes].layout.offset = offset;
		attributes[num_attributes].format.type = CX_MESH_VERTEX_ATTRIBUTE_TYPE_f32;
		attributes[num_attributes].format.count = 3;
		offset += cx_mesh_vertex_attribute_format_size(&attributes[num_attributes].format);
		++num_attributes;

		if (!!p_gltf_tangents_accessor) {
			p_attribute_gltf_accessors[num_attributes] = p_gltf_tangents_accessor;
			attributes[num_attributes].index = 2;
			attributes[num_attributes].vertex_buffer_index = 0;
			attributes[num_attributes].layout.offset = offset;
			attributes[num_attributes].format.type = CX_MESH_VERTEX_ATTRIBUTE_TYPE_f32;
			attributes[num_attributes].format.count = 3;
			offset += cx_mesh_vertex_attribute_format_size(&attributes[num_attributes].format);
			++num_attributes;
		}
	}

	if (!!p_gltf_texcoords_0_accessor) {
		p_attribute_gltf_accessors[num_attributes] = p_gltf_texcoords_0_accessor;
		attributes[num_attributes].index = 3;
		attributes[num_attributes].vertex_buffer_index = 0;
		attributes[num_attributes].layout.offset = offset;
		attributes[num_attributes].format.type = CX_MESH_VERTEX_ATTRIBUTE_TYPE_f32;
		attributes[num_attributes].format.count = 3;
		offset += cx_mesh_vertex_attribute_format_size(&attributes[num_attributes].format);
		++num_attributes;
	}

	if (!!p_gltf_texcoords_1_accessor) {
		p_attribute_gltf_accessors[num_attributes] = p_gltf_texcoords_1_accessor;
		attributes[num_attributes].index = 4;
		attributes[num_attributes].vertex_buffer_index = 0;
		attributes[num_attributes].layout.offset = offset;
		attributes[num_attributes].format.type = CX_MESH_VERTEX_ATTRIBUTE_TYPE_f32;
		attributes[num_attributes].format.count = 3;
		offset += cx_mesh_vertex_attribute_format_size(&attributes[num_attributes].format);
		++num_attributes;
	}

	if (!!p_gltf_colors_accessor) {
		p_attribute_gltf_accessors[num_attributes] = p_gltf_colors_accessor;
		attributes[num_attributes].index = 5;
		attributes[num_attributes].vertex_buffer_index = 0;
		attributes[num_attributes].layout.offset = offset;
		attributes[num_attributes].format.type = CX_MESH_VERTEX_ATTRIBUTE_TYPE_f32;
		attributes[num_attributes].format.count = 4;
		offset += cx_mesh_vertex_attribute_format_size(&attributes[num_attributes].format);
		++num_attributes;
	}

	if (!!p_gltf_joints_accessor && !!p_gltf_weights_accessor) {
		p_attribute_gltf_accessors[num_attributes] = p_gltf_joints_accessor;
		attributes[num_attributes].index = 6;
		attributes[num_attributes].vertex_buffer_index = 0;
		attributes[num_attributes].layout.offset = offset;
		attributes[num_attributes].format.type = CX_MESH_VERTEX_ATTRIBUTE_TYPE_f32;
		attributes[num_attributes].format.count = 4;
		offset += cx_mesh_vertex_attribute_format_size(&attributes[num_attributes].format);
		++num_attributes;

		p_attribute_gltf_accessors[num_attributes] = p_gltf_weights_accessor;
		attributes[num_attributes].index = 7;
		attributes[num_attributes].vertex_buffer_index = 0;
		attributes[num_attributes].layout.offset = offset;
		attributes[num_attributes].format.type = CX_MESH_VERTEX_ATTRIBUTE_TYPE_f32;
		attributes[num_attributes].format.count = 4;
		offset += cx_mesh_vertex_attribute_format_size(&attributes[num_attributes].format);
		++num_attributes;
	} else if (!!p_gltf_joints_accessor || !!p_gltf_weights_accessor) {
		CX_LOG(ERROR, IMPORT_GLTF,
			"Mesh joints data or weights data present,"
			" but missing the other. Skipping import of joints/weights data\n");
	}

	for (size_t i = 0; i < num_attributes; ++i) {
		attributes[i].layout.stride = offset;
	}
	
	struct cx_mesh_data* p_mesh_data = &p_mesh->p_primitives[mesh_primitive_index];
	*p_mesh_data = (struct cx_mesh_data) {0};

	p_mesh_data->vertex_count = p_gltf_positions_accessor->count;

	// Allocate vertex buffer storage

	struct cx_mesh_vertex_buffer buffers[8] = {0};
	uint16_t num_buffers = 0;

	for (size_t i = 0; i < num_attributes; ++i) {
		if (num_buffers <= attributes[i].vertex_buffer_index) {
			num_buffers = attributes[i].vertex_buffer_index + 1;
		}

		const size_t attribute_size = cx_mesh_vertex_attribute_format_size(&attributes[i].format);
		buffers[attributes[i].vertex_buffer_index].size += attribute_size * p_mesh_data->vertex_count;
	}

	for (size_t i = 0; i < num_buffers; ++i) {
		buffers[i].p_bytes = CX_MALLOC(buffers[i].size);
	}

	// Copy over vertex buffer data
	
	for (size_t i = 0; i < num_attributes; ++i) {
		if (p_attribute_gltf_accessors[i]) {
			cx_ed_import_gtlf_copy_accessor_to_vertex_buffer(
				p_context->p_gltf,
				p_attribute_gltf_accessors[i],
				&buffers[attributes[i].vertex_buffer_index],
				&attributes[i].layout);
		}
	}

	// Vertex index buffer

	if (p_gltf_mesh_primitive->vertex_indices_accessor_index != GLTF_INVALID_INDEX) {
		const struct gltf_accessor* p_gltf_indices_accessor =
			&p_context->p_gltf->p_accessors[p_gltf_mesh_primitive->vertex_indices_accessor_index];

		const size_t element_size = cx_ed_import_gltf_compute_accessor_element_size(p_gltf_indices_accessor);
		
		p_mesh_data->layout.index_type = gltf_enum_to_vertex_index_type(p_gltf_indices_accessor->component_type);
		p_mesh_data->index_buffer.count = p_gltf_indices_accessor->count;
		p_mesh_data->index_buffer.p_bytes = CX_MALLOC(p_mesh_data->index_buffer.count * element_size);

		cx_ed_import_gltf_copy_accessor(
			p_context->p_gltf,
			p_gltf_indices_accessor,
			p_mesh_data->index_buffer.p_bytes,
			0);
	}

	if (p_gltf_mesh_primitive->material_index != GLTF_INVALID_INDEX) {
		const struct cx_asset_ref* p_material_asset_ref = 
			cx_array_at(&p_context->material_assets, p_gltf_mesh_primitive->material_index);
		p_mesh->p_primitives_material_asset_refs[mesh_primitive_index] = *p_material_asset_ref;
	}

	p_mesh_data->layout.draw_mode = gltf_enum_to_mesh_primitive_draw_mode(p_gltf_mesh_primitive->mode);

	p_mesh_data->layout.num_vertex_buffers = num_buffers;
	const size_t vertex_buffers_size =
		p_mesh_data->layout.num_vertex_buffers * sizeof(*p_mesh_data->p_vertex_buffers);
	p_mesh_data->p_vertex_buffers = CX_MALLOC(vertex_buffers_size);
	memcpy(p_mesh_data->p_vertex_buffers, buffers, vertex_buffers_size);

	p_mesh_data->layout.num_attributes = num_attributes;
	const size_t attributes_size =
		p_mesh_data->layout.num_attributes * sizeof(*p_mesh_data->layout.p_attributes);
	p_mesh_data->layout.p_attributes = CX_MALLOC(attributes_size);
	memcpy(p_mesh_data->layout.p_attributes, attributes, attributes_size);
	
	// Post-processing

	if (!p_gltf_normals_accessor && b_generate_normals) {
		cx_mesh_data_generate_normals(p_mesh_data, 0, 1);
	}

	memcpy(p_mesh_data->bounds_min, p_gltf_positions_accessor->min, sizeof(p_mesh_data->bounds_min));
	memcpy(p_mesh_data->bounds_max, p_gltf_positions_accessor->max, sizeof(p_mesh_data->bounds_max));
}

void cx_ed_import_gltf_skin(struct cx_ed_import_gltf_context* p_context, size_t gltf_skin_index) {
	const struct gltf_skin* p_gltf_skin = &p_context->p_gltf->p_skins[gltf_skin_index];
	
	struct skeleton* p_skeleton = CX_MALLOC(sizeof(struct skeleton));

	*p_skeleton = (struct skeleton) {
		.num_joints = p_gltf_skin->num_joints,
		.p_joints = CX_CALLOC(p_gltf_skin->num_joints * sizeof(*p_skeleton->p_joints)),
		.p_inverse_bind_matrices = CX_CALLOC(p_gltf_skin->num_joints * sizeof(float) * 16)
	};
	
	if (p_gltf_skin->inverse_bind_matrices_accessor_index == GLTF_INVALID_INDEX) {
		for (size_t i = 0; i < p_skeleton->num_joints; ++i) {
			matrix_make_identity(&p_skeleton->p_inverse_bind_matrices[i * 16]);
		}
	} else {
		const struct gltf_accessor* p_gltf_inverse_bind_matrices_accessor =
			&p_context->p_gltf->p_accessors[p_gltf_skin->inverse_bind_matrices_accessor_index];
		cx_ed_import_gltf_copy_accessor(
			p_context->p_gltf,
			p_gltf_inverse_bind_matrices_accessor,
			p_skeleton->p_inverse_bind_matrices,
			0);
	}

	for (size_t i = 0; i < p_skeleton->num_joints; ++i) {
		const struct gltf_node* p_gltf_joint_node = &p_context->p_gltf->p_nodes[p_gltf_skin->p_joints_indices[i]];
		struct skeleton_joint* p_joint = &p_skeleton->p_joints[i];

		p_joint->num_children = SIZE_MAX;
		p_joint->parent_index = SIZE_MAX;
		memcpy(p_joint->transform, p_gltf_joint_node->matrix, sizeof(p_gltf_joint_node->matrix));
	}

	cx_ed_import_gltf_discover_joint_hierarchy(p_context->p_gltf, p_gltf_skin, p_skeleton, 0);
	
	for (size_t i = 0; i < p_skeleton->num_joints; ++i) {
		struct skeleton_joint* p_joint = &p_skeleton->p_joints[i];
		if (p_joint->parent_index == SIZE_MAX) {
			++p_skeleton->num_root_joints;
			p_skeleton->p_root_joints_indices = CX_REALLOC(
				p_skeleton->p_root_joints_indices,
				p_skeleton->num_root_joints * sizeof(*p_skeleton->p_root_joints_indices));
			p_skeleton->p_root_joints_indices[p_skeleton->num_root_joints - 1] = i;
		}
	}

	// todo: create skeleton asset
}

void cx_ed_import_gltf_animation(struct cx_ed_import_gltf_context* p_context, size_t gltf_animation_index) {
	const struct gltf_animation* p_gltf_animation = &p_context->p_gltf->p_animations[gltf_animation_index];

	const struct gltf_animation_channel* p_gltf_channel = &p_gltf_animation->p_channels[0];
	const struct gltf_node* p_gltf_node = &p_context->p_gltf->p_nodes[p_gltf_channel->target_node_index];
	const int b_is_skeletal = p_gltf_node->b_is_joint;
	
	struct skeletal_animation skeletal_animation;

	for (size_t i = 0; i < p_gltf_animation->num_channels; ++i) {
		p_gltf_channel = &p_gltf_animation->p_channels[i];        
		const struct gltf_animation_sampler* p_gltf_sampler =
			&p_gltf_animation->p_samplers[p_gltf_channel->source_sampler_index];
		const struct gltf_accessor* p_gltf_sampler_input_accessor =
			&p_context->p_gltf->p_accessors[p_gltf_sampler->input_accessor_index];
		const struct gltf_accessor* p_gltf_sampler_output_accessor =
			&p_context->p_gltf->p_accessors[p_gltf_sampler->output_accessor_index];
		
		struct transform_animation transform_animation = {0};

		switch (p_gltf_channel->target_path) {
			case GLTF_ANIMATION_CHANNEL_TARGET_PATH_translation: {
				transform_animation.target = TRANSFORM_ANIMATION_TARGET_translation;
				break;
			}
			case GLTF_ANIMATION_CHANNEL_TARGET_PATH_rotation: {
				transform_animation.target = TRANSFORM_ANIMATION_TARGET_rotation;
				break;
			}
			case GLTF_ANIMATION_CHANNEL_TARGET_PATH_scale: {
				transform_animation.target = TRANSFORM_ANIMATION_TARGET_scale;
				break;
			}
		}

		transform_animation.sampler.num_keyframes = p_gltf_sampler_input_accessor->count;
		
		switch (p_gltf_sampler->interpolation) {
			case GLTF_ANIMATION_SAMPLER_INTERPOLATION_linear: {
				transform_animation.sampler.interpolation_mode = TRANSFORM_ANIMATION_INTERPOLATION_MODE_linear;
				break;
			}

			case GLTF_ANIMATION_SAMPLER_INTERPOLATION_step: {
				transform_animation.sampler.interpolation_mode = TRANSFORM_ANIMATION_INTERPOLATION_MODE_step;
				break;
			}

			case GLTF_ANIMATION_SAMPLER_INTERPOLATION_cubic_splines: {
				transform_animation.sampler.interpolation_mode = TRANSFORM_ANIMATION_INTERPOLATION_MODE_cubic_spline;
				break;
			}
		}

		transform_animation.sampler.p_keyframe_timestamps =
			CX_MALLOC(
				cx_ed_import_gltf_compute_accessor_element_size(p_gltf_sampler_input_accessor)
				* transform_animation.sampler.num_keyframes);

		cx_ed_import_gltf_copy_accessor(
			p_context->p_gltf,
			p_gltf_sampler_input_accessor,
			transform_animation.sampler.p_keyframe_timestamps,
			0);

		transform_animation.sampler.p_keyframe_values =
			CX_MALLOC(
				cx_ed_import_gltf_compute_accessor_element_size(p_gltf_sampler_output_accessor)
				* transform_animation.sampler.num_keyframes);

		cx_ed_import_gltf_copy_accessor(
			p_context->p_gltf,
			p_gltf_sampler_output_accessor,
			transform_animation.sampler.p_keyframe_values,
			0);

		// todo:
		(void)b_is_skeletal;
		(void)skeletal_animation;
		
		// if (b_is_skeletal) {
		//     const struct skeleton* p_skeleton = 0;
		//     size_t skeleton_target_joint_index;

		//     for (size_t j = 0; j < p_gltf->num_skins && p_skeleton == 0; ++j) {
		//         const struct gltf_skin* p_gltf_skin = &p_gltf->p_skins[j];
		//         for (size_t k = 0; k < p_gltf_skin->num_joints; ++k) {
		//             const size_t gltf_skin_joint_index = p_gltf_skin->p_joints_indices[k];
		//             if (gltf_skin_joint_index == p_gltf_channel->target_node_index) {
		//                 p_skeleton = &p_result->p_skeletons[j];
		//                 skeleton_target_joint_index = k;
		//                 break;
		//             }
		//         }
		//     }

		//     // skeletal_animation.p_joint_transform_animations[skeletal_animation.num_joint_transform_animations] = 
		//     //	transform_animation;
		//     // skeletal_animation.p_target_joint_indices[skeletal_animation.num_joint_transform_animations] =
		//     //	skeleton_target_joint_index;
		//     // ++skeletal_animation.num_joint_transform_animations;
		// } else {
		//     // it's just a normal transform animation I suppose...
		// }
	}
}

void cx_ed_import_gltf_scene(struct cx_ed_import_gltf_context* p_context, size_t gltf_scene_index) {
	const struct gltf_scene* p_gltf_scene = &p_context->p_gltf->p_scenes[gltf_scene_index];
	
	struct cx_blueprint* p_blueprint = CX_MALLOC(cx_asset_type_size(CX_ASSET_TYPE_BLUEPRINT));
	*p_blueprint = (struct cx_blueprint) {0};

	for (size_t i = 0; i < p_gltf_scene->num_root_nodes; ++i) {
		const struct gltf_node* p_gltf_root_node = &p_context->p_gltf->p_nodes[p_gltf_scene->p_root_nodes_indices[i]];
		cx_ed_import_gltf_process_scene_node(p_context, p_gltf_root_node, p_blueprint);
	}

	cx_ed_asset_library_new(CX_ASSET_TYPE_BLUEPRINT, p_blueprint, &p_context->asset_ref);
}

const struct gltf_accessor* cx_ed_import_gltf_get_mesh_primitive_vertex_attribute_accessor(
	const struct gltf* p_gltf,
	const struct gltf_mesh_primitive* p_gltf_mesh_primitive,
	enum gltf_mesh_vertex_attribute type) {

	const size_t accessor_index = p_gltf_mesh_primitive->vertex_attribute_accessors_indices[type];

	if (accessor_index == GLTF_INVALID_INDEX) {
		return 0;
	}

	return &p_gltf->p_accessors[accessor_index];
}

void cx_ed_import_gltf_discover_joint_hierarchy(
	const struct gltf* p_gltf,
	const struct gltf_skin* p_gltf_skin,
	struct skeleton* p_skeleton,
	size_t joint_index) {

	const struct gltf_node* p_gltf_joint_node = &p_gltf->p_nodes[p_gltf_skin->p_joints_indices[joint_index]];
	struct skeleton_joint* p_joint = &p_skeleton->p_joints[joint_index];

	if (p_joint->num_children != SIZE_MAX) {
		return;
	}

	// This may break in instances where we have non-joint nodes attached to joint nodes

	p_joint->num_children = p_gltf_joint_node->num_children;
	p_joint->p_children_indices = CX_CALLOC(p_joint->num_children * sizeof(*p_joint->p_children_indices));

	for (size_t i = 0; i < p_joint->num_children; ++i) {
		size_t translated_gltf_child_joint_node_index = SIZE_MAX;
		for (size_t j = 0; j < p_gltf_skin->num_joints; ++j) {
			if (p_gltf_skin->p_joints_indices[j] == p_gltf_joint_node->p_children_indices[i]) {
				translated_gltf_child_joint_node_index = j;
				break;
			}
		}

		p_joint->p_children_indices[i] = translated_gltf_child_joint_node_index;
		
		p_skeleton->p_joints[translated_gltf_child_joint_node_index].parent_index = joint_index;
		
		cx_ed_import_gltf_discover_joint_hierarchy(
			p_gltf,
			p_gltf_skin,
			p_skeleton,
			translated_gltf_child_joint_node_index);
	}
}

uint16_t cx_ed_import_gltf_process_scene_node(
	struct cx_ed_import_gltf_context* p_context,
	const struct gltf_node* p_gltf_node,
	struct cx_blueprint* p_blueprint) {

	uint16_t new_node_id = cx_blueprint_create_node(p_blueprint);
	
	struct cx_cmp_static_mesh* p_static_mesh_component =
		cx_blueprint_node_add_component(p_blueprint, new_node_id, &cmp_type_static_mesh);

	const struct cx_asset_ref* p_static_mesh_asset_ref = cx_array_at(&p_context->static_mesh_assets, p_gltf_node->mesh_index);
	p_static_mesh_component->asset_ref = *p_static_mesh_asset_ref;

	struct transform* p_node_transform = cx_blueprint_node_get_transform(p_blueprint, new_node_id);

	matrix_decompose_trs(
		p_gltf_node->matrix,
		p_node_transform->position,
		p_node_transform->rotation,
		p_node_transform->scale);

	for (size_t i = 0; i < p_gltf_node->num_children; ++i) {
		const struct gltf_node* p_child_gltf_node = &p_context->p_gltf->p_nodes[p_gltf_node->p_children_indices[i]];

		cx_blueprint_node_set_parent(
			p_blueprint,
			cx_ed_import_gltf_process_scene_node(p_context, p_child_gltf_node, p_blueprint),
			new_node_id);
	}

	return new_node_id;
}

size_t cx_ed_import_gltf_compute_accessor_element_size(const struct gltf_accessor* p_gltf_accessor) {
	size_t num_components = 0;
	switch (p_gltf_accessor->type) {
		case GLTF_ACCESSOR_TYPE_scalar: num_components = 1; break;
		case GLTF_ACCESSOR_TYPE_vec2:   num_components = 2; break;
		case GLTF_ACCESSOR_TYPE_vec3:   num_components = 3; break;
		case GLTF_ACCESSOR_TYPE_vec4:
		case GLTF_ACCESSOR_TYPE_mat2:   num_components = 4; break;
		case GLTF_ACCESSOR_TYPE_mat3:   num_components = 9; break;
		case GLTF_ACCESSOR_TYPE_mat4:   num_components = 16; break;
	};
	size_t component_size = 0;
	switch (p_gltf_accessor->component_type) {
		case GLTF_ACCESSOR_COMPONENT_TYPE_byte:
		case GLTF_ACCESSOR_COMPONENT_TYPE_unsigned_byte:  component_size = 1; break;
		case GLTF_ACCESSOR_COMPONENT_TYPE_short:
		case GLTF_ACCESSOR_COMPONENT_TYPE_unsigned_short: component_size = 2; break;
		case GLTF_ACCESSOR_COMPONENT_TYPE_unsigned_int:
		case GLTF_ACCESSOR_COMPONENT_TYPE_float:          component_size = 4; break;
	}
	return num_components * component_size;
}

void cx_ed_import_gltf_copy_accessor(
	const struct gltf* p_gltf,
	const struct gltf_accessor* p_gltf_accessor,
	void* p_dst,
	size_t dst_stride) {

	const struct gltf_buffer_view* p_gltf_buffer_view = &p_gltf->p_buffer_views[p_gltf_accessor->buffer_view_index];
	const struct gltf_buffer* p_gltf_buffer = &p_gltf->p_buffers[p_gltf_buffer_view->buffer_index];

	const size_t src_offset = p_gltf_buffer_view->byte_offset + p_gltf_accessor->byte_offset;
	const void* p_src = (unsigned char*)p_gltf_buffer->p_bytes + src_offset;

	const size_t element_size = cx_ed_import_gltf_compute_accessor_element_size(p_gltf_accessor);
	const size_t src_stride = p_gltf_buffer_view->byte_stride ? p_gltf_buffer_view->byte_stride : element_size;

	if (dst_stride == 0) {
		dst_stride = element_size;
	}

	if (element_size == src_stride && element_size == dst_stride) {
		memcpy(p_dst, p_src, element_size * p_gltf_accessor->count);
	} else {
		const unsigned char* p_src_ptr = p_src;
		unsigned char* p_dst_ptr = p_dst;
		for (size_t i = 0; i < p_gltf_accessor->count; ++i, p_src_ptr += src_stride, p_dst_ptr += dst_stride) {
			memcpy(p_dst_ptr, p_src_ptr, element_size);
		}
	}
}

void cx_ed_import_gtlf_copy_accessor_to_vertex_buffer(
	const struct gltf* p_gltf,
	const struct gltf_accessor* p_gltf_accessor,
	const struct cx_mesh_vertex_buffer* p_dst,
	const struct cx_mesh_vertex_attribute_layout* p_dst_layout) {

	void* p_dst_bytes = (unsigned char*)p_dst->p_bytes + p_dst_layout->offset;
	cx_ed_import_gltf_copy_accessor(p_gltf, p_gltf_accessor, p_dst_bytes, p_dst_layout->stride);
}

enum cx_texture_min_filter_mode gltf_enum_to_texture_filter_min(enum gltf_sampler_min_filter e) {
	switch (e) {
		case GLTF_SAMPLER_MIN_FILTER_nearest:                return CX_TEXTURE_MIN_FILTER_MODE_nearest;
		default:
		case GLTF_SAMPLER_MIN_FILTER_linear:                 return CX_TEXTURE_MIN_FILTER_MODE_linear;
		case GLTF_SAMPLER_MIN_FILTER_nearest_mipmap_nearest: return CX_TEXTURE_MIN_FILTER_MODE_nearest_mipmap_nearest;
		case GLTF_SAMPLER_MIN_FILTER_linear_mipmap_nearest:  return CX_TEXTURE_MIN_FILTER_MODE_linear_mipmap_nearest;
		case GLTF_SAMPLER_MIN_FILTER_nearest_mipmap_linear:  return CX_TEXTURE_MIN_FILTER_MODE_nearest_mipmap_linear;
		case GLTF_SAMPLER_MIN_FILTER_linear_mipmap_linear:   return CX_TEXTURE_MIN_FILTER_MODE_linear_mipmap_linear;
	}
}

enum cx_texture_mag_filter_mode gltf_enum_to_texture_filter_mag(enum gltf_sampler_mag_filter e) {
	switch (e) {
		case GLTF_SAMPLER_MAG_FILTER_nearest: return CX_TEXTURE_MAG_FILTER_MODE_nearest;
		default:
		case GLTF_SAMPLER_MAG_FILTER_linear:  return CX_TEXTURE_MAG_FILTER_MODE_linear;
	}
}

enum cx_texture_address_mode gltf_enum_to_texture_address_mode(enum gltf_sampler_wrap e) {
	switch (e) {
		default:
		case GLTF_SAMPLER_WRAP_clamp_to_edge:   return CX_TEXTURE_ADDRESS_MODE_clamp_to_edge;
		case GLTF_SAMPLER_WRAP_mirrored_repeat: return CX_TEXTURE_ADDRESS_MODE_mirrored_repeat;
		case GLTF_SAMPLER_WRAP_repeat:          return CX_TEXTURE_ADDRESS_MODE_repeat;
	}
}

enum cx_mesh_draw_mode gltf_enum_to_mesh_primitive_draw_mode(enum gltf_mesh_primitive_mode e) {
	switch (e) {
		case GLTF_MESH_PRIMITIVE_MODE_points:         return CX_MESH_DRAW_MODE_points;
		case GLTF_MESH_PRIMITIVE_MODE_lines:          return CX_MESH_DRAW_MODE_lines;
		case GLTF_MESH_PRIMITIVE_MODE_line_loop:      return CX_MESH_DRAW_MODE_line_loop;
		case GLTF_MESH_PRIMITIVE_MODE_line_strip:     return CX_MESH_DRAW_MODE_line_strip;
		default:
		case GLTF_MESH_PRIMITIVE_MODE_triangles:      return CX_MESH_DRAW_MODE_triangles;
		case GLTF_MESH_PRIMITIVE_MODE_triangle_strip: return CX_MESH_DRAW_MODE_triangle_strip;
		case GLTF_MESH_PRIMITIVE_MODE_triangle_fan:   return CX_MESH_DRAW_MODE_triangle_fan;
	}
}

enum cx_mesh_vertex_index_type gltf_enum_to_vertex_index_type(enum gltf_accessor_component_type e) {
	switch (e) {
		case GLTF_ACCESSOR_COMPONENT_TYPE_unsigned_byte:  return CX_MESH_VERTEX_INDEX_TYPE_u8;
		case GLTF_ACCESSOR_COMPONENT_TYPE_unsigned_short: return CX_MESH_VERTEX_INDEX_TYPE_u16;
		default:                                          return CX_MESH_VERTEX_INDEX_TYPE_u32;
	}
}
