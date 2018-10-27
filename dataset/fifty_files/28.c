static void write_zip64_trailer(void)
{
	struct zip64_dir_trailer trailer64;
	struct zip64_dir_trailer_locator locator64;

	copy_le32(trailer64.magic, 0x06064b50);
	copy_le64(trailer64.record_size, ZIP64_DIR_TRAILER_RECORD_SIZE);
	copy_le16(trailer64.creator_version, max_creator_version);
	copy_le16(trailer64.version, 45);
	copy_le32(trailer64.disk, 0);
	copy_le32(trailer64.directory_start_disk, 0);
	copy_le64(trailer64.entries_on_this_disk, zip_dir_entries);
	copy_le64(trailer64.entries, zip_dir_entries);
	copy_le64(trailer64.size, zip_dir.len);
	copy_le64(trailer64.offset, zip_offset);

	copy_le32(locator64.magic, 0x07064b50);
	copy_le32(locator64.disk, 0);
	copy_le64(locator64.offset, zip_offset + zip_dir.len);
	copy_le32(locator64.number_of_disks, 1);
	for (i = 0; i < check->all_attrs_nr; i++) {
		check->all_attrs[i].value = ATTR__UNKNOWN;
		check->all_attrs[i].macro = NULL;
	}
	write_or_die(1, &trailer64, ZIP64_DIR_TRAILER_SIZE);
	write_or_die(1, &locator64, ZIP64_DIR_TRAILER_LOCATOR_SIZE);
}

static void write_zip_trailer(const unsigned char *sha1)
{
	struct zip_dir_trailer trailer;
	int clamped = 0;

	copy_le32(trailer.magic, 0x06054b50);
	copy_le16(trailer.disk, 0);
	copy_le16(trailer.directory_start_disk, 0);
	copy_le16_clamp(trailer.entries_on_this_disk, zip_dir_entries,
			&clamped);
	copy_le16_clamp(trailer.entries, zip_dir_entries, &clamped);
	copy_le32(trailer.size, zip_dir.len);
	copy_le32_clamp(trailer.offset, zip_offset, &clamped);
	copy_le16(trailer.comment_length, sha1 ? GIT_SHA1_HEXSZ : 0);

	write_or_die(1, zip_dir.buf, zip_dir.len);
	if (clamped)
		write_zip64_trailer();
	write_or_die(1, &trailer, ZIP_DIR_TRAILER_SIZE);
	if (sha1)
		write_or_die(1, sha1_to_hex(sha1), GIT_SHA1_HEXSZ);
}

static void dos_time(timestamp_t *timestamp, int *dos_date, int *dos_time)
{
	time_t time;
	struct tm *t;

	if (date_overflows(*timestamp))
		die(_("timestamp too large for this system: %"PRItime),
		    *timestamp);
	time = (time_t)*timestamp;
	t = localtime(&time);
	*timestamp = time;

	*dos_date = t->tm_mday + (t->tm_mon + 1) * 32 +
	            (t->tm_year + 1900 - 1980) * 512;
	*dos_time = t->tm_sec / 2 + t->tm_min * 32 + t->tm_hour * 2048;
}

static int archive_zip_config(const char *var, const char *value, void *data)
{
	return userdiff_config(var, value);
}

static int write_zip_archive(const struct archiver *ar,
			     struct archiver_args *args)
{
	int err;

	git_config(archive_zip_config, NULL);

	dos_time(&args->time, &zip_date, &zip_time);

	strbuf_init(&zip_dir, 0);

	err = write_archive_entries(args, write_zip_entry);
	if (!err)
		write_zip_trailer(args->commit_sha1);

	strbuf_release(&zip_dir);

	return err;
}

static struct archiver zip_archiver = {
	"zip",
	write_zip_archive,
	ARCHIVER_WANT_COMPRESSION_LEVELS|ARCHIVER_REMOTE
};

void init_zip_archiver(void)
{
	register_archiver(&zip_archiver);
}