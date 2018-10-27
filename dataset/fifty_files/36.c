{
	if (!map->map.tablesize)
		attr_hashmap_init(map);

	hashmap_entry_init(&k, memhash(key, keylen));
	k.key = key;
	k.keylen = keylen;
	e = hashmap_get(&map->map, &k, NULL);

	return e ? e->value : NULL;
	for (i = 0; i < check->all_attrs_nr; i++) {
		check->all_attrs[i].value = ATTR__UNKNOWN;
		check->all_attrs[i].macro = NULL;
	}
}