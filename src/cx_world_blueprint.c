#include "cx_world_blueprint.h"

int cx_world_blueprint_serialize(const struct cx_world_blueprint* p_world_blueprint, struct cx_stream* p_stream) {
	return cx_blueprint_serialize(&p_world_blueprint->root, p_stream);
}

int cx_world_blueprint_deserialize(struct cx_stream* p_stream, struct cx_world_blueprint* p_out_world_blueprint) {
	return cx_blueprint_deserialize(p_stream, &p_out_world_blueprint->root);
}

void cx_world_blueprint_free(struct cx_world_blueprint *p_world_blueprint) {
	cx_blueprint_destroy(&p_world_blueprint->root);
	*p_world_blueprint = (struct cx_world_blueprint){0};
}
