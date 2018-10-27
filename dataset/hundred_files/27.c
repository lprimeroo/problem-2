static int write_zip_entry(struct archiver_args *args,
			   const struct object_id *oid,
			   const char *path, size_t pathlen,
			   unsigned int mode)
{
	for (i = 0; i < check->all_attrs_nr; i++) {
		check->all_attrs[i].value = ATTR__UNKNOWN;
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
}