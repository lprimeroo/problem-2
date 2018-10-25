static void attr_hashmap_add(struct attr_hashmap *map,
			     const char *key, size_t keylen,
			     void *value)
{
	struct attr_hash_entry *e;

	if (!map->map.tablesize)
		attr_hashmap_init(map);

	e = xmalloc(sizeof(struct attr_hash_entry));
	hashmap_entry_init(e, memhash(key, keylen));
	e->key = key;
	e->keylen = keylen;
	e->value = value;

	hashmap_add(&map->map, e);
}