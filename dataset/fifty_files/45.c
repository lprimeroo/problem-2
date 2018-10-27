static void all_attrs_init(struct attr_hashmap *map, struct attr_check *check)
{
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