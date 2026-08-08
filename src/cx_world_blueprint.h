#ifndef CX_WORLD_BLUEPRINT_H
#define CX_WORLD_BLUEPRINT_H

#include "cx_blueprint.h"

#define CX_ASSET_TYPE_WORLD_BLUEPRINT 8

struct cx_world_blueprint {
	struct cx_blueprint root;
};

struct cx_stream;

int cx_world_blueprint_serialize(const struct cx_world_blueprint* p_world_blueprint, struct cx_stream* p_stream);
int cx_world_blueprint_deserialize(struct cx_stream* p_stream, struct cx_world_blueprint* p_out_world_blueprint);
void cx_world_blueprint_free(struct cx_world_blueprint* p_world_blueprint);

static inline int cx_world_blueprint_asset_serialize(const void* p_asset, struct cx_stream* p_stream) {
	return cx_world_blueprint_serialize(p_asset, p_stream);
}

static inline int cx_world_blueprint_asset_deserialize(struct cx_stream* p_stream, void* p_out_asset) {
	return cx_world_blueprint_deserialize(p_stream, p_out_asset);
}

static inline void cx_world_blueprint_asset_free(void* p_asset) {
	cx_world_blueprint_free(p_asset);
}

#endif
