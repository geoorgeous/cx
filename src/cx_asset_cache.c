#include "cx_alloc.h"
#include "cx_array.h"
#include "cx_asset.h"
#include "cx_asset_cache.h"
#include "cx_logging.h"
#include "hashtable.h"

struct cx_asset_cache_entry {
	uint32_t ref_count;
	void* p_asset;
};

static struct {
	struct cx_array sources;
	struct hashtable assets;
} cache;

static int cx_asset_source_cmp(const void* p_a, const void* p_b);

void cx_asset_cache_push_source(const struct cx_asset_source* p_source) {
	if (cache.sources.element_size == 0) {
		cx_array_init(sizeof(struct cx_asset_source), &cache.sources);
	}

	cx_array_insert(&cache.sources, 0, p_source);

	CX_LOG_FMT(INFO, ASSET, "New asset cache source added: p_context=%p, f_try_deserialize_asset=%p\n",
		p_source->p_context, p_source->f_try_deserialize_asset);
}

void cx_asset_cache_remove_source(const struct cx_asset_source* p_source) {
	size_t index;
	if (!cx_array_find(&cache.sources, p_source, cx_asset_source_cmp, &index)) {
		return;
	}
	cx_array_remove_at(&cache.sources, index);

	CX_LOG_FMT(INFO, ASSET, "Asset cache source removed: p_context=%p, f_try_deserialize_asset=%p\n",
		p_source->p_context, p_source->f_try_deserialize_asset);
}

void cx_asset_cache_adopt(cx_asset_id asset_id, void* p_asset, struct cx_asset_ref* p_out) {
	if (cache.assets.element_size_ == 0) {
		hashtable_init(&cache.assets, sizeof(struct cx_asset_cache_entry));
	}

	struct cx_asset_cache_entry* p_cache_entry = hashtable_i_get(&cache.assets, asset_id);
	*p_cache_entry = (struct cx_asset_cache_entry) {
		.p_asset = p_asset,
		.ref_count = 1
	};

	*p_out = (struct cx_asset_ref) {
		.asset_id = asset_id,
		.pp_asset = &p_cache_entry->p_asset
	};

	CX_LOG_FMT(INFO, ASSET, "New asset adopted: type=%u(%s), id=%u, p=%p\n",
		CX_ASSET_GET_TYPE_ID(asset_id),
		cx_asset_type_display_name_str(CX_ASSET_GET_TYPE_ID(asset_id)),
		asset_id,
		p_asset);
}

void* cx_asset_cache_acquire(struct cx_asset_ref* p_ref) {
	if (cx_asset_ref_is_valid(p_ref)) {
		return cx_asset_ref_get(p_ref);
	}

	if (cache.assets.element_size_ == 0) {
		hashtable_init(&cache.assets, sizeof(struct cx_asset_cache_entry));
	}

	struct cx_asset_cache_entry* p_cache_entry = hashtable_i_get(&cache.assets, p_ref->asset_id);

	p_cache_entry->ref_count++;

	if (p_cache_entry->ref_count == 1) {
		const cx_asset_type type = CX_ASSET_GET_TYPE_ID(p_ref->asset_id);

		p_cache_entry->p_asset = CX_CALLOC(cx_asset_type_size(type));

		int b_result = CX_FALSE;

		for (size_t i = 0; i < cache.sources.length; ++i) {
			struct cx_asset_source* p_source = cx_array_at(&cache.sources, i);
			b_result = p_source->f_try_deserialize_asset(p_ref->asset_id, p_source->p_context, p_cache_entry->p_asset);
			if (b_result) {
				break;
			}
		}

		if (!b_result) {
			p_cache_entry->ref_count--;
			return CX_NULL;
		}
	}

	p_ref->pp_asset = &p_cache_entry->p_asset;

	CX_LOG_FMT(INFO, ASSET, "Asset reference acquired: type=%u(%s), id=%u, ref_count=%u\n",
		CX_ASSET_GET_TYPE_ID(p_ref->asset_id),
		cx_asset_type_display_name_str(CX_ASSET_GET_TYPE_ID(p_ref->asset_id)),
		p_ref->asset_id,
		p_cache_entry->ref_count);
	
	return p_cache_entry->p_asset;
}

void cx_asset_cache_release(struct cx_asset_ref* p_ref) {
	const cx_asset_id asset_id = p_ref->asset_id;

	*p_ref = (struct cx_asset_ref){0};

	struct hashtable_itr itr;
	if (!hashtable_i_find(&cache.assets, asset_id, &itr)) {
		return;
	}
	
	struct cx_asset_cache_entry* p_cache_entry = itr.p_value;
	p_cache_entry->ref_count--;

	CX_LOG_FMT(INFO, ASSET, "Asset reference released: type=%u(%s), id=%u, ref_count=%u\n",
		CX_ASSET_GET_TYPE_ID(asset_id),
		cx_asset_type_display_name_str(CX_ASSET_GET_TYPE_ID(asset_id)),
		asset_id,
		p_cache_entry->ref_count);
	
	if (p_cache_entry->ref_count > 0) {
		return;
	}

	cx_asset_type_free_asset(CX_ASSET_GET_TYPE_ID(asset_id), p_cache_entry->p_asset);
	free(p_cache_entry->p_asset);
	hashtable_i_remove(&cache.assets, asset_id);
}

void cx_asset_cache_free(void) {
	struct hashtable_itr itr;
	hashtable_itr(&cache.assets, &itr);

	while(hashtable_itr_is_valid(&itr)) {
		const struct cx_asset_cache_entry* p_cache_entry = itr.p_value;
		const cx_asset_id* p_asset_id = itr.p_key;
		const cx_asset_type type = CX_ASSET_GET_TYPE_ID(*p_asset_id);

		cx_asset_type_free_asset(type, p_cache_entry->p_asset);
		free(p_cache_entry->p_asset);

		hashtable_itr_next(&itr);
	}

	hashtable_free(&cache.assets);

	cx_array_free(&cache.sources);
}

int cx_asset_source_cmp(const void* p_a, const void* p_b) {
	const struct cx_asset_source* p_a_ = p_a;
	const struct cx_asset_source* p_b_ = p_b;
	return p_a_->f_try_deserialize_asset == p_b_->f_try_deserialize_asset;
}
