#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gltf.h"
#include "json.h"
#include "logging.h"

#define LOG_CAT_GLTF "glTF"

struct glb_chunk {
    size_t   length;
    uint32_t type;
    void*    p_bytes;
};

struct gltf_reader {
    const char*        s_filename;
    char               s_filedir[260];
    char               s_temp_filepath[260];
    struct gltf*       p_result;
    struct glb_chunk   p_glb_buffer_chunk;
    struct json_value* p_json_gltf;
    int                error;
};

static void copy_file_directory(const char* s_filename, char* s_dst);
static void make_file_path(const char* s_dirname, const char* s_filename, char* s_dst);

static int read_is_file_glb(FILE* p_file, int* p_b_result);
static int read_glb_file(FILE* p_file, struct gltf_reader* p_reader);
static int read_glb_file_chunk(FILE* p_file, struct glb_chunk* p_result);
static int read_json_file(FILE* p_file, struct gltf_reader* p_reader);

static void read_gltf_asset(struct gltf_reader* p_reader);
static void read_gltf_extensions(struct gltf_reader* p_reader);

typedef void(*read_element_proc)(struct gltf_reader*, const struct json_value*, void*);
static void read_gltf_object_array(struct gltf_reader* p_reader, const char* s_array_key, void** p_elements, size_t* n_elements, size_t element_size, read_element_proc read_element);
static void read_gltf_object_property(struct gltf_reader* p_reader, const struct json_value* p_json_gltf_object, const char* s_key, int b_required, int(*type_checker)(const struct json_value*), const struct json_value** p_result);
static void read_gltf_number_array(struct gltf_reader* p_reader, const struct json_value* p_json_gltf_number_array, size_t count, float* p_result);
static void read_gltf_texture_info(struct gltf_reader* p_reader, const struct json_value* p_json_texture_info, struct gltf_material_texture_info* p_texture_info);

static void read_buffer(struct gltf_reader* p_reader, const struct json_value* p_json_buffer, void* p_buffer);
static void read_buffer_view(struct gltf_reader* p_reader, const struct json_value* p_json_buffer, void* p_buffer_view);
static void read_accessor(struct gltf_reader* p_reader, const struct json_value* p_json_buffer, void* p_accessor);
static void read_image(struct gltf_reader* p_reader, const struct json_value* p_json_buffer, void* p_image);
static void read_texture(struct gltf_reader* p_reader, const struct json_value* p_json_buffer, void* p_texture);
static void read_material(struct gltf_reader* p_reader, const struct json_value* p_json_buffer, void* p_material);
static void read_mesh(struct gltf_reader* p_reader, const struct json_value* p_json_buffer, void* p_mesh);
static void read_skin(struct gltf_reader* p_reader, const struct json_value* p_json_buffer, void* p_skin);
static void read_animation(struct gltf_reader* p_reader, const struct json_value* p_json_buffer, void* p_animation);
static void read_node(struct gltf_reader* p_reader, const struct json_value* p_json_buffer, void* p_node);
static void read_scene(struct gltf_reader* p_reader, const struct json_value* p_json_buffer, void* p_scene);
static void read_default_scene(struct gltf_reader* p_reader);

static void* load_uri_payload(struct gltf_reader* p_reader, const char* s_uri, size_t* p_bytes_len);

static void matrix_multiply(const float* p_m1, const float* p_m2, float* p_result);
static void matrix_make_rotation(const float* p_quaternion_xyzw, float* p_result);
static void matrix_make_scale(const float* p_scale_xyz, float* p_result);
static void matrix_make_translation(const float* p_translation_xyz, float* p_result);

int gltf_load_from_file(const char* s_filename, struct gltf* p_result) {
    *p_result = (struct gltf){0};

    struct gltf_reader reader = {
        .s_filename = s_filename,
        .p_result = p_result,
        .error = GLTF_SUCCESS
    };

    FILE* p_file = fopen(s_filename, "rb");
    if (!p_file) {
        cx_log_fmt(CX_LOG_ERROR, LOG_CAT_GLTF, "Failed to open glTF file '%s'\n", s_filename);
        return GLTF_ERROR_FILE;
    }

    cx_log_fmt(CX_LOG_INFO, LOG_CAT_GLTF, "Loading glTF file '%s'...\n", s_filename);

    copy_file_directory(s_filename, reader.s_filedir);

    int b_is_glb = 0;
    read_is_file_glb(p_file, &b_is_glb);

    reader.error = b_is_glb ? read_glb_file(p_file, &reader) : read_json_file(p_file, &reader);

    fclose(p_file);

#define READ_GLTF_OBJECT_ARRAY(KEY, NAME, PROC) (read_gltf_object_array(&reader, KEY, (void**)&p_result->p_##NAME, &p_result->num_##NAME, sizeof(*p_result->p_##NAME), PROC), reader.error == GLTF_SUCCESS)

    if (reader.error == GLTF_SUCCESS) {
        (void)(
            (read_gltf_asset(&reader), reader.error == GLTF_SUCCESS) &&
            (read_gltf_extensions(&reader), reader.error == GLTF_SUCCESS) &&
            READ_GLTF_OBJECT_ARRAY("buffers", buffers, read_buffer) &&
            READ_GLTF_OBJECT_ARRAY("bufferViews", buffer_views, read_buffer_view) &&
            READ_GLTF_OBJECT_ARRAY("accessors", accessors, read_accessor) &&
            READ_GLTF_OBJECT_ARRAY("images", images, read_image) &&
            READ_GLTF_OBJECT_ARRAY("textures", textures, read_texture) &&
            READ_GLTF_OBJECT_ARRAY("materials", materials, read_material) &&
            READ_GLTF_OBJECT_ARRAY("meshes", meshes, read_mesh) &&
            READ_GLTF_OBJECT_ARRAY("nodes", nodes, read_node) &&
            READ_GLTF_OBJECT_ARRAY("skins", skins, read_skin) &&
            READ_GLTF_OBJECT_ARRAY("animations", animations, read_animation) &&
            READ_GLTF_OBJECT_ARRAY("scenes", scenes, read_scene) &&
            (read_default_scene(&reader), reader.error == GLTF_SUCCESS)
        );
        
        json_free(reader.p_json_gltf);
    }

    if (reader.error != GLTF_SUCCESS) {
        free(reader.p_glb_buffer_chunk.p_bytes);
        reader.p_glb_buffer_chunk = (struct glb_chunk){0};
        gltf_free(reader.p_result);
    }

    return reader.error;
}

void gltf_free(struct gltf* p_gltf) {
    free(p_gltf->info.s_version);
    free(p_gltf->info.s_version_min);
    free(p_gltf->info.s_generator);
    free(p_gltf->info.s_copyright);

    for (size_t i = 0; i < p_gltf->info.num_extensions; ++i) {
        free(p_gltf->info.p_extensions[i]);
    }

    for (size_t i = 0; i < p_gltf->num_buffers; ++i) {
        free(p_gltf->p_buffers[i].p_bytes);
    }

    for (size_t i = 0; i < p_gltf->num_images; ++i) {
        if (p_gltf->p_images[i].b_uri_source) {
            free(p_gltf->p_images[i].source.uri.p_data);
        }
    }

    for (size_t i = 0; i < p_gltf->num_meshes; ++i) {
        free(p_gltf->p_meshes[i].p_primitives);
    }

    for (size_t i = 0; i < p_gltf->num_skins; ++i) {
        free(p_gltf->p_skins[i].p_joints_indices);
    }

    for (size_t i = 0; i < p_gltf->num_animations; ++i) {
        free(p_gltf->p_animations[i].p_channels);
        free(p_gltf->p_animations[i].p_samplers);
    }

    for (size_t i = 0; i < p_gltf->num_nodes; ++i) {
        free(p_gltf->p_nodes[i].p_children_indices);
    }

    for (size_t i = 0; i < p_gltf->num_scenes; ++i) {
        free(p_gltf->p_scenes[i].p_root_nodes_indices);
    }

    free(p_gltf->p_buffers);
    free(p_gltf->p_buffer_views);
    free(p_gltf->p_accessors);
    free(p_gltf->p_images);
    free(p_gltf->p_textures);
    free(p_gltf->p_materials);
    free(p_gltf->p_skins);
    free(p_gltf->p_animations);
    free(p_gltf->p_nodes);
    free(p_gltf->p_scenes);

    *p_gltf = (struct gltf){0};
}

void copy_file_directory(const char* s_filename, char* s_dst) {
    const char* p_last_sep = 0;

    for (const char* p = s_filename; *p; ++p) {
        if (*p == '/' || *p == '\\') {
            p_last_sep = p;
        }
    }

    if (!p_last_sep) {
        s_dst[0] = '\0';
        return;
    }

    strncpy(s_dst, s_filename, p_last_sep - s_filename + 1);
    s_dst[p_last_sep - s_filename + 1] = '\0';
}

void make_file_path(const char* s_dirname, const char* s_filename, char* s_dst) {
    size_t dirname_len = strlen(s_dirname);
    strncpy(s_dst, s_dirname, dirname_len);
    strcpy(s_dst + dirname_len, s_filename);
}

int read_is_file_glb(FILE* p_file, int* p_b_result) {
    uint32_t header[3];
    if (!fread(header, sizeof(header), 1, p_file)) {
        return GLTF_ERROR_FILE;
    }

    *p_b_result = header[0] == 0x46546C67;
    fseek(p_file, 0, SEEK_SET);
    return GLTF_SUCCESS;
}

int read_glb_file(FILE* p_file, struct gltf_reader* p_reader) {
    uint32_t header[3];
    if (!fread(header, sizeof(header), 1, p_file)) {
        return GLTF_ERROR_FILE;
    }

    if (header[0] != 0x46546C67) {
        return GLTF_ERROR_FILE;
    }

    struct glb_chunk json_chunk = {0};

    int error;
    
    error = read_glb_file_chunk(p_file, &json_chunk);
    if (error != GLTF_SUCCESS) {
        return error;
    }

    error = read_glb_file_chunk(p_file, &p_reader->p_glb_buffer_chunk);
    if (error != GLTF_SUCCESS) {
        free(json_chunk.p_bytes);
        return error;
    }

    if (!json_parse(json_chunk.p_bytes, json_chunk.length, &p_reader->p_json_gltf)) {
        free(json_chunk.p_bytes);
        free(p_reader->p_glb_buffer_chunk.p_bytes);
        return GLTF_ERROR_JSON_PARSING;
    }

    return GLTF_SUCCESS;
}

int read_glb_file_chunk(FILE* p_file, struct glb_chunk* p_result) {
    uint32_t chunk_header[2];
    if (!fread(chunk_header, sizeof(chunk_header), 1, p_file)) {
        return GLTF_ERROR_FILE;
    }

    p_result->length = chunk_header[0];
    p_result->type = chunk_header[1];

    p_result->p_bytes = malloc(p_result->length);
    if (!fread(p_result->p_bytes, p_result->length, 1, p_file)) {
        return GLTF_ERROR_FILE;
    }

    return GLTF_SUCCESS;
}

int read_json_file(FILE* p_file, struct gltf_reader* p_reader) {
    fseek(p_file, 0, SEEK_END);
    long file_size = ftell(p_file);
    fseek(p_file, 0, SEEK_SET);

    char* p_data = malloc(file_size + 1);
    fread(p_data, file_size, 1, p_file);
    p_data[file_size] = 0;
    
    int b_success = json_parse(p_data, file_size, &p_reader->p_json_gltf);

    free(p_data);

    return b_success ? GLTF_SUCCESS : GLTF_ERROR_JSON_PARSING;
}

void read_gltf_asset(struct gltf_reader* p_reader) {
    const struct json_value* p_json_asset = json_object_get(p_reader->p_json_gltf, "asset");
    const struct json_value* p_json_asset_version = json_object_get(p_json_asset, "version");
    const struct json_value* p_json_asset_min_version = json_object_get(p_json_asset, "minVersion");
    const struct json_value* p_json_asset_generator = json_object_get(p_json_asset, "generator");
    const struct json_value* p_json_asset_copyright = json_object_get(p_json_asset, "copyright");

    if (p_json_asset_version){
        cx_log_fmt(CX_LOG_TRACE, LOG_CAT_GLTF, "glTF specification version: %s\n", json_string(p_json_asset_version));
    }

    if (p_json_asset_min_version){
        cx_log_fmt(CX_LOG_TRACE, LOG_CAT_GLTF, "glTF min specification version: %s\n", json_string(p_json_asset_min_version));
    }

    if (p_json_asset_generator){
        cx_log_fmt(CX_LOG_TRACE, LOG_CAT_GLTF, "Asset generator: %s\n", json_string(p_json_asset_generator));
    }

    if (p_json_asset_copyright) {
        cx_log_fmt(CX_LOG_TRACE, LOG_CAT_GLTF, "Asset copyright notice: %s\n", json_string(p_json_asset_copyright));
    }
}

void read_gltf_extensions(struct gltf_reader* p_reader) {
    const struct json_value* p_json_ext_used = json_object_get(p_reader->p_json_gltf, "extensionsUsed");

    if (!p_json_ext_used) {
        return;
    }

    const size_t n_extensions_used = json_array_len(p_json_ext_used);

    if (n_extensions_used == 0) {
        cx_log(CX_LOG_TRACE, LOG_CAT_GLTF, "Extensions used: none\n");
        return;
    }

    cx_log_fmt(CX_LOG_TRACE, LOG_CAT_GLTF, "Extensions used:\n");
    for (size_t i = 0; i < json_array_len(p_json_ext_used); ++i) {
        const struct json_value* p_json_ext_str = json_array_get(p_json_ext_used, i);
        cx_log_fmt(CX_LOG_TRACE, LOG_CAT_GLTF, "  '%s'\n", json_string(p_json_ext_str));
    }

    const struct json_value* p_json_ext_required = json_object_get(p_reader->p_json_gltf, "extensionsRequired");

    if (!p_json_ext_required) {
        return;
    }

    const size_t n_extensions_required = json_array_len(p_json_ext_required);

    if (n_extensions_required == 0) {
        cx_log(CX_LOG_TRACE, LOG_CAT_GLTF, "Extensions required: none\n");
        return;
    }

    cx_log(CX_LOG_TRACE, LOG_CAT_GLTF, "Extensions required:\n");
    for (size_t i = 0; i < json_array_len(p_json_ext_required); ++i) {
        const struct json_value* p_json_ext_str = json_array_get(p_json_ext_required, i);
        cx_log_fmt(CX_LOG_TRACE, LOG_CAT_GLTF, "  '%s'\n", json_string(p_json_ext_str));
    }
}

void read_gltf_object_array(struct gltf_reader* p_reader, const char* s_array_key, void** p_elements, size_t* n_elements, size_t element_size, read_element_proc read_element) {
    const struct json_value* p_json_array = json_object_get(p_reader->p_json_gltf, s_array_key);

    if (!p_json_array) {
        return;
    }

    if (!json_is_array(p_json_array)) {
        p_reader->error = GLTF_ERROR_BAD_TYPE;
        return;
    }

    *n_elements = json_array_len(p_json_array);
    *p_elements = calloc(*n_elements, element_size);

    if (!*p_elements) {
        p_reader->error = GLTF_ERROR_MEMORY;
        return;
    }

    const struct json_value* p_json_element;
    void* p_element;
    
    for (size_t i = 0; i < *n_elements; ++i) {
        p_json_element = json_array_get(p_json_array, i);

        if (!json_is_object(p_json_element)) {
            p_reader->error = GLTF_ERROR_BAD_TYPE;
            return;
        }

        p_element = ((unsigned char*)(*p_elements)) + (element_size * i);

        read_element(p_reader, p_json_element, p_element);

        if (p_reader->error != GLTF_SUCCESS){
            return;
        }
    }

    cx_log_fmt(CX_LOG_TRACE, LOG_CAT_GLTF, "%s count: %llu\n", s_array_key, *n_elements);
}

void read_gltf_object_property(struct gltf_reader* p_reader, const struct json_value* p_json_gltf_object, const char* s_key, int b_required, int(*type_checker)(const struct json_value*), const struct json_value** p_result) {
    *p_result = json_object_get(p_json_gltf_object, s_key);
    if (b_required && !*p_result) {
        p_reader->error = GLTF_ERROR_OBJECT_MISSING_REQUIRED_PROPERTY;
        return;
    }

    if (!p_result){
        return;
    }

    if (type_checker(*p_result)){
        return;
    }

    if (!b_required) {
        p_result = 0;
        return;
    }

    p_reader->error =  GLTF_ERROR_BAD_TYPE;
}

void read_gltf_number_array(struct gltf_reader* p_reader, const struct json_value* p_json_gltf_number_array, size_t count, float* p_result) {
    if (json_array_len(p_json_gltf_number_array) < count) {
        p_reader->error = GLTF_ERROR_OBJECT_MISSING_REQUIRED_PROPERTY;
        return;
    }

    const struct json_value* p_json_number;
    for (size_t i = 0; i < count; ++i) {
        p_json_number = json_array_get(p_json_gltf_number_array, i);

        if (!json_is_number(p_json_number)) {
            p_reader->error = GLTF_ERROR_BAD_TYPE;
            return;
        }

        p_result[i] = json_number(p_json_number);
    }
}

void read_gltf_texture_info(struct gltf_reader* p_reader, const struct json_value* p_json_texture_info, struct gltf_material_texture_info* p_texture_info) {
    const struct json_value* p_json_texture_info_texture;
    const struct json_value* p_json_texture_info_tex_coord;
    
    read_gltf_object_property(p_reader, p_json_texture_info, "index", 1, json_is_number, &p_json_texture_info_texture);
    read_gltf_object_property(p_reader, p_json_texture_info, "texCoord", 0, json_is_number, &p_json_texture_info_tex_coord);
    
    if (p_reader->error != GLTF_SUCCESS) {
        return;
    }

    p_texture_info->source_texture_index = json_number(p_json_texture_info_texture);

    if (p_json_texture_info_tex_coord) {
        p_texture_info->tex_coord_set_index = json_number(p_json_texture_info_tex_coord);
    }
}

void read_buffer(struct gltf_reader* p_reader, const struct json_value* p_json_buffer, void* p_buffer) {
    struct gltf_buffer* p_buffer_ = p_buffer;

    const struct json_value* p_json_buffer_byte_length;
    read_gltf_object_property(p_reader, p_json_buffer, "byteLength", 1, json_is_number, &p_json_buffer_byte_length);

    if (p_reader->error != GLTF_SUCCESS) {
        return;
    }

    p_buffer_->byte_length = json_number(p_json_buffer_byte_length);

    if (p_reader->p_glb_buffer_chunk.p_bytes) {
        p_buffer_->p_bytes = p_reader->p_glb_buffer_chunk.p_bytes;
        p_reader->p_glb_buffer_chunk = (struct glb_chunk){0};
        return;
    }

    const struct json_value* p_json_buffer_uri;
    read_gltf_object_property(p_reader, p_json_buffer, "uri", 1, json_is_string, &p_json_buffer_uri);

    if (p_reader->error != GLTF_SUCCESS) {
        return;
    }

    size_t buffer_len = 0;
    p_buffer_->p_bytes = load_uri_payload(p_reader, json_string(p_json_buffer_uri), &buffer_len);
}

void read_buffer_view(struct gltf_reader* p_reader, const struct json_value* p_json_buffer_view, void* p_buffer_view) {
    struct gltf_buffer_view* p_buffer_view_ = p_buffer_view;

    const struct json_value* p_json_buffer_view_buffer;
    const struct json_value* p_json_buffer_view_byte_offset;
    const struct json_value* p_json_buffer_view_byte_length;
    const struct json_value* p_json_buffer_view_byte_stride;
    const struct json_value* p_json_buffer_view_byte_target;

    read_gltf_object_property(p_reader, p_json_buffer_view, "buffer", 1, json_is_number, &p_json_buffer_view_buffer);
    read_gltf_object_property(p_reader, p_json_buffer_view, "byteOffset", 0, json_is_number, &p_json_buffer_view_byte_offset);
    read_gltf_object_property(p_reader, p_json_buffer_view, "byteLength", 1, json_is_number, &p_json_buffer_view_byte_length);
    read_gltf_object_property(p_reader, p_json_buffer_view, "byteStride", 0, json_is_number, &p_json_buffer_view_byte_stride);
    read_gltf_object_property(p_reader, p_json_buffer_view, "target", 0, json_is_number, &p_json_buffer_view_byte_target);

    if (p_reader->error != GLTF_SUCCESS) {
        return;
    }

    p_buffer_view_->buffer_index = json_number(p_json_buffer_view_buffer);

    if (p_json_buffer_view_byte_offset) {
        p_buffer_view_->byte_offset = json_number(p_json_buffer_view_byte_offset);
    }
    
    p_buffer_view_->byte_length = json_number(p_json_buffer_view_byte_length);

    if (p_json_buffer_view_byte_stride) {
        p_buffer_view_->byte_stride = json_number(p_json_buffer_view_byte_stride);
    }

    if (p_json_buffer_view_byte_target) {
        p_buffer_view_->target = (enum gltf_buffer_view_target)json_number(p_json_buffer_view_byte_target);
    }
}

void read_accessor(struct gltf_reader* p_reader, const struct json_value* p_json_accessor, void* p_accessor) {
    struct gltf_accessor* p_accessor_ = p_accessor;

    const struct json_value* p_json_accessor_buffer_view;
    const struct json_value* p_json_accessor_byte_offset;
    const struct json_value* p_json_accessor_component_type;
    const struct json_value* p_json_accessor_normalized;
    const struct json_value* p_json_accessor_count;
    const struct json_value* p_json_accessor_type;
    const struct json_value* p_json_accessor_max;
    const struct json_value* p_json_accessor_min;
    
    read_gltf_object_property(p_reader, p_json_accessor, "bufferView", 0, json_is_number, &p_json_accessor_buffer_view);
    read_gltf_object_property(p_reader, p_json_accessor, "byteOffset", 0, json_is_number, &p_json_accessor_byte_offset);
    read_gltf_object_property(p_reader, p_json_accessor, "componentType", 1, json_is_number, &p_json_accessor_component_type);
    read_gltf_object_property(p_reader, p_json_accessor, "normalized", 0, json_is_bool, &p_json_accessor_normalized);
    read_gltf_object_property(p_reader, p_json_accessor, "count", 1, json_is_number, &p_json_accessor_count);
    read_gltf_object_property(p_reader, p_json_accessor, "type", 1, json_is_string, &p_json_accessor_type);
    read_gltf_object_property(p_reader, p_json_accessor, "max", 0, json_is_array, &p_json_accessor_max);
    read_gltf_object_property(p_reader, p_json_accessor, "min", 0, json_is_array, &p_json_accessor_min);

    if (p_reader->error != GLTF_SUCCESS)
        return;

    if (p_json_accessor_buffer_view) {
        p_accessor_->buffer_view_index = json_number(p_json_accessor_buffer_view);
    } else {
        p_accessor_->buffer_view_index = GLTF_INVALID_INDEX;
    }

    if (p_json_accessor_byte_offset) {
        p_accessor_->byte_offset = json_number(p_json_accessor_byte_offset);
    }

    p_accessor_->component_type = json_number(p_json_accessor_component_type);

    if (p_json_accessor_normalized) {
        p_accessor_->byte_offset = json_bool(p_json_accessor_normalized);
    }

    p_accessor_->count = json_number(p_json_accessor_count);

    size_t num_components = 0;
    const char* s_type = json_string(p_json_accessor_type);
    if (strcmp(s_type, "SCALAR") == 0) {
        p_accessor_->type = GLTF_ACCESSOR_TYPE_scalar;
        num_components = 1;
    } else if (strcmp(s_type, "VEC2") == 0) {
        p_accessor_->type = GLTF_ACCESSOR_TYPE_vec2;
        num_components = 2;
    } else if (strcmp(s_type, "VEC3") == 0) {
        p_accessor_->type = GLTF_ACCESSOR_TYPE_vec3;
        num_components = 3;
    } else if (strcmp(s_type, "VEC4") == 0) {
        p_accessor_->type = GLTF_ACCESSOR_TYPE_vec4;
        num_components = 4;
    } else if (strcmp(s_type, "MAT2") == 0) {
        p_accessor_->type = GLTF_ACCESSOR_TYPE_mat2;
        num_components = 4;
    } else if (strcmp(s_type, "MAT3") == 0) {
        p_accessor_->type = GLTF_ACCESSOR_TYPE_mat3;
        num_components = 9;
    } else if (strcmp(s_type, "MAT4") == 0) {
        p_accessor_->type = GLTF_ACCESSOR_TYPE_mat4;
        num_components = 16;
    }

    if(p_json_accessor_max) {
        read_gltf_number_array(p_reader, p_json_accessor_max, num_components, p_accessor_->max);
        if (p_reader->error != GLTF_SUCCESS)
            return;
    }
    
    if(p_json_accessor_min) {
        read_gltf_number_array(p_reader, p_json_accessor_min, num_components, p_accessor_->min);
        if (p_reader->error != GLTF_SUCCESS)
            return;
    }
}

void read_image(struct gltf_reader* p_reader, const struct json_value* p_json_image, void* p_image) {
    struct gltf_image* p_image_ = p_image;

    const struct json_value* p_json_image_uri;
    const struct json_value* p_json_image_mime_type;
    const struct json_value* p_json_image_buffer_view;
    
    read_gltf_object_property(p_reader, p_json_image, "uri", 0, json_is_string, &p_json_image_uri);
    read_gltf_object_property(p_reader, p_json_image, "mimeType", 0, json_is_string, &p_json_image_mime_type);
    read_gltf_object_property(p_reader, p_json_image, "bufferView", 0, json_is_number, &p_json_image_buffer_view);

    if (p_reader->error != GLTF_SUCCESS) {
        return;
    }

    if (p_json_image_uri && p_json_image_buffer_view) {
        p_reader->error = GLTF_ERROR_UNKNOWN;
        return;
    }

    if (p_json_image_buffer_view && !p_json_image_mime_type) {
        p_reader->error = GLTF_ERROR_UNKNOWN;
        return;
    }

    if (p_json_image_mime_type) {
        strcpy(p_image_->s_mime_type, json_string(p_json_image_mime_type));
    }

    if (p_json_image_uri) {
        p_image_->b_uri_source = 1;
        const char* s_uri = json_string(p_json_image_uri);
        p_image_->source.uri.p_data = load_uri_payload(p_reader, s_uri, &p_image_->source.uri.size);
        return;
    }

    if (!p_json_image_buffer_view) {
        p_image_->source.buffer_view_index = GLTF_INVALID_INDEX;
        p_reader->error = GLTF_ERROR_UNKNOWN;
        return;
    }

    p_image_->source.buffer_view_index = json_number(p_json_image_buffer_view);
}

void read_texture(struct gltf_reader* p_reader, const struct json_value* p_json_texture, void* p_texture) {
    struct gltf_texture* p_texture_ = p_texture;
    *p_texture_ = (struct gltf_texture) {
        .sampler_wrap_s = GLTF_SAMPLER_WRAP_repeat,
        .sampler_wrap_t = GLTF_SAMPLER_WRAP_repeat
    };

    const struct json_value* p_json_texture_sampler;
    const struct json_value* p_json_texture_source;
    
    read_gltf_object_property(p_reader, p_json_texture, "sampler", 0, json_is_number, &p_json_texture_sampler);
    read_gltf_object_property(p_reader, p_json_texture, "source", 1, json_is_number, &p_json_texture_source);

    if (p_reader->error != GLTF_SUCCESS) {
        return;
    }

    if (p_json_texture_sampler) {
        const size_t sampler_index = json_number(p_json_texture_sampler);
        const struct json_value* p_json_samplers = json_object_get(p_reader->p_json_gltf, "samplers");
        const struct json_value* p_json_sampler = json_array_get(p_json_samplers, sampler_index);

        if (p_json_sampler) {
            const struct json_value* p_json_sampler_mag_filter;
            const struct json_value* p_json_sampler_min_filter;
            const struct json_value* p_json_sampler_wrap_s;
            const struct json_value* p_json_sampler_wrap_t;
            
            read_gltf_object_property(p_reader, p_json_sampler, "magFilter", 0, json_is_number, &p_json_sampler_mag_filter);
            read_gltf_object_property(p_reader, p_json_sampler, "minFilter", 0, json_is_number, &p_json_sampler_min_filter);
            read_gltf_object_property(p_reader, p_json_sampler, "wrapS", 0, json_is_number, &p_json_sampler_wrap_s);
            read_gltf_object_property(p_reader, p_json_sampler, "wrapT", 0, json_is_number, &p_json_sampler_wrap_t);

            if (p_json_sampler_mag_filter)
                p_texture_->sampler_mag_filter = json_number(p_json_sampler_mag_filter);
                
            if (p_json_sampler_min_filter)
                p_texture_->sampler_min_filter = json_number(p_json_sampler_min_filter);

            p_texture_->sampler_wrap_s = p_json_sampler_wrap_s ? json_number(p_json_sampler_wrap_s) : GLTF_SAMPLER_WRAP_repeat;
            p_texture_->sampler_wrap_t = p_json_sampler_wrap_t ? json_number(p_json_sampler_wrap_t) : GLTF_SAMPLER_WRAP_repeat;
        }
    }

    p_texture_->source_image_index = json_number(p_json_texture_source);
}

void read_material(struct gltf_reader* p_reader, const struct json_value* p_json_material, void* p_material) {
    struct gltf_material* p_material_ = p_material;
    *p_material_ = (struct gltf_material) {
        .pbr_base_color_factor = { 1, 1, 1, 1 },
        .pbr_metallic_factor = 1,
        .pbr_roughness_factor = 1,
        .alpha_mode = GLTF_MATERIAL_ALPHA_MODE_opaque,
        .mask_alpha_cutoff = 0.5f,
    };

    const struct json_value* p_json_material_pbr_metallic_roughness;
    const struct json_value* p_json_material_normal_texture;
    const struct json_value* p_json_material_occlusion_texture;
    const struct json_value* p_json_material_emissive_texture;
    const struct json_value* p_json_material_emissive_factor;
    const struct json_value* p_json_material_alpha_mode;
    const struct json_value* p_json_material_alpha_cutoff;
    const struct json_value* p_json_material_alpha_double_sided;
    
    read_gltf_object_property(p_reader, p_json_material, "pbrMetallicRoughness", 0, json_is_object, &p_json_material_pbr_metallic_roughness);
    read_gltf_object_property(p_reader, p_json_material, "normalTexture", 0, json_is_object, &p_json_material_normal_texture);
    read_gltf_object_property(p_reader, p_json_material, "occlusionTexture", 0, json_is_object, &p_json_material_occlusion_texture);
    read_gltf_object_property(p_reader, p_json_material, "emissiveTexture", 0, json_is_object, &p_json_material_emissive_texture);
    read_gltf_object_property(p_reader, p_json_material, "emissiveFactor", 0, json_is_array, &p_json_material_emissive_factor);
    read_gltf_object_property(p_reader, p_json_material, "alphaMode", 0, json_is_string, &p_json_material_alpha_mode);
    read_gltf_object_property(p_reader, p_json_material, "alphaCutoff", 0, json_is_number, &p_json_material_alpha_cutoff);
    read_gltf_object_property(p_reader, p_json_material, "doubleSided", 0, json_is_bool, &p_json_material_alpha_double_sided);

    if (p_json_material_pbr_metallic_roughness) {
        const struct json_value* p_json_pbr_metallic_roughness_base_color_factor;
        const struct json_value* p_json_pbr_metallic_roughness_base_color_texture;
        const struct json_value* p_json_pbr_metallic_roughness_metallic_factor;
        const struct json_value* p_json_pbr_metallic_roughness_roughness_factor;
        const struct json_value* p_json_pbr_metallic_roughness_metallic_roughness_texture;

        read_gltf_object_property(p_reader, p_json_material_pbr_metallic_roughness, "baseColorFactor", 0, json_is_array, &p_json_pbr_metallic_roughness_base_color_factor);
        read_gltf_object_property(p_reader, p_json_material_pbr_metallic_roughness, "baseColorTexture", 0, json_is_object, &p_json_pbr_metallic_roughness_base_color_texture);
        read_gltf_object_property(p_reader, p_json_material_pbr_metallic_roughness, "metallicFactor", 0, json_is_number, &p_json_pbr_metallic_roughness_metallic_factor);
        read_gltf_object_property(p_reader, p_json_material_pbr_metallic_roughness, "roughnessFactor", 0, json_is_number, &p_json_pbr_metallic_roughness_roughness_factor);
        read_gltf_object_property(p_reader, p_json_material_pbr_metallic_roughness, "metallicRoughnessTexture", 0, json_is_object, &p_json_pbr_metallic_roughness_metallic_roughness_texture);

        if (p_json_pbr_metallic_roughness_base_color_factor) {
            read_gltf_number_array(p_reader, p_json_pbr_metallic_roughness_base_color_factor, 4, p_material_->pbr_base_color_factor);
            if (p_reader->error != GLTF_SUCCESS)
                return;
        }

        if (p_json_pbr_metallic_roughness_base_color_texture) {
            read_gltf_texture_info(p_reader, p_json_pbr_metallic_roughness_base_color_texture, &p_material_->pbr_base_color_texture);
            if (p_reader->error != GLTF_SUCCESS)
                return;
        }

        if (p_json_pbr_metallic_roughness_metallic_factor) {
            p_material_->pbr_metallic_factor = json_number(p_json_pbr_metallic_roughness_metallic_factor);
        }

        if (p_json_pbr_metallic_roughness_roughness_factor) {
            p_material_->pbr_roughness_factor = json_number(p_json_pbr_metallic_roughness_roughness_factor);
        }

        if (p_json_pbr_metallic_roughness_metallic_roughness_texture) {
            read_gltf_texture_info(p_reader, p_json_pbr_metallic_roughness_metallic_roughness_texture, &p_material_->pbr_metallic_roughness_texture);
            if (p_reader->error != GLTF_SUCCESS)
                return;
        }
    }

    if (p_json_material_normal_texture) {
        read_gltf_texture_info(p_reader, p_json_material_normal_texture, &p_material_->normal_texture);
    }

    if (p_json_material_occlusion_texture) {
        read_gltf_texture_info(p_reader, p_json_material_occlusion_texture, &p_material_->occlusion_texture);
    }

    if (p_json_material_emissive_texture) {
        read_gltf_texture_info(p_reader, p_json_material_emissive_texture, &p_material_->emissive_texture);
    }

    if (p_reader->error != GLTF_SUCCESS) {
        return;
    }
    
    if(p_json_material_emissive_factor) {
        read_gltf_number_array(p_reader, p_json_material_emissive_factor, 3, p_material_->emissive_factor);
        if (p_reader->error != GLTF_SUCCESS) {
            return;
        }
    }

    if (p_json_material_alpha_mode) {
        const char* s_alpha_mode = json_string(p_json_material_alpha_mode);
        if (strcmp(s_alpha_mode, "MASK") == 0) {
            p_material_->alpha_mode = GLTF_MATERIAL_ALPHA_MODE_mask;
        } else if (strcmp(s_alpha_mode, "BLEND") == 0) {
            p_material_->alpha_mode = GLTF_MATERIAL_ALPHA_MODE_blend;
        }
    }

    if (p_json_material_alpha_cutoff) {
        json_number(p_json_material_alpha_cutoff);
    }

    if (p_json_material_alpha_double_sided) {
        p_material_->b_double_sided = json_bool(p_json_material_alpha_double_sided);
    }
}

void read_mesh(struct gltf_reader* p_reader, const struct json_value* p_json_mesh, void* p_mesh) {
    struct gltf_mesh* p_mesh_ = p_mesh;

    const struct json_value* p_json_mesh_primitives;

    read_gltf_object_property(p_reader, p_json_mesh, "primitives", 1, json_is_array, &p_json_mesh_primitives);

    if (p_reader->error != GLTF_SUCCESS) {
        return;
    }

    p_mesh_->num_primitives = json_array_len(p_json_mesh_primitives);
    p_mesh_->p_primitives = calloc(p_mesh_->num_primitives, sizeof(*p_mesh_->p_primitives));

    for (size_t i = 0; i < p_mesh_->num_primitives; ++i) {
        const struct json_value* p_json_mesh_primitive = json_array_get(p_json_mesh_primitives, i);
        
        if (!json_is_object(p_json_mesh_primitive)) {
            p_reader->error = GLTF_ERROR_BAD_TYPE;
            return;
        }

        const struct json_value* p_json_mesh_primitive_attributes;
        const struct json_value* p_json_mesh_primitive_indices;
        const struct json_value* p_json_mesh_primitive_material;
        const struct json_value* p_json_mesh_primitive_mode;

        read_gltf_object_property(p_reader, p_json_mesh_primitive, "attributes", 1, json_is_object, &p_json_mesh_primitive_attributes);
        read_gltf_object_property(p_reader, p_json_mesh_primitive, "indices", 0, json_is_number, &p_json_mesh_primitive_indices);
        read_gltf_object_property(p_reader, p_json_mesh_primitive, "material", 0, json_is_number, &p_json_mesh_primitive_material);
        read_gltf_object_property(p_reader, p_json_mesh_primitive, "mode", 0, json_is_number, &p_json_mesh_primitive_mode);

        if (p_reader->error != GLTF_SUCCESS) {
            return;
        }

        struct gltf_mesh_primitive* p_mesh_primitive = &p_mesh_->p_primitives[i];
        *p_mesh_primitive = (struct gltf_mesh_primitive) {
            .mode = GLTF_MESH_PRIMITIVE_MODE_triangles
        };

        const struct json_value* p_json_mesh_primitive_attributes_position;
        const struct json_value* p_json_mesh_primitive_attributes_normal;
        const struct json_value* p_json_mesh_primitive_attributes_tangent;
        const struct json_value* p_json_mesh_primitive_attributes_texcoord_0;
        const struct json_value* p_json_mesh_primitive_attributes_texcoord_1;
        const struct json_value* p_json_mesh_primitive_attributes_color;
        const struct json_value* p_json_mesh_primitive_attributes_joints;
        const struct json_value* p_json_mesh_primitive_attributes_weights;

        read_gltf_object_property(p_reader, p_json_mesh_primitive_attributes, "POSITION", 0, json_is_number, &p_json_mesh_primitive_attributes_position);
        read_gltf_object_property(p_reader, p_json_mesh_primitive_attributes, "NORMAL", 0, json_is_number, &p_json_mesh_primitive_attributes_normal);
        read_gltf_object_property(p_reader, p_json_mesh_primitive_attributes, "TANGENT", 0, json_is_number, &p_json_mesh_primitive_attributes_tangent);
        read_gltf_object_property(p_reader, p_json_mesh_primitive_attributes, "TEXCOORD_0", 0, json_is_number, &p_json_mesh_primitive_attributes_texcoord_0);
        read_gltf_object_property(p_reader, p_json_mesh_primitive_attributes, "TEXCOORD_1", 0, json_is_number, &p_json_mesh_primitive_attributes_texcoord_1);
        read_gltf_object_property(p_reader, p_json_mesh_primitive_attributes, "COLOR_0", 0, json_is_number, &p_json_mesh_primitive_attributes_color);
        read_gltf_object_property(p_reader, p_json_mesh_primitive_attributes, "JOINTS_0", 0, json_is_number, &p_json_mesh_primitive_attributes_joints);
        read_gltf_object_property(p_reader, p_json_mesh_primitive_attributes, "WEIGHTS_0", 0, json_is_number, &p_json_mesh_primitive_attributes_weights);

        if (p_json_mesh_primitive_attributes_position) {
            p_mesh_primitive->vertex_attribute_accessors_indices[GLTF_MESH_VERTEX_ATTRIBUTE_position] = json_number(p_json_mesh_primitive_attributes_position);
        } else {
            p_mesh_primitive->vertex_attribute_accessors_indices[GLTF_MESH_VERTEX_ATTRIBUTE_position] = GLTF_INVALID_INDEX;
        }
        
        if (p_json_mesh_primitive_attributes_normal) {
            p_mesh_primitive->vertex_attribute_accessors_indices[GLTF_MESH_VERTEX_ATTRIBUTE_normal] = json_number(p_json_mesh_primitive_attributes_normal);
        } else {
            p_mesh_primitive->vertex_attribute_accessors_indices[GLTF_MESH_VERTEX_ATTRIBUTE_normal] = GLTF_INVALID_INDEX;
        }
        
        if (p_json_mesh_primitive_attributes_tangent) {
            p_mesh_primitive->vertex_attribute_accessors_indices[GLTF_MESH_VERTEX_ATTRIBUTE_tangent] = json_number(p_json_mesh_primitive_attributes_tangent);
        } else {
            p_mesh_primitive->vertex_attribute_accessors_indices[GLTF_MESH_VERTEX_ATTRIBUTE_tangent] = GLTF_INVALID_INDEX;
        }
        
        if (p_json_mesh_primitive_attributes_texcoord_0) {
            p_mesh_primitive->vertex_attribute_accessors_indices[GLTF_MESH_VERTEX_ATTRIBUTE_texcoord_0] = json_number(p_json_mesh_primitive_attributes_texcoord_0);
        } else {
            p_mesh_primitive->vertex_attribute_accessors_indices[GLTF_MESH_VERTEX_ATTRIBUTE_texcoord_0] = GLTF_INVALID_INDEX;
        }
        
        if (p_json_mesh_primitive_attributes_texcoord_1) {
            p_mesh_primitive->vertex_attribute_accessors_indices[GLTF_MESH_VERTEX_ATTRIBUTE_texcoord_1] = json_number(p_json_mesh_primitive_attributes_texcoord_1);
        } else {
            p_mesh_primitive->vertex_attribute_accessors_indices[GLTF_MESH_VERTEX_ATTRIBUTE_texcoord_1] = GLTF_INVALID_INDEX;
        }
        
        if (p_json_mesh_primitive_attributes_color) {
            p_mesh_primitive->vertex_attribute_accessors_indices[GLTF_MESH_VERTEX_ATTRIBUTE_color] = json_number(p_json_mesh_primitive_attributes_color);
        } else {
            p_mesh_primitive->vertex_attribute_accessors_indices[GLTF_MESH_VERTEX_ATTRIBUTE_color] = GLTF_INVALID_INDEX;
        }
        
        if (p_json_mesh_primitive_attributes_joints) {
            p_mesh_primitive->vertex_attribute_accessors_indices[GLTF_MESH_VERTEX_ATTRIBUTE_joints] = json_number(p_json_mesh_primitive_attributes_joints);
        } else {
            p_mesh_primitive->vertex_attribute_accessors_indices[GLTF_MESH_VERTEX_ATTRIBUTE_joints] = GLTF_INVALID_INDEX;
        }
        
        if (p_json_mesh_primitive_attributes_weights) {
            p_mesh_primitive->vertex_attribute_accessors_indices[GLTF_MESH_VERTEX_ATTRIBUTE_weights] = json_number(p_json_mesh_primitive_attributes_weights);
        } else {
            p_mesh_primitive->vertex_attribute_accessors_indices[GLTF_MESH_VERTEX_ATTRIBUTE_weights] = GLTF_INVALID_INDEX;
        }

        if (p_json_mesh_primitive_indices) {
            p_mesh_primitive->vertex_indices_accessor_index = json_number(p_json_mesh_primitive_indices);
        } else {
            p_mesh_primitive->vertex_indices_accessor_index = GLTF_INVALID_INDEX;
        }

        if (p_json_mesh_primitive_material) {
            p_mesh_primitive->material_index = json_number(p_json_mesh_primitive_material);
        } else {
            p_mesh_primitive->material_index = GLTF_INVALID_INDEX;
        }

        if (p_json_mesh_primitive_mode) {
            p_mesh_primitive->mode = (enum gltf_mesh_primitive_mode)json_number(p_json_mesh_primitive_mode);
        }
    }
}

void read_skin(struct gltf_reader* p_reader, const struct json_value* p_json_skin, void* p_skin) {
    struct gltf_skin* p_skin_ = p_skin;

    const struct json_value* p_json_skin_inverse_bind_matrices;
    const struct json_value* p_json_skin_joints;

    read_gltf_object_property(p_reader, p_json_skin, "inverseBindMatrices", 0, json_is_number, &p_json_skin_inverse_bind_matrices);
    read_gltf_object_property(p_reader, p_json_skin, "joints", 1, json_is_array, &p_json_skin_joints);

    if (p_reader->error != GLTF_SUCCESS) {
        return;
    }

    if (p_json_skin_inverse_bind_matrices) {
        p_skin_->inverse_bind_matrices_accessor_index = json_number(p_json_skin_inverse_bind_matrices);
    } else {
        p_skin_->inverse_bind_matrices_accessor_index = GLTF_INVALID_INDEX;
    }

    json_array_len(p_json_skin_joints);

    
    p_skin_->num_joints = json_array_len(p_json_skin_joints);
    p_skin_->p_joints_indices = calloc(p_skin_->num_joints, sizeof(*p_skin_->p_joints_indices));

    for (size_t i = 0; i < p_skin_->num_joints; ++i) {
        const struct json_value* p_json_joint_index = json_array_get(p_json_skin_joints, i);
        
        if (!json_is_number(p_json_joint_index)) {
            p_reader->error = GLTF_ERROR_BAD_TYPE;
            return;
        }

        p_skin_->p_joints_indices[i] = json_number(p_json_joint_index);

        p_reader->p_result->p_nodes[p_skin_->p_joints_indices[i]].b_is_joint = 1;
    }
}

void read_animation(struct gltf_reader* p_reader, const struct json_value* p_json_animation, void* p_animation) {
    struct gltf_animation* p_animation_ = p_animation;

    const struct json_value* p_json_animation_channels;
    const struct json_value* p_json_animation_samplers;

    read_gltf_object_property(p_reader, p_json_animation, "channels", 1, json_is_array, &p_json_animation_channels);
    read_gltf_object_property(p_reader, p_json_animation, "samplers", 1, json_is_array, &p_json_animation_samplers);

    if (p_reader->error != GLTF_SUCCESS) {
        return;
    }
    
    p_animation_->num_samplers = json_array_len(p_json_animation_samplers);
    p_animation_->p_samplers = calloc(p_animation_->num_samplers, sizeof(*p_animation_->p_samplers));

    for (size_t i = 0; i < p_animation_->num_samplers; ++i) {
        const struct json_value* p_json_animation_sampler = json_array_get(p_json_animation_samplers, i);

        if (!json_is_object(p_json_animation_sampler)) {
            p_reader->error = GLTF_ERROR_BAD_TYPE;
            return;
        }

        const struct json_value* p_json_animation_sampler_input;
        const struct json_value* p_json_animation_sampler_interpolation;
        const struct json_value* p_json_animation_sampler_output;
        
        read_gltf_object_property(p_reader, p_json_animation_sampler, "input", 1, json_is_number, &p_json_animation_sampler_input);
        read_gltf_object_property(p_reader, p_json_animation_sampler, "interpolation", 0, json_is_string, &p_json_animation_sampler_interpolation);
        read_gltf_object_property(p_reader, p_json_animation_sampler, "output", 1, json_is_number, &p_json_animation_sampler_output);

        if (p_reader->error != GLTF_SUCCESS)
            return;

        struct gltf_animation_sampler* p_sampler = &p_animation_->p_samplers[i];
        *p_sampler = (struct gltf_animation_sampler) {
            .interpolation = GLTF_ANIMATION_SAMPLER_INTERPOLATION_linear
        };

        p_sampler->input_accessor_index = json_number(p_json_animation_sampler_input);

        p_sampler->output_accessor_index = json_number(p_json_animation_sampler_output);

        if (p_json_animation_sampler_interpolation) {
            p_sampler->interpolation = (enum gltf_animation_sampler_interpolation)json_number(p_json_animation_sampler_interpolation);
        }
    }

    p_animation_->num_channels = json_array_len(p_json_animation_channels);
    p_animation_->p_channels = calloc(p_animation_->num_channels, sizeof(*p_animation_->p_channels));

    for (size_t i = 0; i < p_animation_->num_channels; ++i) {
        const struct json_value* p_json_animation_channel = json_array_get(p_json_animation_channels, i);

        if (!json_is_object(p_json_animation_channel)) {
            p_reader->error = GLTF_ERROR_BAD_TYPE;
            return;
        }

        const struct json_value* p_json_animation_channel_sampler;
        const struct json_value* p_json_animation_channel_target;
        const struct json_value* p_json_animation_channel_target_node;
        const struct json_value* p_json_animation_channel_target_path;

        read_gltf_object_property(p_reader, p_json_animation_channel, "sampler", 1, json_is_number, &p_json_animation_channel_sampler);
        read_gltf_object_property(p_reader, p_json_animation_channel, "target", 1, json_is_object, &p_json_animation_channel_target);
        read_gltf_object_property(p_reader, p_json_animation_channel_target, "node", 1, json_is_number, &p_json_animation_channel_target_node);
        read_gltf_object_property(p_reader, p_json_animation_channel_target, "path", 1, json_is_string, &p_json_animation_channel_target_path);

        if (p_reader->error != GLTF_SUCCESS) {
            return;
        }

        struct gltf_animation_channel* p_channel = &p_animation_->p_channels[i];
        
        p_channel->source_sampler_index = json_number(p_json_animation_channel_sampler);
        
        p_channel->target_node_index = json_number(p_json_animation_channel_target_node);
        
        const char* s_target_path = json_string(p_json_animation_channel_target_path);
        if (strcmp(s_target_path, "translation") == 0) {
            p_channel->target_path = GLTF_ANIMATION_CHANNEL_TARGET_PATH_translation;
        } else if (strcmp(s_target_path, "rotation") == 0) {
            p_channel->target_path = GLTF_ANIMATION_CHANNEL_TARGET_PATH_translation;
        } else if (strcmp(s_target_path, "scale") == 0) {
            p_channel->target_path = GLTF_ANIMATION_CHANNEL_TARGET_PATH_translation;
        } else {
            p_reader->error = GLTF_ERROR_BAD_VALUE;
            return;
        }
    }
}

void read_node(struct gltf_reader* p_reader, const struct json_value* p_json_node, void* p_node) {
    struct gltf_node* p_node_ = p_node;
    *p_node_ = (struct gltf_node) {
        .matrix = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 },
    };

    const struct json_value* p_json_node_children;
    const struct json_value* p_json_node_skin;
    const struct json_value* p_json_node_matrix;
    const struct json_value* p_json_node_mesh;
    const struct json_value* p_json_node_rotation;
    const struct json_value* p_json_node_scale;
    const struct json_value* p_json_node_translation;
    const struct json_value* p_json_node_name;

    read_gltf_object_property(p_reader, p_json_node, "children", 0, json_is_array, &p_json_node_children);
    read_gltf_object_property(p_reader, p_json_node, "skin", 0, json_is_number, &p_json_node_skin);
    read_gltf_object_property(p_reader, p_json_node, "matrix", 0, json_is_array, &p_json_node_matrix);
    read_gltf_object_property(p_reader, p_json_node, "mesh", 0, json_is_number, &p_json_node_mesh);
    read_gltf_object_property(p_reader, p_json_node, "rotation", 0, json_is_array, &p_json_node_rotation);
    read_gltf_object_property(p_reader, p_json_node, "scale", 0, json_is_array, &p_json_node_scale);
    read_gltf_object_property(p_reader, p_json_node, "translation", 0, json_is_array, &p_json_node_translation);
    read_gltf_object_property(p_reader, p_json_node, "name", 0, json_is_string, &p_json_node_name);

    if (p_reader->error != GLTF_SUCCESS) {
        return;
    }

    if (p_json_node_children) {
        p_node_->num_children = json_array_len(p_json_node_children);
        p_node_->p_children_indices = calloc(p_node_->num_children, sizeof(*p_node_->p_children_indices));

        for (size_t i = 0; i < p_node_->num_children; ++i) {
            const struct json_value* p_json_node_children_elem = json_array_get(p_json_node_children, i);

            if (!json_is_number(p_json_node_children_elem)) {
                p_reader->error = GLTF_ERROR_BAD_TYPE;
                return;
            }

            p_node_->p_children_indices[i] = json_number(p_json_node_children_elem);
        }
    }

    if (p_json_node_skin) {
        p_node_->skin_index = json_number(p_json_node_skin);
    } else {
        p_node_->skin_index = GLTF_INVALID_INDEX;
    }

    if (p_json_node_mesh) {
        p_node_->mesh_index = json_number(p_json_node_mesh);
    } else {
        p_node_->mesh_index = GLTF_INVALID_INDEX;
    }

    if (p_json_node_matrix) {
        read_gltf_number_array(p_reader, p_json_node_matrix, 16, p_node_->matrix);

        if (p_reader->error != GLTF_SUCCESS) {
            return;
        }
    } else if (p_json_node_rotation || p_json_node_scale || p_json_node_translation) {
        float rotation[4] = { 0, 0, 0, 1 };
        float scale[3] = { 1, 1, 1 };
        float translation[3] = { 0, 0, 0, };

        if (p_json_node_rotation) {
            read_gltf_number_array(p_reader, p_json_node_rotation, 4, rotation);
        }

        if (p_json_node_scale) {
            read_gltf_number_array(p_reader, p_json_node_scale, 3, scale);
        }

        if (p_json_node_translation) {
            read_gltf_number_array(p_reader, p_json_node_translation, 3, translation);
        }

        if (p_reader->error != GLTF_SUCCESS) {
            return;
        }

        float rotation_matrix[16];
        float scale_matrix[16];
        float translation_matrix[16];

        matrix_make_rotation(rotation, rotation_matrix);
        matrix_make_scale(scale, scale_matrix);
        matrix_make_translation(translation, translation_matrix);

        matrix_multiply(rotation_matrix, scale_matrix, p_node_->matrix);
        matrix_multiply(translation_matrix, p_node_->matrix, p_node_->matrix);
    }

    if (p_json_node_name) {
        strcpy(p_node_->s_name, json_string(p_json_node_name));
    }
}

void read_scene(struct gltf_reader* p_reader, const struct json_value* p_json_scene, void* p_scene) {
    struct gltf_scene* p_scene_ = p_scene;

    const struct json_value* p_json_scene_nodes;

    read_gltf_object_property(p_reader, p_json_scene, "nodes", 0, json_is_array, &p_json_scene_nodes);

    if (!p_json_scene_nodes) {
        return;
    }

    p_scene_->num_root_nodes = json_array_len(p_json_scene_nodes);
    p_scene_->p_root_nodes_indices = calloc(p_scene_->num_root_nodes, sizeof(*p_scene_->p_root_nodes_indices));

    for (size_t i = 0; i < p_scene_->num_root_nodes; ++i) {
        const struct json_value* p_json_scene_nodes_elem = json_array_get(p_json_scene_nodes, i);

        if (!json_is_number(p_json_scene_nodes_elem)) {
            p_reader->error = GLTF_ERROR_BAD_TYPE;
            return;
        }

        p_scene_->p_root_nodes_indices[i] = json_number(p_json_scene_nodes_elem);
    }
}

void read_default_scene(struct gltf_reader* p_reader) {
    const struct json_value* p_json_gltf_scene;

    read_gltf_object_property(p_reader, p_reader->p_json_gltf, "scene", 0, json_is_number, &p_json_gltf_scene);

    if (!p_json_gltf_scene) {
        p_reader->p_result->default_scene_index = GLTF_INVALID_INDEX;
        return;
    }

    p_reader->p_result->default_scene_index = json_number(p_json_gltf_scene);
}

void* load_uri_payload(struct gltf_reader* p_reader, const char* s_uri, size_t* p_bytes_len) {
    if (strncmp(s_uri, "data:", 5) == 0) {
        const char* p_data_start = strchr(s_uri, ',') + 1;
        const size_t uri_len = strlen(s_uri);
        const size_t data_len = uri_len - (p_data_start - s_uri);
        const size_t data_bytes = ((data_len + (data_len % 4)) / 4) * 3;

        unsigned char* p_data = malloc(data_bytes);

        if (!p_data) {
            p_reader->error = GLTF_ERROR_MEMORY;
            return 0;
        }

        printf("Decoding Base64: ");

        for (size_t i = 0; i < data_bytes / 3; ++i) {
            char sextets[4] = {0};

            for (size_t j = 0; j < 4 && (i * 4 + j) < data_len; ++j) {
                char c = p_data_start[i * 4 + j];

                if (c >= 'A' && c <= 'Z') {
                    sextets[j] = c - 65;
                } else if (c >= 'a' && c <= 'z') {
                    sextets[j] = c - 71;
                } else if (c >= '0' && c <= '9') {
                    sextets[j] = c + 4;
                } else if (c == '+') {
                    sextets[j] = 62;
                } else if (c == '/') {
                    sextets[j] = 63;
                }
                    
                printf("%c", c);
            }

            printf("[");

            for (size_t j = 0; j < 3 && (i * 3 + j) < data_bytes; ++j) {
                const int first_sextet_left_shift = 2 * (j + 1);
                const int second_sextet_left_shift = 6 - first_sextet_left_shift;
                p_data[i * 3 + j] = (sextets[j] << first_sextet_left_shift) | (sextets[j + 1] >> second_sextet_left_shift);

                printf("%s%d", j != 0 ? "-" : "", p_data[i * 3 + j]);
            }
            
            printf("] ");
        }
        
        printf("\n");

        *p_bytes_len = data_bytes;
        return p_data;
    }

    make_file_path(p_reader->s_filedir, s_uri, p_reader->s_temp_filepath);
    FILE* p_file = fopen(p_reader->s_temp_filepath, "rb");
    if (!p_file) {
        cx_log_fmt(CX_LOG_ERROR, LOG_CAT_GLTF, "Failed to open glTF file from URI path '%s'\n", p_reader->s_temp_filepath);  
        p_reader->error = GLTF_ERROR_BAD_URI;
    }

    fseek(p_file, 0, SEEK_END);
    const long file_size = ftell(p_file);
    fseek(p_file, 0, SEEK_SET);

    void* p_data = malloc(file_size);
    if (!p_data) {
        p_reader->error = GLTF_ERROR_MEMORY;
        return 0;
    }

    fread(p_data, file_size, 1, p_file);

    fclose(p_file);

    *p_bytes_len = file_size;
    return p_data;
}

void matrix_multiply(const float* p_m1, const float* p_m2, float* p_result) {
    float result[16] = {0};
    for (int n = 0; n < 4; ++n) {
        for (int m = 0; m < 4; ++m) {
            for (int i = 0; i < 4; ++i) {
                result[n * 4 + m] += p_m1[i * 4 + m] * p_m2[n * 4 + i];
            }
        }
    }
    memcpy(p_result, result, sizeof(*result) * 16);
}

void matrix_make_rotation(const float* p_quaternion_xyzw, float* p_result) {
    const float i = p_quaternion_xyzw[0];
    const float j = p_quaternion_xyzw[1];
    const float k = p_quaternion_xyzw[2];
    const float w = p_quaternion_xyzw[3];
    const float ij = i * j;
    const float ik = i * k;
    const float iw = i * w;
    const float jk = j * k;
    const float jw = j * w;
    const float kw = k * w;
    const float ww = w * w;

    p_result[ 0] = 2 * (ww + i*i) - 1;
    p_result[ 4] = 2 * (ij - kw);
    p_result[ 8] = 2 * (ik + jw);

    p_result[ 1] = 2 * (ij + kw);
    p_result[ 5] = 2 * (ww + j*j) - 1;
    p_result[ 9] = 2 * (jk - iw);

    p_result[ 2] = 2 * (ik - jw);
    p_result[ 6] = 2 * (jk + iw);
    p_result[10] = 2 * (ww + k*k) - 1;

    p_result[ 3] = p_result[7] = p_result[11] = p_result[12] = p_result[13] = p_result[14] = 0.f;
    p_result[15] = 1.f;
}

void matrix_make_scale(const float* p_scale_xyz, float* p_result) {
    memset(p_result, 0, sizeof(*p_result) * 16);
    p_result[ 0] = p_scale_xyz[0];
    p_result[ 5] = p_scale_xyz[1];
    p_result[10] = p_scale_xyz[2];
    p_result[15] = 1.f;
}

void matrix_make_translation(const float* p_translation_xyz, float* p_result) {
    memset(p_result, 0, sizeof(*p_result) * 16);
    p_result[12] = p_translation_xyz[0];
    p_result[13] = p_translation_xyz[1];
    p_result[14] = p_translation_xyz[2];
    p_result[ 0] = p_result[5] = p_result[10] = p_result[15] = 1.f;
}