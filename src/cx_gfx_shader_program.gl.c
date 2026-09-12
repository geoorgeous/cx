#include <string.h>

#include "gl.h"

#include "cx_alloc.h"
#include "cx_gfx_shader_program.h"
#include "cx_gfx_shader_program_interface.gl.h"
#include "cx_logging.h"
#include "cx_shader.h"

#define CX_UNIFORM_NAME_MAX_LEN 128

struct cx_gfx_shader_program_gl_internals {
	GLuint gl_handle;
};

static cx_result create_shader_stage(GLenum gl_shader_stage, const GLchar* p_source, GLint source_len, GLuint* p_out);
static int is_opengl_enum_glsl_sampler_type(GLenum e);
static GLenum opengl_sampler_type_to_texture_target(GLenum e);
static enum cx_gfx_shader_program_value_type opengl_enum_to_shader_program_value_type(GLenum e);
static enum cx_gfx_shader_program_texture_type opengl_enum_to_shader_program_texture_type(GLenum e);
static void log_program_info_log(GLuint gl_handle, int level, const char* s_message);
static const char* cx_gfx_shader_program_value_type_str(enum cx_gfx_shader_program_value_type type);
static const char* cx_gfx_shader_program_texture_type_str(enum cx_gfx_shader_program_texture_type type);
static const char* opengl_texture_target_str(GLenum e);

cx_result cx_gfx_shader_program_create(struct cx_gfx_shader_program* p_out) {
	struct cx_gfx_shader_program_gl_internals* p_internals = (void*)p_out->internals_.bytes_;

	(void)glGetError();
	p_internals->gl_handle = glCreateProgram();
	
	if (p_internals->gl_handle == 0) {
		GLenum glerr = glGetError();
		return
			glerr == GL_INVALID_OPERATION ? CX_ERROR_INVALID_OPERATION :
			glerr == GL_OUT_OF_MEMORY ? CX_ERROR_OUT_OF_MEMORY :
			CX_ERROR_UNKNOWN;
	}
	return CX_SUCCESS;
}

void cx_gfx_shader_program_destroy(struct cx_gfx_shader_program* p_shader_program) {
	struct cx_gfx_shader_program_gl_internals* p_internals = (void*)p_shader_program->internals_.bytes_;

	if (p_internals->gl_handle) {
		glDeleteProgram(p_internals->gl_handle);
	}
}

cx_result cx_gfx_shader_program_build_from_source(
	const struct cx_gfx_shader_program* p_shader_program, const struct cx_shader_source* p_source) {

	const struct cx_gfx_shader_program_gl_internals* p_internals = (const void*)p_shader_program->internals_.bytes_;

	if (p_internals->gl_handle == 0) {
		return CX_ERROR_INVALID_ARG;
	}

	GLuint gl_vertex_stage_handle;
	GLuint gl_fragment_stage_handle;

	cx_result res;
	
	res = create_shader_stage(
		GL_VERTEX_SHADER, 
		(const GLchar*)p_source->p_vertex_stage_source, 
		(GLint)p_source->vertex_stage_source_len,
		&gl_vertex_stage_handle);

	if (res != CX_SUCCESS) {
		return res;
	}

	res = create_shader_stage(
		GL_FRAGMENT_SHADER,
		(const GLchar*)p_source->p_fragment_stage_source, 
		(GLint)p_source->fragment_stage_source_len, 
		&gl_fragment_stage_handle);

	if (res != CX_SUCCESS) {
		CX_LOG_FMT(TRACE, GFX_SHADER_PROGRAM, "Program compiled: gl_id=%u\n", p_internals->gl_handle);
		return res;
	}

	glAttachShader(p_internals->gl_handle, gl_vertex_stage_handle);
	glAttachShader(p_internals->gl_handle, gl_fragment_stage_handle);

	GLint gl_program_ival;
	
	glLinkProgram(p_internals->gl_handle);
	glGetProgramiv(p_internals->gl_handle, GL_LINK_STATUS, &gl_program_ival);

	if (gl_program_ival == GL_FALSE) {
		log_program_info_log(p_internals->gl_handle, CX_LOG_LEVEL_ERROR, "Program linking failed");
		res = CX_ERROR_INVALID_ARG;
		goto error;
	}

	glValidateProgram(p_internals->gl_handle);
	glGetProgramiv(p_internals->gl_handle, GL_VALIDATE_STATUS, &gl_program_ival);

	if (gl_program_ival == GL_FALSE) {
		log_program_info_log(p_internals->gl_handle, CX_LOG_LEVEL_ERROR, "Program validation failed");
		res = CX_ERROR_INVALID_ARG;
		goto error;
	}

error:
	glDeleteShader(gl_vertex_stage_handle);
	glDeleteShader(gl_fragment_stage_handle);
	return res;
}

void cx_gfx_shader_program_reflect_interface(
	const struct cx_gfx_shader_program* p_shader_program, struct cx_gfx_shader_program_interface* p_out) {

	CX_LOG(TRACE, GFX_SHADER_PROGRAM, "Reflecting program interface...\n");

	const struct cx_gfx_shader_program_gl_internals* p_internals = (const void*)p_shader_program->internals_.bytes_;

	glUseProgram(p_internals->gl_handle);

	GLint count;
	char string_buf[CX_GFX_SHADER_PROGRAM_NAME_MAX_LEN];

	uint16_t num_parameters = 0;
	uint16_t num_textures = 0;

	glGetProgramiv(p_internals->gl_handle, GL_ACTIVE_UNIFORMS, &count);

	for (GLuint i = 0; i < (GLuint)count; ++i) {
		GLint uniform_block_index;
		glGetActiveUniformsiv(p_internals->gl_handle, 1, &i, GL_UNIFORM_BLOCK_INDEX, &uniform_block_index);

		if (uniform_block_index >= 0) {
			continue;
		}

		GLsizei uniform_name_len;
		GLenum  uniform_type;
		GLint   uniform_size;
		
		glGetActiveUniform(
			p_internals->gl_handle,
			(GLuint)i, CX_UNIFORM_NAME_MAX_LEN,
			&uniform_name_len,
			&uniform_size,
			&uniform_type,
			string_buf);

		if (is_opengl_enum_glsl_sampler_type(uniform_type)) {
			num_textures++;
		} else {
			num_parameters++;
		}
	}

	struct cx_gfx_shader_program_parameter_info* p_parameters =
		CX_MALLOC(sizeof(*p_out->p_parameters) * num_parameters);
	struct cx_gfx_shader_program_texture_info* p_textures =
		CX_MALLOC(sizeof(*p_out->p_textures) * num_textures);

	GLint texture_unit = 0;

	for (GLuint i = 0; i < (GLuint)count; ++i) {
		GLint uniform_block_index;
		glGetActiveUniformsiv(p_internals->gl_handle, 1, &i, GL_UNIFORM_BLOCK_INDEX, &uniform_block_index);

		if (uniform_block_index >= 0) {
			continue;
		}

		GLsizei uniform_name_len;
		GLenum  uniform_type;
		GLint   uniform_size;
		
		glGetActiveUniform(
			p_internals->gl_handle,
			i,
			CX_UNIFORM_NAME_MAX_LEN,
			&uniform_name_len,
			&uniform_size,
			&uniform_type,
			string_buf);

		if (is_opengl_enum_glsl_sampler_type(uniform_type)) {
			struct cx_gfx_shader_program_texture_info* p_info =
				&p_textures[p_out->num_textures];

			strcpy(p_info->name, string_buf);

			p_info->type = opengl_enum_to_shader_program_texture_type(uniform_type);

			struct cx_gfx_shader_program_texture_info_gl_internals* p_info_internals =
				CX_GET_OPAQUE_INTERNALS(*p_info);

			p_info_internals->texture_target = opengl_sampler_type_to_texture_target(uniform_type);
			p_info_internals->texture_unit = texture_unit;
			
			glUniform1i(glGetUniformLocation(p_internals->gl_handle, string_buf), texture_unit);

			texture_unit++;

			p_out->num_textures++;
		} else {
			struct cx_gfx_shader_program_parameter_info* p_info =
				&p_parameters[p_out->num_parameters];

			strcpy(p_info->name, string_buf);

			p_info->type = opengl_enum_to_shader_program_value_type(uniform_type);

			struct cx_gfx_shader_program_parameter_info_gl_internals* p_info_internals =
				CX_GET_OPAQUE_INTERNALS(*p_info);

			p_info_internals->uniform_location = glGetUniformLocation(p_internals->gl_handle, string_buf);
			
			p_out->num_parameters++;
		}
	}

	glGetProgramiv(p_internals->gl_handle, GL_ACTIVE_UNIFORM_BLOCKS, &count);
	
	struct cx_gfx_shader_program_block_info* p_blocks = CX_MALLOC(sizeof(*p_out->p_blocks) * (uint16_t)count);

	for (GLuint i = 0; i < (GLuint)count; ++i) {
		struct cx_gfx_shader_program_block_info* p_info = &p_blocks[i];

		GLsizei block_name_len;
		glGetActiveUniformBlockName(p_internals->gl_handle, i, CX_UNIFORM_NAME_MAX_LEN, &block_name_len, string_buf);

		strcpy(p_info->name, string_buf);

		GLint block_data_size;
		glGetActiveUniformBlockiv(p_internals->gl_handle, i, GL_UNIFORM_BLOCK_DATA_SIZE, &block_data_size);

		p_info->size = (size_t)block_data_size;

		GLint block_num_members;
		glGetActiveUniformBlockiv(
			p_internals->gl_handle,
			i,
			GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS,
			&block_num_members);

		p_info->num_members = (uint16_t)block_num_members;

		p_info->p_members = CX_MALLOC(sizeof(*p_info->p_members) * p_info->num_members);

		GLint block_member_uniform_indices[256];
		glGetActiveUniformBlockiv(
			p_internals->gl_handle,
			i,
			GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES,
			block_member_uniform_indices);

		for (uint16_t j = 0; j < p_info->num_members; ++j) {
			const GLuint block_member_uniform_index = (GLuint)block_member_uniform_indices[j];

			GLsizei uniform_name_len;
			GLenum  uniform_type;
			GLint   uniform_size;
			
			glGetActiveUniform(
				p_internals->gl_handle,
				block_member_uniform_index,
				CX_UNIFORM_NAME_MAX_LEN,
				&uniform_name_len,
				&uniform_size,
				&uniform_type,
				string_buf);

			struct cx_gfx_shader_program_block_member_info* p_member_info = &p_info->p_members[j];

			strcpy(p_member_info->name, string_buf);

			p_member_info->type = opengl_enum_to_shader_program_value_type(uniform_type);

			GLint member_offset;
			glGetActiveUniformsiv(
				p_internals->gl_handle, 1, &block_member_uniform_index, GL_UNIFORM_OFFSET, &member_offset);
			p_member_info->offset = (size_t)member_offset;

			GLint member_array_count;
			glGetActiveUniformsiv(
				p_internals->gl_handle, 1, &block_member_uniform_index, GL_UNIFORM_SIZE, &member_array_count);
			p_member_info->array_len = (uint16_t)member_array_count;

			GLint member_array_stride;
			glGetActiveUniformsiv(
				p_internals->gl_handle, 1, &block_member_uniform_index, GL_UNIFORM_ARRAY_STRIDE, &member_array_stride);
			p_member_info->array_stride = (size_t)member_array_stride;

			GLint member_matrix_stride;
			glGetActiveUniformsiv(
				p_internals->gl_handle,
				1,
				&block_member_uniform_index,
				GL_UNIFORM_MATRIX_STRIDE,
				&member_matrix_stride);
			p_member_info->matrix_stride = (size_t)member_array_stride;
		}

		struct cx_gfx_shader_program_block_info_gl_internals* p_info_internals = CX_GET_OPAQUE_INTERNALS(*p_info);

		const GLuint binding_point = i;
		glUniformBlockBinding(p_internals->gl_handle, i, binding_point);

		p_info_internals->uniform_block_binding_point = i;

		p_out->num_blocks++;
	}

	p_out->p_parameters = p_parameters;
	p_out->p_textures = p_textures;
	p_out->p_blocks = p_blocks;

	if (p_out->num_parameters > 0) {
		CX_LOG_FMT(TRACE, GFX_SHADER_PROGRAM, "  Parameters: (%u)\n", p_out->num_parameters);
	}

	for (uint16_t i = 0; i < p_out->num_parameters; ++i) {
		const struct cx_gfx_shader_program_parameter_info* p_info = &p_out->p_parameters[i];
		const struct cx_gfx_shader_program_parameter_info_gl_internals* p_info_internals =
			CX_GET_OPAQUE_INTERNALS_CONST(*p_info);

		CX_LOG_FMT(TRACE, GFX_SHADER_PROGRAM, "    [%u]: name='%s', type=%s, opengl_uniform_location=%d\n",
			i, p_info->name, cx_gfx_shader_program_value_type_str(p_info->type), p_info_internals->uniform_location);
	}

	if (p_out->num_textures > 0) {
		CX_LOG_FMT(TRACE, GFX_SHADER_PROGRAM, "  Textures: (%u)\n", p_out->num_textures);
	}

	for (uint16_t i = 0; i < p_out->num_textures; ++i) {
		const struct cx_gfx_shader_program_texture_info* p_info = &p_out->p_textures[i];
		const struct cx_gfx_shader_program_texture_info_gl_internals* p_info_internals =
			CX_GET_OPAQUE_INTERNALS_CONST(*p_info);

		CX_LOG_FMT(TRACE, GFX_SHADER_PROGRAM, 
			"    [%u]: name='%s', type='%s', opengl_texture_target=%s, opengl_texture_unit=GL_TEXTURE%d\n",
			i,
			p_info->name,
			cx_gfx_shader_program_texture_type_str(p_info->type),
			opengl_texture_target_str(p_info_internals->texture_target),
			p_info_internals->texture_unit);
	}

	if (p_out->num_blocks > 0) {
		CX_LOG_FMT(TRACE, GFX_SHADER_PROGRAM, "  Blocks: (%u)\n", p_out->num_blocks);
	}

	for (uint16_t i = 0; i < p_out->num_blocks; ++i) {
		const struct cx_gfx_shader_program_block_info* p_info = &p_out->p_blocks[i];
		const struct cx_gfx_shader_program_block_info_gl_internals* p_info_internals =
			CX_GET_OPAQUE_INTERNALS_CONST(*p_info);

		CX_LOG_FMT(TRACE, GFX_SHADER_PROGRAM,
			"    [%u]: name='%s', size=%"CX_PRI_SIZE", opengl_binding_point=%u\n",
			i, p_info->name, p_info->size, p_info_internals->uniform_block_binding_point);

		CX_LOG_FMT(TRACE, GFX_SHADER_PROGRAM, "      Members: (%u)\n", p_info->num_members);

		for (uint16_t j = 0; j < p_info->num_members; ++j) {
			const struct cx_gfx_shader_program_block_member_info* p_member_info = &p_info->p_members[j];

			CX_LOG_FMT(TRACE, GFX_SHADER_PROGRAM,
				"      [%u]: name='%s', offset=%"CX_PRI_SIZE", type=%s, count=%u\n",
				j,
				p_member_info->name,
				p_member_info->offset,
				cx_gfx_shader_program_value_type_str(p_member_info->type),
				p_member_info->array_len);
		}
	}
}

void cx_gfx_shader_program_free_interface(struct cx_gfx_shader_program_interface* p_interface) {
	CX_FREE(p_interface->p_parameters);

	CX_FREE(p_interface->p_textures);

	for (uint16_t i = 0; i < p_interface->num_blocks; ++i) {
		CX_FREE(p_interface->p_blocks[i].p_members);
	}

	CX_FREE(p_interface->p_blocks);
}

void cx_gfx_shader_program_bind(const struct cx_gfx_shader_program* p_shader_program) {
	const struct cx_gfx_shader_program_gl_internals* p_internals = (const void*)p_shader_program->internals_.bytes_;

	glUseProgram(p_internals->gl_handle);
}

cx_result create_shader_stage(GLenum gl_shader_stage, const GLchar* p_source, GLint source_len, GLuint* p_out) {
	GLuint gl_shader_handle = glCreateShader(gl_shader_stage);

	if (gl_shader_stage == 0) {
		return CX_ERROR_UNKNOWN;
	}

	glShaderSource(gl_shader_handle, 1, &p_source, &source_len);

	glCompileShader(gl_shader_handle);

	GLint b_is_compiled;
	glGetShaderiv(gl_shader_handle, GL_COMPILE_STATUS, &b_is_compiled);

	if (b_is_compiled) {
		*p_out = gl_shader_handle;
		return CX_SUCCESS;
	}

	char log[1024];
	glGetShaderInfoLog(gl_shader_handle, sizeof(log), 0, log);

	CX_LOG_FMT(ERROR, GFX_SHADER_PROGRAM, "Shader compilation failed. Reason: %s\n", log);

	glDeleteShader(gl_shader_handle);

	return CX_ERROR_INVALID_ARG;
}

int is_opengl_enum_glsl_sampler_type(GLenum e) {
	return
		e == GL_SAMPLER_2D ||
		e == GL_SAMPLER_CUBE;
}

GLenum opengl_sampler_type_to_texture_target(GLenum e) {
	switch(e) {
		case GL_SAMPLER_2D:   return GL_TEXTURE_2D;
		case GL_SAMPLER_CUBE: return GL_TEXTURE_CUBE_MAP;
		default:              return 0;
	}
}

enum cx_gfx_shader_program_value_type opengl_enum_to_shader_program_value_type(GLenum e) {
	switch (e) {
		case GL_FLOAT:             return CX_GFX_SHADER_PROGRAM_VALUE_TYPE_f32;
		case GL_FLOAT_VEC2:        return CX_GFX_SHADER_PROGRAM_VALUE_TYPE_fvec2;
		case GL_FLOAT_VEC3:        return CX_GFX_SHADER_PROGRAM_VALUE_TYPE_fvec3;
		case GL_FLOAT_VEC4:        return CX_GFX_SHADER_PROGRAM_VALUE_TYPE_fvec4;
		case GL_INT:               return CX_GFX_SHADER_PROGRAM_VALUE_TYPE_i32;
		case GL_INT_VEC2:          return CX_GFX_SHADER_PROGRAM_VALUE_TYPE_ivec2;
		case GL_INT_VEC3:          return CX_GFX_SHADER_PROGRAM_VALUE_TYPE_ivec3;
		case GL_INT_VEC4:          return CX_GFX_SHADER_PROGRAM_VALUE_TYPE_ivec4;
		case GL_UNSIGNED_INT:      return CX_GFX_SHADER_PROGRAM_VALUE_TYPE_u32;
		case GL_UNSIGNED_INT_VEC2: return CX_GFX_SHADER_PROGRAM_VALUE_TYPE_uvec2;
		case GL_UNSIGNED_INT_VEC3: return CX_GFX_SHADER_PROGRAM_VALUE_TYPE_uvec3;
		case GL_UNSIGNED_INT_VEC4: return CX_GFX_SHADER_PROGRAM_VALUE_TYPE_uvec4;
		case GL_FLOAT_MAT2:        return CX_GFX_SHADER_PROGRAM_VALUE_TYPE_mat2;
		case GL_FLOAT_MAT3:        return CX_GFX_SHADER_PROGRAM_VALUE_TYPE_mat3;
		case GL_FLOAT_MAT4:        return CX_GFX_SHADER_PROGRAM_VALUE_TYPE_mat4;
		case GL_FLOAT_MAT2x3:      return CX_GFX_SHADER_PROGRAM_VALUE_TYPE_mat2x3;
		case GL_FLOAT_MAT2x4:      return CX_GFX_SHADER_PROGRAM_VALUE_TYPE_mat2x4;
		case GL_FLOAT_MAT3x2:      return CX_GFX_SHADER_PROGRAM_VALUE_TYPE_mat3x2;
		case GL_FLOAT_MAT3x4:      return CX_GFX_SHADER_PROGRAM_VALUE_TYPE_mat3x4;
		case GL_FLOAT_MAT4x2:      return CX_GFX_SHADER_PROGRAM_VALUE_TYPE_mat4x2;
		case GL_FLOAT_MAT4x3:      return CX_GFX_SHADER_PROGRAM_VALUE_TYPE_mat4x3;
		default:                   return 0;
	};
}

enum cx_gfx_shader_program_texture_type opengl_enum_to_shader_program_texture_type(GLenum e) {
	switch (e) {
		case GL_SAMPLER_2D:   return CX_GFX_SHADER_PROGRAM_TEXTURE_TYPE_texture_2d;
		case GL_SAMPLER_CUBE: return CX_GFX_SHADER_PROGRAM_TEXTURE_TYPE_texture_cube;
		default:              return 0;
	};
}

void log_program_info_log(GLuint gl_handle, int level, const char* s_message) {
	char info_log_buf[1024];
	glGetProgramInfoLog(gl_handle, sizeof(info_log_buf), 0, info_log_buf);
	cx_log_fmt(
		level,
		CX_LOG_CAT_GFX_SHADER_PROGRAM,
		"%s: id=%d, info_log=\"%s\"\n",
		s_message, gl_handle, info_log_buf);
}

const char* cx_gfx_shader_program_value_type_str(enum cx_gfx_shader_program_value_type type) {
	return (const char*[]){
		"bool",
		"u32",
		"i32",
		"f32",
		"uvec2",
		"ivec2",
		"fvec2",
		"uvec3",
		"ivec3",
		"fvec3",
		"uvec4",
		"ivec4",
		"fvec4",
		"mat2",
		"mat3",
		"mat4",
		"mat2x3",
		"mat2x4",
		"mat3x2",
		"mat3x4",
		"mat4x2",
		"mat4x3"
	}[type];
}

const char* cx_gfx_shader_program_texture_type_str(enum cx_gfx_shader_program_texture_type type) {
	return (const char*[]){
		"2d",
		"cube"
	}[type];
}

const char* opengl_texture_target_str(GLenum e) {
	switch(e) {
		case GL_TEXTURE_2D:       return "GL_TEXTURE_2D";
		case GL_TEXTURE_CUBE_MAP: return "GL_TEXTURE_CUBE_MAP";
		default:                  return "???";
	};
}
