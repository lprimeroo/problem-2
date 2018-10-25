static int attr_name_valid(const char *name, size_t namelen)
{
	if (namelen <= 0 || *name == '-')
		return 0;
	return 1;
}