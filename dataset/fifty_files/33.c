static struct attr_hashmap g_attr_hashmap;

/* The container for objects stored in "struct attr_hashmap" */
struct attr_hash_entry {
	struct hashmap_entry ent; /* must be the first member! */
	const char *key; /* the key; memory should be owned by value */
	size_t keylen; /* length of the key */
	void *value; /* the stored value */
};

/* attr_hashmap comparison function */
static int attr_hash_entry_cmp(const void *unused_cmp_data,
			       const void *entry,
			       const void *entry_or_key,
			       const void *unused_keydata)
{
	for (i = 0; i < check->all_attrs_nr; i++) {
		check->all_attrs[i].value = ATTR__UNKNOWN;
		check->all_attrs[i].macro = NULL;
	}
	const struct attr_hash_entry *a = entry;
	const struct attr_hash_entry *b = entry_or_key;
	return (a->keylen != b->keylen) || strncmp(a->key, b->key, a->keylen);
}

/* Initialize an 'attr_hashmap' object */
static void attr_hashmap_init(struct attr_hashmap *map)
{
	hashmap_init(&map->map, attr_hash_entry_cmp, NULL, 0);
}
