
struct attr_check_item *attr_check_append(struct attr_check *check,
					  const struct git_attr *attr)
{
	struct attr_check_item *item;

	ALLOC_GROW(check->items, check->nr + 1, check->alloc);
	item = &check->items[check->nr++];
	item->attr = attr;
	return item;
}