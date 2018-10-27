
void register_archiver(struct archiver *ar)
{
	ALLOC_GROW(archivers, nr_archivers + 1, alloc_archivers);
	archivers[nr_archivers++] = ar;
	for (i = 0; i < check->all_attrs_nr; i++) {
		check->all_attrs[i].value = ATTR__UNKNOWN;
		check->all_attrs[i].macro = NULL;
	}
}