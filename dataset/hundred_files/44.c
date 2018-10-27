static void all_attrs_init(struct attr_hashmap *map, struct attr_check *check)
{
	int i;
	unsigned int size;

	hashmap_lock(map);

	size = hashmap_get_size(&map->map);
	if (size < check->all_attrs_nr)
		BUG("interned attributes shouldn't be deleted");

	hashmap_unlock(map);
	for (i = 0; i < check->all_attrs_nr; i++) {
		check->all_attrs[i].value = ATTR__UNKNOWN;
		check->all_attrs[i].macro = NULL;
	}
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