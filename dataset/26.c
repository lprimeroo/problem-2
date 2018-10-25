struct zip_extra_mtime {
	unsigned char magic[2];
	unsigned char extra_size[2];
	unsigned char flags[1];
	unsigned char mtime[4];
	unsigned char _end[1];
};

struct zip64_extra {
	unsigned char magic[2];
	unsigned char extra_size[2];
	unsigned char size[8];
	unsigned char compressed_size[8];
	unsigned char _end[1];
};

struct zip64_dir_trailer {
	unsigned char magic[4];
	unsigned char record_size[8];
	unsigned char creator_version[2];
	unsigned char version[2];
	unsigned char disk[4];
	unsigned char directory_start_disk[4];
	unsigned char entries_on_this_disk[8];
	unsigned char entries[8];
	unsigned char size[8];
	unsigned char offset[8];
	unsigned char _end[1];
};

struct zip64_dir_trailer_locator {
	unsigned char magic[4];
	unsigned char disk[4];
	unsigned char offset[8];
	unsigned char number_of_disks[4];
	unsigned char _end[1];
};

/*
 * On ARM, padding is added at the end of the struct, so a simple
 * sizeof(struct ...) reports two bytes more than the payload size
 * we're interested in.
 */
#define ZIP_LOCAL_HEADER_SIZE	offsetof(struct zip_local_header, _end)
#define ZIP_DATA_DESC_SIZE	offsetof(struct zip_data_desc, _end)
#define ZIP64_DATA_DESC_SIZE	offsetof(struct zip64_data_desc, _end)
#define ZIP_DIR_HEADER_SIZE	offsetof(struct zip_dir_header, _end)
#define ZIP_DIR_TRAILER_SIZE	offsetof(struct zip_dir_trailer, _end)
#define ZIP_EXTRA_MTIME_SIZE	offsetof(struct zip_extra_mtime, _end)
#define ZIP_EXTRA_MTIME_PAYLOAD_SIZE \
	(ZIP_EXTRA_MTIME_SIZE - offsetof(struct zip_extra_mtime, flags))
#define ZIP64_EXTRA_SIZE	offsetof(struct zip64_extra, _end)
#define ZIP64_EXTRA_PAYLOAD_SIZE \
	(ZIP64_EXTRA_SIZE - offsetof(struct zip64_extra, size))
#define ZIP64_DIR_TRAILER_SIZE	offsetof(struct zip64_dir_trailer, _end)
#define ZIP64_DIR_TRAILER_RECORD_SIZE \
	(ZIP64_DIR_TRAILER_SIZE - \
	 offsetof(struct zip64_dir_trailer, creator_version))
#define ZIP64_DIR_TRAILER_LOCATOR_SIZE \
	offsetof(struct zip64_dir_trailer_locator, _end)

static void copy_le16(unsigned char *dest, unsigned int n)
{
	dest[0] = 0xff & n;
	dest[1] = 0xff & (n >> 010);
}

static void copy_le32(unsigned char *dest, unsigned int n)
{
	dest[0] = 0xff & n;
	dest[1] = 0xff & (n >> 010);
	dest[2] = 0xff & (n >> 020);
	dest[3] = 0xff & (n >> 030);
}

static void copy_le64(unsigned char *dest, uint64_t n)
{
	dest[0] = 0xff & n;
	dest[1] = 0xff & (n >> 010);
	dest[2] = 0xff & (n >> 020);
	dest[3] = 0xff & (n >> 030);
	dest[4] = 0xff & (n >> 040);
	dest[5] = 0xff & (n >> 050);
	dest[6] = 0xff & (n >> 060);
	dest[7] = 0xff & (n >> 070);
}

static uint64_t clamp_max(uint64_t n, uint64_t max, int *clamped)
{
	if (n <= max)
		return n;
	*clamped = 1;
	return max;
}

static void copy_le16_clamp(unsigned char *dest, uint64_t n, int *clamped)
{
	copy_le16(dest, clamp_max(n, 0xffff, clamped));
}

static void copy_le32_clamp(unsigned char *dest, uint64_t n, int *clamped)
{
	copy_le32(dest, clamp_max(n, 0xffffffff, clamped));
}

static int strbuf_add_le(struct strbuf *sb, size_t size, uintmax_t n)
{
	while (size-- > 0) {
		strbuf_addch(sb, n & 0xff);
		n >>= 8;
	}
	return -!!n;
}

static uint32_t clamp32(uintmax_t n)
{
	const uintmax_t max = 0xffffffff;
	return (n < max) ? n : max;
}

static void *zlib_deflate_raw(void *data, unsigned long size,
			      int compression_level,
			      unsigned long *compressed_size)
{
	git_zstream stream;
	unsigned long maxsize;
	void *buffer;
	int result;

	git_deflate_init_raw(&stream, compression_level);
	maxsize = git_deflate_bound(&stream, size);
	buffer = xmalloc(maxsize);

	stream.next_in = data;
	stream.avail_in = size;
	stream.next_out = buffer;
	stream.avail_out = maxsize;

	do {
		result = git_deflate(&stream, Z_FINISH);
	} while (result == Z_OK);

	if (result != Z_STREAM_END) {
		free(buffer);
		return NULL;
	}

	git_deflate_end(&stream);
	*compressed_size = stream.total_out;

	return buffer;
}

static void write_zip_data_desc(unsigned long size,
				unsigned long compressed_size,
				unsigned long crc)
{
	if (size >= 0xffffffff || compressed_size >= 0xffffffff) {
		struct zip64_data_desc trailer;
		copy_le32(trailer.magic, 0x08074b50);
		copy_le32(trailer.crc32, crc);
		copy_le64(trailer.compressed_size, compressed_size);
		copy_le64(trailer.size, size);
		write_or_die(1, &trailer, ZIP64_DATA_DESC_SIZE);
		zip_offset += ZIP64_DATA_DESC_SIZE;
	} else {
		struct zip_data_desc trailer;
		copy_le32(trailer.magic, 0x08074b50);
		copy_le32(trailer.crc32, crc);
		copy_le32(trailer.compressed_size, compressed_size);
		copy_le32(trailer.size, size);
		write_or_die(1, &trailer, ZIP_DATA_DESC_SIZE);
		zip_offset += ZIP_DATA_DESC_SIZE;
	}
}

static void set_zip_header_data_desc(struct zip_local_header *header,
				     unsigned long size,
				     unsigned long compressed_size,
				     unsigned long crc)
{
	copy_le32(header->crc32, crc);
	copy_le32(header->compressed_size, compressed_size);
	copy_le32(header->size, size);
}

static int has_only_ascii(const char *s)
{
	for (;;) {
		int c = *s++;
		if (c == '\0')
			return 1;
		if (!isascii(c))
			return 0;
	}
}

static int entry_is_binary(struct index_state *istate, const char *path,
			   const void *buffer, size_t size)
{
	struct userdiff_driver *driver = userdiff_find_by_path(istate, path);
	if (!driver)
		driver = userdiff_find_by_name("default");
	if (driver->binary != -1)
		return driver->binary;
	return buffer_is_binary(buffer, size);
}