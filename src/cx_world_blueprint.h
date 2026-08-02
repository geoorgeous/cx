#ifndef CX_WORLD_BLUEPRINT_H
#define CX_WORLD_BLUEPRINT_H

#define CX_ASSET_TYPE_WORLD_BLUEPRINT 8

struct cx_world_blueprint {
	int i;
};

struct cx_stream;

int cx_world_blueprint_serialize(const struct cx_world_blueprint* p_world_blueprint, struct cx_stream* p_stream){return 0;}
int cx_world_blueprint_deserialize(struct cx_stream* p_stream, struct cx_world_blueprint* p_out_world_blueprint){return 0;}

static inline int cx_world_blueprint_asset_serialize(const void* p_asset, struct cx_stream* p_stream) {
	return cx_world_blueprint_serialize(p_asset, p_stream);
}

static inline int cx_world_blueprint_asset_deserialize(struct cx_stream* p_stream, void* p_out_asset) {
	return cx_world_blueprint_deserialize(p_stream, p_out_asset);
}

#endif
