#include <stdlib.h>
#include <string.h>

#include "asset.h"
#include "asset.h"
#include "gltf.h"
#include "image.h"
#include "import_gltf.h"
#include "logging.h"
#include "material.h"
#include "matrix.h"
#include "mesh.h"
#include "scene.h"
#include "skeleton.h"
#include "skeletal_animation.h"
#include "static_mesh.h"
#include "stb_image.h"
#include "texture.h"
#include "transform_animation.h"

struct gltf_importer {
    const struct gltf*         p_gltf;
    struct import_gltf_result* p_result;
    struct asset_package*      p_asset_package;
};

static void import_gltf_image(struct gltf_importer* p_importer, size_t gltf_image_index);
static void import_gltf_texture(struct gltf_importer* p_importer, size_t gltf_texture_index);
static void import_gltf_material(struct gltf_importer* p_importer, size_t gltf_material_index);
static void import_gltf_mesh(struct gltf_importer* p_importer, size_t gltf_mesh_index);
static void import_gltf_mesh_primitive(struct gltf_importer* p_importer, struct static_mesh* p_mesh, size_t gltf_mesh_index, size_t mesh_primitive_index);
static void import_gltf_skin(struct gltf_importer* p_importer, size_t gltf_skin_index);
static void import_gltf_animation(struct gltf_importer* p_importer, size_t gltf_animation_index);
static void import_gltf_scene(struct gltf_importer* p_importer, size_t gltf_scene_index);

static const struct gltf_accessor* get_gltf_mesh_primitive_vertex_attribute_accessor(const struct gltf* p_gltf, const struct gltf_mesh_primitive* p_gltf_mesh_primitive, enum gltf_mesh_vertex_attribute type);

static void discover_joint_hierarchy(const struct gltf* p_gltf, const struct gltf_skin* p_gltf_skin, struct skeleton* p_skeleton, size_t joint_index);

static struct scene_entity* process_scene_node(struct gltf_importer* p_importer, const struct gltf_node* p_gltf_node, struct scene* p_scene);

static size_t gltf_accessor_element_size(const struct gltf_accessor* p_gltf_accessor);
static void   copy_gltf_accessor(const struct gltf* p_gltf, const struct gltf_accessor* p_gltf_accessor, void* p_dst, size_t dst_stride);
static void   copy_gltf_accessor_to_vertex_buffer(const struct gltf* p_gltf, const struct gltf_accessor* p_gltf_accessor, const struct vertex_buffer* p_dst, const struct vertex_attribute_layout* p_dst_layout);

static enum texture_filter_min       gltf_enum_to_texture_filter_min(enum gltf_sampler_min_filter en);
static enum texture_filter_mag       gltf_enum_to_texture_filter_mag(enum gltf_sampler_mag_filter en);
static enum texture_address_mode     gltf_enum_to_texture_address_mode(enum gltf_sampler_wrap en);
static enum vertex_index_type        gltf_enum_to_vertex_index_type(enum gltf_accessor_component_type type);
static enum mesh_primitive_draw_mode gltf_enum_to_mesh_primitive_draw_mode(enum gltf_mesh_primitive_mode mode);

void import_gltf(const struct gltf* p_gltf, struct asset_package* p_asset_package, struct import_gltf_result* p_result) {
    struct gltf_importer importer = {
        .p_gltf = p_gltf,
        .p_result = p_result,
        .p_asset_package = p_asset_package
    };

    *p_result = (struct import_gltf_result) {0};
    
    p_result->p_images = calloc(p_gltf->num_images, sizeof(*p_result->p_images));
    for (size_t i = 0; i < p_gltf->num_images; ++i) {
        import_gltf_image(&importer, i);
    }

    p_result->p_textures = calloc(p_gltf->num_textures, sizeof(*p_result->p_textures));
    for (size_t i = 0; i < p_gltf->num_textures; ++i) {
        import_gltf_texture(&importer, i);
    }

    p_result->p_materials = calloc(p_gltf->num_materials, sizeof(*p_result->p_materials));
    for (size_t i = 0; i < p_gltf->num_materials; ++i) {
        import_gltf_material(&importer, i);
    }

    p_result->p_meshes = calloc(p_gltf->num_meshes, sizeof(*p_result->p_meshes));
    for (size_t i = 0; i < p_gltf->num_meshes; ++i) {
        import_gltf_mesh(&importer, i);
    }

    p_result->p_skeletons = calloc(p_gltf->num_skins, sizeof(*p_result->p_skeletons));
    for (size_t i = 0; i < p_gltf->num_skins; ++i) {
        import_gltf_skin(&importer, i);
    }

    // todo
    for (size_t i = 0; i < p_gltf->num_animations; ++i) {
        import_gltf_animation(&importer, i);
    }

    p_result->p_scenes = calloc(p_gltf->num_scenes, sizeof(*p_result->p_scenes));
    for (size_t i = 0; i < p_gltf->num_scenes; ++i) {
        import_gltf_scene(&importer, i);
    }
}

void import_gltf_free(struct import_gltf_result* p_result) {
    free(p_result->p_images);
    free(p_result->p_textures);
    free(p_result->p_materials);
    free(p_result->p_meshes);
    free(p_result->p_skeletons);
    free(p_result->p_scenes);
    *p_result = (struct import_gltf_result) {0};
}

void import_gltf_image(struct gltf_importer* p_importer, size_t gltf_image_index) {
    const struct gltf_image* p_gltf_image = &p_importer->p_gltf->p_images[gltf_image_index];

    const void* p_bytes;
    size_t size;

    if (p_gltf_image->b_uri_source) {
        p_bytes = p_gltf_image->source.uri.p_data;
        size = p_gltf_image->source.uri.size;
    } else {
        const struct gltf_buffer_view* p_gltf_buffer_view = &p_importer->p_gltf->p_buffer_views[p_gltf_image->source.buffer_view_index];
        const struct gltf_buffer* p_gltf_buffer = &p_importer->p_gltf->p_buffers[p_gltf_buffer_view->buffer_index];
        p_bytes = (unsigned char*)p_gltf_buffer->p_bytes + p_gltf_buffer_view->byte_offset;
        size = p_gltf_buffer_view->byte_length;
    }

    struct image* p_image = malloc(sizeof(struct image));

    int x, y, comp;
    p_image->p_pixel_data = stbi_load_from_memory(p_bytes, size, &x, &y, &comp, 0);

    if (!p_image->p_pixel_data) {
        cx_log(CX_LOG_ERROR, 0, "Failed to import image from glTF\n");
        free(p_image);
        return;
    }

    p_image->width = x;
    p_image->height = y;
    p_image->num_channels = comp;
    
    asset_handle handle = asset_package_new_record(p_importer->p_asset_package, ASSET_TYPE_IMAGE);
    handle->_asset._p_data = p_image;

    p_importer->p_result->p_images[p_importer->p_result->num_images] = handle;
    ++p_importer->p_result->num_images;
}

void import_gltf_texture(struct gltf_importer* p_importer, size_t gltf_texture_index) {
    const struct gltf_texture* p_gltf_texture = &p_importer->p_gltf->p_textures[gltf_texture_index];

    struct texture* p_texture = malloc(sizeof(struct texture));

    *p_texture = (struct texture){
        .p_source_image = p_importer->p_result->p_images[p_gltf_texture->source_image_index],
        .sampler = {
            .filter_min = gltf_enum_to_texture_filter_min(p_gltf_texture->sampler_min_filter),
            .filter_mag = gltf_enum_to_texture_filter_mag(p_gltf_texture->sampler_mag_filter),
            .address_mode_u = gltf_enum_to_texture_address_mode(p_gltf_texture->sampler_wrap_s),
            .address_mode_v = gltf_enum_to_texture_address_mode(p_gltf_texture->sampler_wrap_t)
        }
    };
    
    asset_handle handle = asset_package_new_record(p_importer->p_asset_package, ASSET_TYPE_TEXTURE);
    handle->_asset._p_data = p_texture;

    p_importer->p_result->p_textures[p_importer->p_result->num_textures] = handle;
    ++p_importer->p_result->num_textures;
}

void import_gltf_material(struct gltf_importer* p_importer, size_t gltf_material_index) {
    const struct gltf_material* p_gltf_material = &p_importer->p_gltf->p_materials[gltf_material_index];
    struct material* p_material = malloc(sizeof(struct material));

    *p_material = (struct material) {
        .p_texture = p_importer->p_result->p_textures[p_gltf_material->pbr_base_color_texture.source_texture_index]
    };
    
    asset_handle handle = asset_package_new_record(p_importer->p_asset_package, ASSET_TYPE_MATERIAL);
    handle->_asset._p_data = p_material;

    p_importer->p_result->p_materials[p_importer->p_result->num_materials] = handle;
    ++p_importer->p_result->num_materials;
}

void import_gltf_mesh(struct gltf_importer* p_importer, size_t gltf_mesh_index) {
    const struct gltf_mesh* p_gltf_mesh = &p_importer->p_gltf->p_meshes[gltf_mesh_index];
    struct static_mesh* p_mesh = malloc(sizeof(struct static_mesh));

    *p_mesh = (struct static_mesh) {
        .num_primitives = p_gltf_mesh->num_primitives,
        .p_primitives = calloc(p_gltf_mesh->num_primitives, sizeof(*p_mesh->p_primitives)),
        .p_materials = calloc(p_gltf_mesh->num_primitives, sizeof(*p_mesh->p_materials)),
    };
    
    for (size_t i = 0; i < p_gltf_mesh->num_primitives; ++i) {
        import_gltf_mesh_primitive(p_importer, p_mesh, gltf_mesh_index, i);
    }
    
    asset_handle handle = asset_package_new_record(p_importer->p_asset_package, ASSET_TYPE_STATIC_MESH);
    handle->_asset._p_data = p_mesh;

    p_importer->p_result->p_meshes[p_importer->p_result->num_meshes] = handle;
    ++p_importer->p_result->num_meshes;
}

void import_gltf_mesh_primitive(struct gltf_importer* p_importer, struct static_mesh* p_mesh, size_t gltf_mesh_index, size_t mesh_primitive_index) {
    const struct gltf_mesh_primitive* p_gltf_mesh_primitive = &p_importer->p_gltf->p_meshes[gltf_mesh_index].p_primitives[mesh_primitive_index];
    struct mesh_primitive* p_mesh_primitive = &p_mesh->p_primitives[mesh_primitive_index];

    const int b_generate_normals = 1;
    
    const struct gltf_accessor* p_gltf_positions_accessor =   get_gltf_mesh_primitive_vertex_attribute_accessor(p_importer->p_gltf, p_gltf_mesh_primitive, GLTF_MESH_VERTEX_ATTRIBUTE_position);
    const struct gltf_accessor* p_gltf_normals_accessor =     get_gltf_mesh_primitive_vertex_attribute_accessor(p_importer->p_gltf, p_gltf_mesh_primitive, GLTF_MESH_VERTEX_ATTRIBUTE_normal);
    const struct gltf_accessor* p_gltf_tangents_accessor =    get_gltf_mesh_primitive_vertex_attribute_accessor(p_importer->p_gltf, p_gltf_mesh_primitive, GLTF_MESH_VERTEX_ATTRIBUTE_tangent);
    const struct gltf_accessor* p_gltf_texcoords_0_accessor = get_gltf_mesh_primitive_vertex_attribute_accessor(p_importer->p_gltf, p_gltf_mesh_primitive, GLTF_MESH_VERTEX_ATTRIBUTE_texcoord_0);
    const struct gltf_accessor* p_gltf_texcoords_1_accessor = get_gltf_mesh_primitive_vertex_attribute_accessor(p_importer->p_gltf, p_gltf_mesh_primitive, GLTF_MESH_VERTEX_ATTRIBUTE_texcoord_1);
    const struct gltf_accessor* p_gltf_colors_accessor =      get_gltf_mesh_primitive_vertex_attribute_accessor(p_importer->p_gltf, p_gltf_mesh_primitive, GLTF_MESH_VERTEX_ATTRIBUTE_color);
    const struct gltf_accessor* p_gltf_joints_accessor =      get_gltf_mesh_primitive_vertex_attribute_accessor(p_importer->p_gltf, p_gltf_mesh_primitive, GLTF_MESH_VERTEX_ATTRIBUTE_joints);
    const struct gltf_accessor* p_gltf_weights_accessor =     get_gltf_mesh_primitive_vertex_attribute_accessor(p_importer->p_gltf, p_gltf_mesh_primitive, GLTF_MESH_VERTEX_ATTRIBUTE_weights);
    
    const struct gltf_accessor* p_attribute_gltf_accessors[8] = {0};
    struct vertex_attribute attributes[8] = {0};
    size_t num_attributes = 0;

    size_t offset = 0;

    if (!!p_gltf_positions_accessor) {
        p_attribute_gltf_accessors[num_attributes] = p_gltf_positions_accessor;
        attributes[num_attributes].index = 0;
        attributes[num_attributes].vertex_buffer_index = 0;
        attributes[num_attributes].layout.offset = offset;
        attributes[num_attributes].layout.component_count = 3;
        attributes[num_attributes].layout.component_type = VERTEX_ATTRIBUTE_TYPE_f32;
        offset += vertex_attribute_layout_element_size(&attributes[num_attributes].layout);
        ++num_attributes;
    } else {
        cx_log(CX_LOG_ERROR, 0, "Mesh import failed: missing position data\n");
        return;
    }

    if (!!p_gltf_normals_accessor || b_generate_normals) {
        p_attribute_gltf_accessors[num_attributes] = p_gltf_normals_accessor;
        attributes[num_attributes].index = 1;
        attributes[num_attributes].vertex_buffer_index = 0;
        attributes[num_attributes].layout.offset = offset;
        attributes[num_attributes].layout.component_count = 3;
        attributes[num_attributes].layout.component_type = VERTEX_ATTRIBUTE_TYPE_f32;
        offset += vertex_attribute_layout_element_size(&attributes[num_attributes].layout);
        ++num_attributes;

        if (!!p_gltf_tangents_accessor) {
            p_attribute_gltf_accessors[num_attributes] = p_gltf_tangents_accessor;
            attributes[num_attributes].index = 2;
            attributes[num_attributes].vertex_buffer_index = 0;
            attributes[num_attributes].layout.offset = offset;
            attributes[num_attributes].layout.component_count = 3;
            attributes[num_attributes].layout.component_type = VERTEX_ATTRIBUTE_TYPE_f32;
            offset += vertex_attribute_layout_element_size(&attributes[num_attributes].layout);
            ++num_attributes;
        }
    }

    if (!!p_gltf_texcoords_0_accessor) {
        p_attribute_gltf_accessors[num_attributes] = p_gltf_texcoords_0_accessor;
        attributes[num_attributes].index = 3;
        attributes[num_attributes].vertex_buffer_index = 0;
        attributes[num_attributes].layout.offset = offset;
        attributes[num_attributes].layout.component_count = 3;
        attributes[num_attributes].layout.component_type = VERTEX_ATTRIBUTE_TYPE_f32;
        offset += vertex_attribute_layout_element_size(&attributes[num_attributes].layout);
        ++num_attributes;
    }

    if (!!p_gltf_texcoords_1_accessor) {
        p_attribute_gltf_accessors[num_attributes] = p_gltf_texcoords_1_accessor;
        attributes[num_attributes].index = 4;
        attributes[num_attributes].vertex_buffer_index = 0;
        attributes[num_attributes].layout.offset = offset;
        attributes[num_attributes].layout.component_count = 3;
        attributes[num_attributes].layout.component_type = VERTEX_ATTRIBUTE_TYPE_f32;
        offset += vertex_attribute_layout_element_size(&attributes[num_attributes].layout);
        ++num_attributes;
    }

    if (!!p_gltf_colors_accessor) {
        p_attribute_gltf_accessors[num_attributes] = p_gltf_colors_accessor;
        attributes[num_attributes].index = 5;
        attributes[num_attributes].vertex_buffer_index = 0;
        attributes[num_attributes].layout.offset = offset;
        attributes[num_attributes].layout.component_count = 4;
        attributes[num_attributes].layout.component_type = VERTEX_ATTRIBUTE_TYPE_f32;
        offset += vertex_attribute_layout_element_size(&attributes[num_attributes].layout);
        ++num_attributes;
    }

    if (!!p_gltf_joints_accessor && !!p_gltf_weights_accessor) {
        p_attribute_gltf_accessors[num_attributes] = p_gltf_joints_accessor;
        attributes[num_attributes].index = 6;
        attributes[num_attributes].vertex_buffer_index = 0;
        attributes[num_attributes].layout.offset = offset;
        attributes[num_attributes].layout.component_count = 4;
        attributes[num_attributes].layout.component_type = VERTEX_ATTRIBUTE_TYPE_f32;
        offset += vertex_attribute_layout_element_size(&attributes[num_attributes].layout);
        ++num_attributes;

        p_attribute_gltf_accessors[num_attributes] = p_gltf_weights_accessor;
        attributes[num_attributes].index = 7;
        attributes[num_attributes].vertex_buffer_index = 0;
        attributes[num_attributes].layout.offset = offset;
        attributes[num_attributes].layout.component_count = 4;
        attributes[num_attributes].layout.component_type = VERTEX_ATTRIBUTE_TYPE_f32;
        offset += vertex_attribute_layout_element_size(&attributes[num_attributes].layout);
        ++num_attributes;
    } else if (!!p_gltf_joints_accessor || !!p_gltf_weights_accessor) {
        cx_log(CX_LOG_ERROR, 0, "Mesh joints data or weights data present, but missing the other. Skipping import of joints/weights data\n");
    }

    for (size_t i = 0; i < num_attributes; ++i) {
        attributes[i].layout.stride = offset;
    }
    
    p_mesh_primitive->vertex_count = p_gltf_positions_accessor->count;

    // Allocate vertex buffer storage

    struct vertex_buffer buffers[8] = {0};
    size_t num_buffers = 0;

    for (size_t i = 0; i < num_attributes; ++i) {
        if (num_buffers <= attributes[i].vertex_buffer_index) {
            num_buffers = attributes[i].vertex_buffer_index + 1;
        }

        const size_t attribute_size = vertex_attribute_layout_element_size(&attributes[i].layout);
        buffers[attributes[i].vertex_buffer_index].size += attribute_size * p_mesh_primitive->vertex_count;
    }

    for (size_t i = 0; i < num_buffers; ++i) {
        buffers[i].p_bytes = malloc(buffers[i].size);
    }

    // Copy over vertex buffer data
    
    for (size_t i = 0; i < num_attributes; ++i) {
        if (p_attribute_gltf_accessors[i]) {
            copy_gltf_accessor_to_vertex_buffer(p_importer->p_gltf, p_attribute_gltf_accessors[i], &buffers[attributes[i].vertex_buffer_index], &attributes[i].layout);
        }
    }

    // Vertex index buffer

    if (p_gltf_mesh_primitive->vertex_indices_accessor_index != GLTF_INVALID_INDEX) {
        const struct gltf_accessor* p_gltf_indices_accessor = &p_importer->p_gltf->p_accessors[p_gltf_mesh_primitive->vertex_indices_accessor_index];

        const size_t element_size = gltf_accessor_element_size(p_gltf_indices_accessor);
        
        p_mesh_primitive->index_buffer.count = p_gltf_indices_accessor->count;
        p_mesh_primitive->index_buffer.p_bytes = malloc(p_mesh_primitive->index_buffer.count * element_size);
        p_mesh_primitive->index_buffer.type = gltf_enum_to_vertex_index_type(p_gltf_indices_accessor->component_type);

        copy_gltf_accessor(p_importer->p_gltf, p_gltf_indices_accessor, p_mesh_primitive->index_buffer.p_bytes, 0);
    }

    if (p_gltf_mesh_primitive->material_index != GLTF_INVALID_INDEX) {
        p_mesh->p_materials[mesh_primitive_index] = p_importer->p_result->p_materials[p_gltf_mesh_primitive->material_index];
    }

    p_mesh_primitive->draw_mode = gltf_enum_to_mesh_primitive_draw_mode(p_gltf_mesh_primitive->mode);

    p_mesh_primitive->num_vertex_buffers = num_buffers;
    p_mesh_primitive->p_vertex_buffers = malloc(p_mesh_primitive->num_vertex_buffers * sizeof(*p_mesh_primitive->p_vertex_buffers));
    memcpy(p_mesh_primitive->p_vertex_buffers, buffers, p_mesh_primitive->num_vertex_buffers * sizeof(*p_mesh_primitive->p_vertex_buffers));

    p_mesh_primitive->num_attributes = num_attributes;
    p_mesh_primitive->p_attributes = malloc(p_mesh_primitive->num_attributes * sizeof(*p_mesh_primitive->p_attributes));
    memcpy(p_mesh_primitive->p_attributes, attributes, p_mesh_primitive->num_attributes * sizeof(*p_mesh_primitive->p_attributes));
    
    // Post-processing

    if (!p_gltf_normals_accessor && b_generate_normals) {
        mesh_generate_normals(p_mesh_primitive, 0, 1);
    }

    memcpy(p_mesh_primitive->bounds_min, p_gltf_positions_accessor->min, sizeof(p_mesh_primitive->bounds_min));
    memcpy(p_mesh_primitive->bounds_max, p_gltf_positions_accessor->max, sizeof(p_mesh_primitive->bounds_max));
}

void import_gltf_skin(struct gltf_importer* p_importer, size_t gltf_skin_index) {
    const struct gltf_skin* p_gltf_skin = &p_importer->p_gltf->p_skins[gltf_skin_index];
    struct skeleton* p_skeleton = malloc(sizeof(struct skeleton));

    *p_skeleton = (struct skeleton) {
        .num_joints = p_gltf_skin->num_joints,
        .p_joints = calloc(p_gltf_skin->num_joints, sizeof(*p_skeleton->p_joints)),
        .p_inverse_bind_matrices = calloc(p_gltf_skin->num_joints, sizeof(float) * 16)
    };
    
    if (p_gltf_skin->inverse_bind_matrices_accessor_index == GLTF_INVALID_INDEX) {
        for (size_t i = 0; i < p_skeleton->num_joints; ++i) {
            matrix_make_identity(&p_skeleton->p_inverse_bind_matrices[i * 16]);
        }
    } else {
        const struct gltf_accessor* p_gltf_inverse_bind_matrices_accessor = &p_importer->p_gltf->p_accessors[p_gltf_skin->inverse_bind_matrices_accessor_index];
        copy_gltf_accessor(p_importer->p_gltf, p_gltf_inverse_bind_matrices_accessor, p_skeleton->p_inverse_bind_matrices, 0);
    }

    for (size_t i = 0; i < p_skeleton->num_joints; ++i) {
        const struct gltf_node* p_gltf_joint_node = &p_importer->p_gltf->p_nodes[p_gltf_skin->p_joints_indices[i]];
        struct skeleton_joint* p_joint = &p_skeleton->p_joints[i];

        p_joint->num_children = SIZE_MAX;
        p_joint->parent_index = SIZE_MAX;
        memcpy(p_joint->transform, p_gltf_joint_node->matrix, sizeof(p_gltf_joint_node->matrix));
    }

    discover_joint_hierarchy(p_importer->p_gltf, p_gltf_skin, p_skeleton, 0);
    
    for (size_t i = 0; i < p_skeleton->num_joints; ++i) {
        struct skeleton_joint* p_joint = &p_skeleton->p_joints[i];
        if (p_joint->parent_index == SIZE_MAX) {
            ++p_skeleton->num_root_joints;
            p_skeleton->p_root_joints_indices = realloc(p_skeleton->p_root_joints_indices, p_skeleton->num_root_joints * sizeof(*p_skeleton->p_root_joints_indices));
            p_skeleton->p_root_joints_indices[p_skeleton->num_root_joints - 1] = i;
        }
    }

    asset_handle handle = asset_package_new_record(p_importer->p_asset_package, ASSET_TYPE_SKELETON);
    handle->_asset._p_data = p_skeleton;

    p_importer->p_result->p_skeletons[p_importer->p_result->num_skeleton] = handle;
    ++p_importer->p_result->num_skeleton;
}

void import_gltf_animation(struct gltf_importer* p_importer, size_t gltf_animation_index) {
    const struct gltf_animation* p_gltf_animation = &p_importer->p_gltf->p_animations[gltf_animation_index];

    const struct gltf_animation_channel* p_gltf_channel = &p_gltf_animation->p_channels[0];
    const struct gltf_node* p_gltf_node = &p_importer->p_gltf->p_nodes[p_gltf_channel->target_node_index];
    const int b_is_skeletal = p_gltf_node->b_is_joint;
    
    struct skeletal_animation skeletal_animation;

    for (size_t i = 0; i < p_gltf_animation->num_channels; ++i) {
        p_gltf_channel = &p_gltf_animation->p_channels[i];        
        const struct gltf_animation_sampler* p_gltf_sampler = &p_gltf_animation->p_samplers[p_gltf_channel->source_sampler_index];
        const struct gltf_accessor* p_gltf_sampler_input_accessor = &p_importer->p_gltf->p_accessors[p_gltf_sampler->input_accessor_index];
        const struct gltf_accessor* p_gltf_sampler_output_accessor = &p_importer->p_gltf->p_accessors[p_gltf_sampler->output_accessor_index];
        
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

        transform_animation.sampler.p_keyframe_timestamps = malloc(gltf_accessor_element_size(p_gltf_sampler_input_accessor) * transform_animation.sampler.num_keyframes);
        copy_gltf_accessor(p_importer->p_gltf, p_gltf_sampler_input_accessor, transform_animation.sampler.p_keyframe_timestamps, 0);

        transform_animation.sampler.p_keyframe_values = malloc(gltf_accessor_element_size(p_gltf_sampler_output_accessor) * transform_animation.sampler.num_keyframes);
        copy_gltf_accessor(p_importer->p_gltf, p_gltf_sampler_output_accessor, transform_animation.sampler.p_keyframe_values, 0);
        
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

        //     // skeletal_animation.p_joint_transform_animations[skeletal_animation.num_joint_transform_animations] = transform_animation;
        //     // skeletal_animation.p_target_joint_indices[skeletal_animation.num_joint_transform_animations] = skeleton_target_joint_index;
        //     // ++skeletal_animation.num_joint_transform_animations;
        // } else {
        //     // it's just a normal transform animation I suppose...
        // }
    }
}

void import_gltf_scene(struct gltf_importer* p_importer, size_t gltf_scene_index) {
    const struct gltf_scene* p_gltf_scene = &p_importer->p_gltf->p_scenes[gltf_scene_index];
    struct scene* p_scene = malloc(sizeof(struct scene));
    scene_init(p_scene);

    for (size_t i = 0; i < p_gltf_scene->num_root_nodes; ++i) {
        const struct gltf_node* p_gltf_root_node = &p_importer->p_gltf->p_nodes[p_gltf_scene->p_root_nodes_indices[i]];
        process_scene_node(p_importer, p_gltf_root_node, p_scene);
    }

    asset_handle handle = asset_package_new_record(p_importer->p_asset_package, ASSET_TYPE_SCENE);
    handle->_asset._p_data = p_scene;

    p_importer->p_result->p_scenes[p_importer->p_result->num_scenes] = handle;
    ++p_importer->p_result->num_scenes;
}

const struct gltf_accessor* get_gltf_mesh_primitive_vertex_attribute_accessor(const struct gltf* p_gltf, const struct gltf_mesh_primitive* p_gltf_mesh_primitive, enum gltf_mesh_vertex_attribute type) {
    const size_t accessor_index = p_gltf_mesh_primitive->vertex_attribute_accessors_indices[type];

    if (accessor_index == GLTF_INVALID_INDEX) {
        return 0;
    }

    return &p_gltf->p_accessors[accessor_index];
}

void discover_joint_hierarchy(const struct gltf* p_gltf, const struct gltf_skin* p_gltf_skin, struct skeleton* p_skeleton, size_t joint_index) {
    const struct gltf_node* p_gltf_joint_node = &p_gltf->p_nodes[p_gltf_skin->p_joints_indices[joint_index]];
    struct skeleton_joint* p_joint = &p_skeleton->p_joints[joint_index];

    if (p_joint->num_children != SIZE_MAX) {
        return;
    }

    // This may break in instances where we have non-joint nodes attached to joint nodes

    p_joint->num_children = p_gltf_joint_node->num_children;
    p_joint->p_children_indices = calloc(p_joint->num_children, sizeof(*p_joint->p_children_indices));

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
        
        discover_joint_hierarchy(p_gltf, p_gltf_skin, p_skeleton, translated_gltf_child_joint_node_index);
    }
}

struct scene_entity* process_scene_node(struct gltf_importer* p_importer, const struct gltf_node* p_gltf_node, struct scene* p_scene) {
    struct scene_entity* p_entity = scene_new_entity(p_scene);
    
    p_entity->p_mesh = p_importer->p_result->p_meshes[p_gltf_node->mesh_index];

    matrix_decompose_trs(p_gltf_node->matrix, p_entity->transform.position, p_entity->transform.rotation, p_entity->transform.scale);

    for (size_t i = 0; i < p_gltf_node->num_children; ++i) {
        const struct gltf_node* p_child_gltf_node = &p_importer->p_gltf->p_nodes[p_gltf_node->p_children_indices[i]];
        struct scene_entity* p_child_entity = process_scene_node(p_importer, p_child_gltf_node, p_scene);
        p_child_entity->transform.p_local_transform = &p_entity->transform;
    }

    return p_entity;
}

size_t gltf_accessor_element_size(const struct gltf_accessor* p_gltf_accessor) {
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

void copy_gltf_accessor(const struct gltf* p_gltf, const struct gltf_accessor* p_gltf_accessor, void* p_dst, size_t dst_stride) {
    const struct gltf_buffer_view* p_gltf_buffer_view = &p_gltf->p_buffer_views[p_gltf_accessor->buffer_view_index];
    const struct gltf_buffer* p_gltf_buffer = &p_gltf->p_buffers[p_gltf_buffer_view->buffer_index];
    const void* p_src = (unsigned char*)p_gltf_buffer->p_bytes + p_gltf_buffer_view->byte_offset + p_gltf_accessor->byte_offset;

    const size_t element_size = gltf_accessor_element_size(p_gltf_accessor);
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

void copy_gltf_accessor_to_vertex_buffer(const struct gltf* p_gltf, const struct gltf_accessor* p_gltf_accessor, const struct vertex_buffer* p_dst, const struct vertex_attribute_layout* p_dst_layout) {
    void* p_dst_bytes = (unsigned char*)p_dst->p_bytes + p_dst_layout->offset;
    copy_gltf_accessor(p_gltf, p_gltf_accessor, p_dst_bytes, p_dst_layout->stride);
}

enum texture_filter_min gltf_enum_to_texture_filter_min(enum gltf_sampler_min_filter en) {
    switch (en) {
        case GLTF_SAMPLER_MIN_FILTER_nearest:                return TEXTURE_FILTER_MIN_nearest;
        default:
        case GLTF_SAMPLER_MIN_FILTER_linear:                 return TEXTURE_FILTER_MIN_linear;
        case GLTF_SAMPLER_MIN_FILTER_nearest_mipmap_nearest: return TEXTURE_FILTER_MIN_nearest_mipmap_nearest;
        case GLTF_SAMPLER_MIN_FILTER_linear_mipmap_nearest:  return TEXTURE_FILTER_MIN_linear_mipmap_nearest;
        case GLTF_SAMPLER_MIN_FILTER_nearest_mipmap_linear:  return TEXTURE_FILTER_MIN_nearest_mipmap_linear;
        case GLTF_SAMPLER_MIN_FILTER_linear_mipmap_linear:   return TEXTURE_FILTER_MIN_linear_mipmap_linear;
    }
}

enum texture_filter_mag gltf_enum_to_texture_filter_mag(enum gltf_sampler_mag_filter en) {
    switch (en) {
        case GLTF_SAMPLER_MAG_FILTER_nearest:                return TEXTURE_FILTER_MAG_nearest;
        default:
        case GLTF_SAMPLER_MAG_FILTER_linear:                 return TEXTURE_FILTER_MAG_linear;
    }
}

enum texture_address_mode gltf_enum_to_texture_address_mode(enum gltf_sampler_wrap en) {
    switch (en) {
        default:
        case GLTF_SAMPLER_WRAP_clamp_to_edge: return TEXTURE_ADDRESS_MODE_clamp_to_edge;
        case GLTF_SAMPLER_WRAP_mirrored_repeat: return TEXTURE_ADDRESS_MODE_mirrored_repeat;
        case GLTF_SAMPLER_WRAP_repeat: return TEXTURE_ADDRESS_MODE_repeat;
    }
}

enum mesh_primitive_draw_mode gltf_enum_to_mesh_primitive_draw_mode(enum gltf_mesh_primitive_mode en) {
    switch (en) {
        case GLTF_MESH_PRIMITIVE_MODE_points:         return MESH_PRIMITIVE_DRAW_MODE_points;
        case GLTF_MESH_PRIMITIVE_MODE_lines:          return MESH_PRIMITIVE_DRAW_MODE_lines;
        case GLTF_MESH_PRIMITIVE_MODE_line_loop:      return MESH_PRIMITIVE_DRAW_MODE_line_loop;
        case GLTF_MESH_PRIMITIVE_MODE_line_strip:     return MESH_PRIMITIVE_DRAW_MODE_line_strip;
        default:
        case GLTF_MESH_PRIMITIVE_MODE_triangles:      return MESH_PRIMITIVE_DRAW_MODE_triangles;
        case GLTF_MESH_PRIMITIVE_MODE_triangle_strip: return MESH_PRIMITIVE_DRAW_MODE_triangle_strip;
        case GLTF_MESH_PRIMITIVE_MODE_triangle_fan:   return MESH_PRIMITIVE_DRAW_MODE_triangle_fan;
    }
}

enum vertex_index_type gltf_enum_to_vertex_index_type(enum gltf_accessor_component_type en) {
    switch (en) {
        case GLTF_ACCESSOR_COMPONENT_TYPE_unsigned_byte:  return VERTEX_INDEX_TYPE_u8;
        case GLTF_ACCESSOR_COMPONENT_TYPE_unsigned_short: return VERTEX_INDEX_TYPE_u16;
        default:                                          return VERTEX_INDEX_TYPE_u32;
    }
}