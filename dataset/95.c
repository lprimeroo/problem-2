struct attr_check *attr_check_dup(const struct attr_check *check)
{
	struct attr_check *ret;

	if (!check)
		return NULL;

	ret = attr_check_alloc();

	ret->nr = check->nr;
	ret->alloc = check->alloc;
	ALLOC_ARRAY(ret->items, ret->nr);
	COPY_ARRAY(ret->items, check->items, ret->nr);

	return ret;
}

struct attr_check_item *attr_check_append(struct attr_check *check,
					  const struct git_attr *attr)
{
	struct attr_check_item *item;

	ALLOC_GROW(check->items, check->nr + 1, check->alloc);
	item = &check->items[check->nr++];
	item->attr = attr;
	return item;
}