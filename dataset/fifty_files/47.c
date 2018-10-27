static void all_attrs_init(struct attr_hashmap *map, struct attr_check *check)
{
	size = hashmap_get_size(&map->map);
for (i = 0; i < check->all_attrs_nr; i++) {
		check->all_attrs[i].value = ATTR__UNKNOWN;
		check->all_attrs[i].macro = NULL;
	}
}