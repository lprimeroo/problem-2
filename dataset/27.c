#define STREAM_BUFFER_SIZE (1024 * 16)

static int write_zip_entry(struct archiver_args *args,
			   const struct object_id *oid,
			   const char *path, size_t pathlen,
			   unsigned int mode)
{
	struct zip_local_header header;
	uintmax_t offset = zip_offset;
	struct zip_extra_mtime extra;
	struct zip64_extra extra64;
	size_t header_extra_size = ZIP_EXTRA_MTIME_SIZE;
	int need_zip64_extra = 0;
	unsigned long attr2;
	unsigned long compressed_size;
	unsigned long crc;
	int method;
	unsigned char *out;
	void *deflated = NULL;
	void *buffer;
	struct git_istream *stream = NULL;
	unsigned long flags = 0;
	unsigned long size;
	int is_binary = -1;
	const char *path_without_prefix = path + args->baselen;
	unsigned int creator_version = 0;
	unsigned int version_needed = 10;
	size_t zip_dir_extra_size = ZIP_EXTRA_MTIME_SIZE;
	size_t zip64_dir_extra_payload_size = 0;

	crc = crc32(0, NULL, 0);

	if (!has_only_ascii(path)) {
		if (is_utf8(path))
			flags |= ZIP_UTF8;
		else
			warning(_("path is not valid UTF-8: %s"), path);
	}

	if (pathlen > 0xffff) {
		return error(_("path too long (%d chars, SHA1: %s): %s"),
				(int)pathlen, oid_to_hex(oid), path);
	}

	if (S_ISDIR(mode) || S_ISGITLINK(mode)) {
		method = 0;
		attr2 = 16;
		out = NULL;
		size = 0;
		compressed_size = 0;
		buffer = NULL;
	} else if (S_ISREG(mode) || S_ISLNK(mode)) {
		enum object_type type = oid_object_info(args->repo, oid,
							&size);

		method = 0;
		attr2 = S_ISLNK(mode) ? ((mode | 0777) << 16) :
			(mode & 0111) ? ((mode) << 16) : 0;
		if (S_ISLNK(mode) || (mode & 0111))
			creator_version = 0x0317;
		if (S_ISREG(mode) && args->compression_level != 0 && size > 0)
			method = 8;

		if (S_ISREG(mode) && type == OBJ_BLOB && !args->convert &&
		    size > big_file_threshold) {
			stream = open_istream(oid, &type, &size, NULL);
			if (!stream)
				return error(_("cannot stream blob %s"),
					     oid_to_hex(oid));
			flags |= ZIP_STREAM;
			out = buffer = NULL;
		} else {
			buffer = object_file_to_archive(args, path, oid, mode,
							&type, &size);
			if (!buffer)
				return error(_("cannot read %s"),
					     oid_to_hex(oid));
			crc = crc32(crc, buffer, size);
			is_binary = entry_is_binary(args->repo->index,
						    path_without_prefix,
						    buffer, size);
			out = buffer;
		}
		compressed_size = (method == 0) ? size : 0;
	} else {
		return error(_("unsupported file mode: 0%o (SHA1: %s)"), mode,
				oid_to_hex(oid));
	}

	if (creator_version > max_creator_version)
		max_creator_version = creator_version;

	if (buffer && method == 8) {
		out = deflated = zlib_deflate_raw(buffer, size,
						  args->compression_level,
						  &compressed_size);
		if (!out || compressed_size >= size) {
			out = buffer;
			method = 0;
			compressed_size = size;
		}
	}

	copy_le16(extra.magic, 0x5455);
	copy_le16(extra.extra_size, ZIP_EXTRA_MTIME_PAYLOAD_SIZE);
	extra.flags[0] = 1;	/* just mtime */
	copy_le32(extra.mtime, args->time);

	if (size > 0xffffffff || compressed_size > 0xffffffff)
		need_zip64_extra = 1;
	if (stream && size > 0x7fffffff)
		need_zip64_extra = 1;

	if (need_zip64_extra)
		version_needed = 45;

	copy_le32(header.magic, 0x04034b50);
	copy_le16(header.version, version_needed);
	copy_le16(header.flags, flags);
	copy_le16(header.compression_method, method);
	copy_le16(header.mtime, zip_time);
	copy_le16(header.mdate, zip_date);
	if (need_zip64_extra) {
		set_zip_header_data_desc(&header, 0xffffffff, 0xffffffff, crc);
		header_extra_size += ZIP64_EXTRA_SIZE;
	} else {
		set_zip_header_data_desc(&header, size, compressed_size, crc);
	}
	copy_le16(header.filename_length, pathlen);
	copy_le16(header.extra_length, header_extra_size);
	write_or_die(1, &header, ZIP_LOCAL_HEADER_SIZE);
	zip_offset += ZIP_LOCAL_HEADER_SIZE;
	write_or_die(1, path, pathlen);
	zip_offset += pathlen;
	write_or_die(1, &extra, ZIP_EXTRA_MTIME_SIZE);
	zip_offset += ZIP_EXTRA_MTIME_SIZE;
	if (need_zip64_extra) {
		copy_le16(extra64.magic, 0x0001);
		copy_le16(extra64.extra_size, ZIP64_EXTRA_PAYLOAD_SIZE);
		copy_le64(extra64.size, size);
		copy_le64(extra64.compressed_size, compressed_size);
		write_or_die(1, &extra64, ZIP64_EXTRA_SIZE);
		zip_offset += ZIP64_EXTRA_SIZE;
	}

	if (stream && method == 0) {
		unsigned char buf[STREAM_BUFFER_SIZE];
		ssize_t readlen;

		for (;;) {
			readlen = read_istream(stream, buf, sizeof(buf));
			if (readlen <= 0)
				break;
			crc = crc32(crc, buf, readlen);
			if (is_binary == -1)
				is_binary = entry_is_binary(args->repo->index,
							    path_without_prefix,
							    buf, readlen);
			write_or_die(1, buf, readlen);
		}
		close_istream(stream);
		if (readlen)
			return readlen;

		compressed_size = size;
		zip_offset += compressed_size;

		write_zip_data_desc(size, compressed_size, crc);
	} else if (stream && method == 8) {
		unsigned char buf[STREAM_BUFFER_SIZE];
		ssize_t readlen;
		git_zstream zstream;
		int result;
		size_t out_len;
		unsigned char compressed[STREAM_BUFFER_SIZE * 2];

		git_deflate_init_raw(&zstream, args->compression_level);

		compressed_size = 0;
		zstream.next_out = compressed;
		zstream.avail_out = sizeof(compressed);

		for (;;) {
			readlen = read_istream(stream, buf, sizeof(buf));
			if (readlen <= 0)
				break;
			crc = crc32(crc, buf, readlen);
			if (is_binary == -1)
				is_binary = entry_is_binary(args->repo->index,
							    path_without_prefix,
							    buf, readlen);

			zstream.next_in = buf;
			zstream.avail_in = readlen;
			result = git_deflate(&zstream, 0);
			if (result != Z_OK)
				die(_("deflate error (%d)"), result);
			out_len = zstream.next_out - compressed;

			if (out_len > 0) {
				write_or_die(1, compressed, out_len);
				compressed_size += out_len;
				zstream.next_out = compressed;
				zstream.avail_out = sizeof(compressed);
			}

		}
		close_istream(stream);
		if (readlen)
			return readlen;

		zstream.next_in = buf;
		zstream.avail_in = 0;
		result = git_deflate(&zstream, Z_FINISH);
		if (result != Z_STREAM_END)
			die("deflate error (%d)", result);

		git_deflate_end(&zstream);
		out_len = zstream.next_out - compressed;
		write_or_die(1, compressed, out_len);
		compressed_size += out_len;
		zip_offset += compressed_size;

		write_zip_data_desc(size, compressed_size, crc);
	} else if (compressed_size > 0) {
		write_or_die(1, out, compressed_size);
		zip_offset += compressed_size;
	}

	free(deflated);
	free(buffer);

	if (compressed_size > 0xffffffff || size > 0xffffffff ||
	    offset > 0xffffffff) {
		if (compressed_size >= 0xffffffff)
			zip64_dir_extra_payload_size += 8;
		if (size >= 0xffffffff)
			zip64_dir_extra_payload_size += 8;
		if (offset >= 0xffffffff)
			zip64_dir_extra_payload_size += 8;
		zip_dir_extra_size += 2 + 2 + zip64_dir_extra_payload_size;
	}

	strbuf_add_le(&zip_dir, 4, 0x02014b50);	/* magic */
	strbuf_add_le(&zip_dir, 2, creator_version);
	strbuf_add_le(&zip_dir, 2, version_needed);
	strbuf_add_le(&zip_dir, 2, flags);
	strbuf_add_le(&zip_dir, 2, method);
	strbuf_add_le(&zip_dir, 2, zip_time);
	strbuf_add_le(&zip_dir, 2, zip_date);
	strbuf_add_le(&zip_dir, 4, crc);
	strbuf_add_le(&zip_dir, 4, clamp32(compressed_size));
	strbuf_add_le(&zip_dir, 4, clamp32(size));
	strbuf_add_le(&zip_dir, 2, pathlen);
	strbuf_add_le(&zip_dir, 2, zip_dir_extra_size);
	strbuf_add_le(&zip_dir, 2, 0);		/* comment length */
	strbuf_add_le(&zip_dir, 2, 0);		/* disk */
	strbuf_add_le(&zip_dir, 2, !is_binary);
	strbuf_add_le(&zip_dir, 4, attr2);
	strbuf_add_le(&zip_dir, 4, clamp32(offset));
	strbuf_add(&zip_dir, path, pathlen);
	strbuf_add(&zip_dir, &extra, ZIP_EXTRA_MTIME_SIZE);
	if (zip64_dir_extra_payload_size) {
		strbuf_add_le(&zip_dir, 2, 0x0001);	/* magic */
		strbuf_add_le(&zip_dir, 2, zip64_dir_extra_payload_size);
		if (size >= 0xffffffff)
			strbuf_add_le(&zip_dir, 8, size);
		if (compressed_size >= 0xffffffff)
			strbuf_add_le(&zip_dir, 8, compressed_size);
		if (offset >= 0xffffffff)
			strbuf_add_le(&zip_dir, 8, offset);
	}
	zip_dir_entries++;

	return 0;
}