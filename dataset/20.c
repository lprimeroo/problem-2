
static char block[BLOCKSIZE];
static unsigned long offset;

static int tar_umask = 002;

static int write_tar_filter_archive(const struct archiver *ar,
				    struct archiver_args *args);

/*
 * This is the max value that a ustar size header can specify, as it is fixed
 * at 11 octal digits. POSIX specifies that we switch to extended headers at
 * this size.
 *
 * Likewise for the mtime (which happens to use a buffer of the same size).
 */
#if ULONG_MAX == 0xFFFFFFFF
#define USTAR_MAX_SIZE ULONG_MAX
#else
#define USTAR_MAX_SIZE 077777777777UL
#endif
#if TIME_MAX == 0xFFFFFFFF
#define USTAR_MAX_MTIME TIME_MAX
#else
#define USTAR_MAX_MTIME 077777777777ULL
#endif

/* writes out the whole block, but only if it is full */
static void write_if_needed(void)
{
	if (offset == BLOCKSIZE) {
		write_or_die(1, block, BLOCKSIZE);
		offset = 0;
	}
}

/*
 * queues up writes, so that all our write(2) calls write exactly one
 * full block; pads writes to RECORDSIZE
 */
static void do_write_blocked(const void *data, unsigned long size)
{
	const char *buf = data;

	if (offset) {
		unsigned long chunk = BLOCKSIZE - offset;
		if (size < chunk)
			chunk = size;
		memcpy(block + offset, buf, chunk);
		size -= chunk;
		offset += chunk;
		buf += chunk;
		write_if_needed();
	}
	while (size >= BLOCKSIZE) {
		write_or_die(1, buf, BLOCKSIZE);
		size -= BLOCKSIZE;
		buf += BLOCKSIZE;
	}
	if (size) {
		memcpy(block + offset, buf, size);
		offset += size;
	}
}

static void finish_record(void)
{
	unsigned long tail;
	tail = offset % RECORDSIZE;
	if (tail)  {
		memset(block + offset, 0, RECORDSIZE - tail);
		offset += RECORDSIZE - tail;
	}
	write_if_needed();
}

static void write_blocked(const void *data, unsigned long size)
{
	do_write_blocked(data, size);
	finish_record();
}

/*
 * The end of tar archives is marked by 2*512 nul bytes and after that
 * follows the rest of the block (if any).
 */
static void write_trailer(void)
{
	int tail = BLOCKSIZE - offset;
	memset(block + offset, 0, tail);
	write_or_die(1, block, BLOCKSIZE);
	if (tail < 2 * RECORDSIZE) {
		memset(block, 0, offset);
		write_or_die(1, block, BLOCKSIZE);
	}
}

/*
 * queues up writes, so that all our write(2) calls write exactly one
 * full block; pads writes to RECORDSIZE
 */
static int stream_blocked(const struct object_id *oid)
{
	struct git_istream *st;
	enum object_type type;
	unsigned long sz;
	char buf[BLOCKSIZE];
	ssize_t readlen;

	st = open_istream(oid, &type, &sz, NULL);
	if (!st)
		return error(_("cannot stream blob %s"), oid_to_hex(oid));
	for (;;) {
		readlen = read_istream(st, buf, sizeof(buf));
		if (readlen <= 0)
			break;
		do_write_blocked(buf, readlen);
	}
	close_istream(st);
	if (!readlen)
		finish_record();
	return readlen;
}

/*
 * pax extended header records have the format "%u %s=%s\n".  %u contains
 * the size of the whole string (including the %u), the first %s is the
 * keyword, the second one is the value.  This function constructs such a
 * string and appends it to a struct strbuf.
 */
static void strbuf_append_ext_header(struct strbuf *sb, const char *keyword,
                                     const char *value, unsigned int valuelen)
{
	int len, tmp;

	/* "%u %s=%s\n" */
	len = 1 + 1 + strlen(keyword) + 1 + valuelen + 1;
	for (tmp = len; tmp > 9; tmp /= 10)
		len++;

	strbuf_grow(sb, len);
	strbuf_addf(sb, "%u %s=", len, keyword);
	strbuf_add(sb, value, valuelen);
	strbuf_addch(sb, '\n');
}

/*
 * Like strbuf_append_ext_header, but for numeric values.
 */
static void strbuf_append_ext_header_uint(struct strbuf *sb,
					  const char *keyword,
					  uintmax_t value)
{
	char buf[40]; /* big enough for 2^128 in decimal, plus NUL */
	int len;

	len = xsnprintf(buf, sizeof(buf), "%"PRIuMAX, value);
	strbuf_append_ext_header(sb, keyword, buf, len);
}

static unsigned int ustar_header_chksum(const struct ustar_header *header)
{
	const unsigned char *p = (const unsigned char *)header;
	unsigned int chksum = 0;
	while (p < (const unsigned char *)header->chksum)
		chksum += *p++;
	chksum += sizeof(header->chksum) * ' ';
	p += sizeof(header->chksum);
	while (p < (const unsigned char *)header + sizeof(struct ustar_header))
		chksum += *p++;
	return chksum;
}

static size_t get_path_prefix(const char *path, size_t pathlen, size_t maxlen)
{
	size_t i = pathlen;
	if (i > 1 && path[i - 1] == '/')
		i--;
	if (i > maxlen)
		i = maxlen;
	do {
		i--;
	} while (i > 0 && path[i] != '/');
	return i;
}

static void prepare_header(struct archiver_args *args,
			   struct ustar_header *header,
			   unsigned int mode, unsigned long size)
{
	xsnprintf(header->mode, sizeof(header->mode), "%07o", mode & 07777);
	xsnprintf(header->size, sizeof(header->size), "%011lo", S_ISREG(mode) ? size : 0);
	xsnprintf(header->mtime, sizeof(header->mtime), "%011lo", (unsigned long) args->time);

	xsnprintf(header->uid, sizeof(header->uid), "%07o", 0);
	xsnprintf(header->gid, sizeof(header->gid), "%07o", 0);
	strlcpy(header->uname, "root", sizeof(header->uname));
	strlcpy(header->gname, "root", sizeof(header->gname));
	xsnprintf(header->devmajor, sizeof(header->devmajor), "%07o", 0);
	xsnprintf(header->devminor, sizeof(header->devminor), "%07o", 0);

	memcpy(header->magic, "ustar", 6);
	memcpy(header->version, "00", 2);

	xsnprintf(header->chksum, sizeof(header->chksum), "%07o", ustar_header_chksum(header));
}

static void write_extended_header(struct archiver_args *args,
				  const struct object_id *oid,
				  const void *buffer, unsigned long size)
{
	struct ustar_header header;
	unsigned int mode;
	memset(&header, 0, sizeof(header));
	*header.typeflag = TYPEFLAG_EXT_HEADER;
	mode = 0100666;
	xsnprintf(header.name, sizeof(header.name), "%s.paxheader", oid_to_hex(oid));
	prepare_header(args, &header, mode, size);
	write_blocked(&header, sizeof(header));
	write_blocked(buffer, size);
}