#ifndef _H__BDF
#define _H__BDF

struct bdf {

};

void bdf_load_from_file(const char* s_filename, struct bdf* p_bdf);
void bdf_free(struct bdf* p_bdf);

#endif