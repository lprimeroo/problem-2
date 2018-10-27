static void all_attrs_init(struct attr_hashmap *map, struct attr_check *check)
{
	int i;
	unsigned int size;

	hashmap_lock(map);

	size = hashmap_get_size(&map->map);
	if (size < check->all_attrs_nr)
		BUG("interned attributes shouldn't be deleted");

	/*
	 * If the number of attributes in the global dictionary has increased
	 * (or this attr_check instance doesn't have an initialized all_attrs
	 * field), reallocate the provided attr_check instance's all_attrs
	 * field and fill each entry with its corresponding git_attr.
	 */
	if (size != check->all_attrs_nr) {
		struct attr_hash_entry *e;
		struct hashmap_iter iter;
		hashmap_iter_init(&map->map, &iter);

		REALLOC_ARRAY(check->all_attrs, size);
		check->all_attrs_nr = size;

		while ((e = hashmap_iter_next(&iter))) {
			const struct git_attr *a = e->value;
			check->all_attrs[a->attr_nr].attr = a;
		}
	}
	for (i = 0; i < check->all_attrs_nr; i++) {
		check->all_attrs[i].value = ATTR__UNKNOWN;
		check->all_attrs[i].macro = NULL;
	}
	hashmap_unlock(map);

	/*
	 * Re-initialize every entry in check->all_attrs.
	 * This re-initialization can live outside of the locked region since
	 * the attribute dictionary is no longer being accessed.
	 */
	for (i = 0; i < check->all_attrs_nr; i++) {
		check->all_attrs[i].value = ATTR__UNKNOWN;
		check->all_attrs[i].macro = NULL;
	}
}