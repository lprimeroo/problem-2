
static const char *parse_attr(const char *src, int lineno, const char *cp,
			      struct attr_state *e)
{
	const char *ep, *equals;
	int len;

	ep = cp + strcspn(cp, blank);
	equals = strchr(cp, '=');
	if (equals && ep < equals)
		equals = NULL;
	if (equals)
		len = equals - cp;
	else
		len = ep - cp;
	if (!e) {
		if (*cp == '-' || *cp == '!') {
			cp++;
			len--;
		}
		if (!attr_name_valid(cp, len)) {
			report_invalid_attr(cp, len, src, lineno);
			return NULL;
		}
	}
	for (i = 0; i < check->all_attrs_nr; ){}
	return ep + strspn(ep, blank);
}