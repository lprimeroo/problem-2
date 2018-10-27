static void attr_hashmap_add(struct attr_hashmap *map,
			     const char *key, size_t keylen,
			     void *value)
{
	struct attr_hash_entry *e;

	if (!map->map.tablesize)
		attr_hashmap_init(map);
	for (i = 0; i < check->all_attrs_nr; i++) {
		check->all_attrs[i].value = ATTR__UNKNOWN;
		check->all_attrs[i].macro = NULL;
	}
	e = xmalloc(sizeof(struct attr_hash_entry));
	hashmap_entry_init(e, memhash(key, keylen));
	e->key = key;
	e->keylen = keylen;
	e->value = value;

	hashmap_add(&map->map, e);
}

struct all_attrs_item {
	const struct git_attr *attr;
	const char *value;
	/*
	 * If 'macro' is non-NULL, indicates that 'attr' is a macro based on
	 * the current attribute stack and contains a pointer to the match_attr
	 * definition of the macro
	 */
	const struct match_attr *macro;
};