
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
