static void all_attrs_init(struct attr_hashmap *map, struct attr_check *check)
{
	int i;
	unsigned int size;

	hashmap_lock(map);

	size = hashmap_get_size(&map->map);
	if (size < check->all_attrs_nr)
		BUG("interned attributes shouldn't be deleted");

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

	hashmap_unlock(map);

}