#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>

#include "cx_io.h"

struct cx_io_dir_internals {
	DIR* p_dir;
};

struct cx_io_dir_entry_internals {
	struct dirent* p_dirent;
};

cx_result cx_io_dir_open(const char* s_dir, struct cx_io_dir* p_out) {
	*p_out = (struct cx_io_dir) {
		.s_dir = s_dir
	};

	struct cx_io_dir_internals* p_internals = (void*)p_out->internals_.bytes_;
	*p_internals = (struct cx_io_dir_internals) {
		.p_dir = opendir(s_dir)
	};

	if (p_internals->p_dir) {
		return CX_SUCCESS;
	}

	switch (errno) {
		case EACCES:       return CX_ERROR_PERMISSION_DENIED;
		case EMFILE:
		case ENFILE:
		case ENOMEM:       return CX_ERROR_OUT_OF_MEMORY;
		case ENAMETOOLONG: return CX_ERROR_INVALID_ARG;
		case ENOENT:
		case ENOTDIR:      return CX_ERROR_NOT_FOUND;
		default:           return CX_ERROR_UNKNOWN;
	}
}

cx_result cx_io_dir_next_entry(struct cx_io_dir* p_dir, struct cx_io_dir_entry* p_out) {
	struct cx_io_dir_internals* p_internals = (void*)p_dir->internals_.bytes_;

	errno = 0;
	struct dirent* p_dirent = readdir(p_internals->p_dir);

	if (p_dirent == CX_NULL) {
		if (errno == 0) {
			*p_out = (struct cx_io_dir_entry) {
				.type = CX_IO_DIR_ENTRY_TYPE_EOD
			};
			return CX_SUCCESS;
		}

		if (errno == EBADF) {
			return CX_ERROR_INVALID_ARG;
		}

		return CX_ERROR_UNKNOWN;
	}

	struct stat st;
	char filepath[256];

	cx_io_filepath_join(p_dir->s_dir, p_dirent->d_name, filepath);

	if (stat(filepath, &st) != 0) {
		return CX_ERROR_UNKNOWN;
	}

	*p_out = (struct cx_io_dir_entry) {
		.p_dir = p_dir,
		.s_name = p_dirent->d_name,
	};

	struct cx_io_dir_entry_internals* p_entry_internals = (void*)p_out->internals_.bytes_;
	*p_entry_internals = (struct cx_io_dir_entry_internals) {
		.p_dirent = p_dirent
	};

	if (S_ISDIR(st.st_mode)) {
		p_out->type = CX_IO_DIR_ENTRY_TYPE_DIR;
	} else if (S_ISREG(st.st_mode)) {
		p_out->type = CX_IO_DIR_ENTRY_TYPE_FILE;
	}

	return CX_SUCCESS;
}

void cx_io_dir_close(struct cx_io_dir* p_dir) {
	struct cx_io_dir_internals* p_internals = (void*)p_dir->internals_.bytes_;
	closedir(p_internals->p_dir);
	*p_dir = (struct cx_io_dir){0};
}
