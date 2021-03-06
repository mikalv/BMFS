/* BareMetal File System Utility */
/* Written by Ian Seyler of Return Infinity */
/* v1.3.0 (2017 10 11) */

#include <bmfs/entry.h>
#include <bmfs/limits.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

void bmfs_entry_init(struct BMFSEntry *entry)
{
	for (unsigned int i = 0; i < BMFS_FILE_NAME_MAX; i++)
		entry->FileName[i] = 0;

	entry->FileName[0] = 1;

	entry->FileSize = 0;
	entry->StartingBlock = 0;
	entry->ReservedBlocks = 0;
	entry->Type = BMFS_TYPE_FILE;
	entry->Permissions = BMFS_PERMISSION_NONE;
	entry->Unused[0] = 0;
	entry->Unused[1] = 0;
	entry->Unused[2] = 0;
	entry->Unused[3] = 0;
	entry->Unused[4] = 0;
	entry->Unused[5] = 0;
}

int bmfs_entry_cmp_by_filename(const struct BMFSEntry *a,
                               const struct BMFSEntry *b)
{
	if (bmfs_entry_is_empty(a)
	 && bmfs_entry_is_empty(b))
		return 0;

	if (bmfs_entry_is_empty(a))
		return 1;
	else if (bmfs_entry_is_empty(b))
		return -1;

	return bmfs_entry_cmp_filename(a, b->FileName);
}

int bmfs_entry_cmp_by_starting_block(const struct BMFSEntry *a,
                                     const struct BMFSEntry *b)
{
	if (bmfs_entry_is_empty(a)
	 && bmfs_entry_is_empty(b))
		return 0;

	if (bmfs_entry_is_empty(a))
		return 1;
	else if (bmfs_entry_is_empty(b))
		return -1;

	return bmfs_entry_cmp_starting_block(a, b->StartingBlock);
}

int bmfs_entry_cmp_filename(const struct BMFSEntry *entry, const char *filename)
{
	uint64_t i = 0;
	while (1)
	{
		if (entry->FileName[i] > filename[i])
			return 1;
		else if (entry->FileName[i] < filename[i])
			return -1;
		else if (entry->FileName[i] == 0)
			break;
		else if (i >= BMFS_FILE_NAME_MAX)
			return -1;
		else
			i++;
	}

	return 0;
}

int bmfs_entry_cmp_starting_block(const struct BMFSEntry *entry, uint64_t starting_block)
{
	if (entry->StartingBlock > starting_block)
		return 1;
	else if (entry->StartingBlock < starting_block)
		return -1;
	return 0;
}

int bmfs_entry_get_offset(const struct BMFSEntry *entry, uint64_t *offset)
{
	if ((entry == NULL)
	 || (offset == NULL))
		return -EFAULT;

	*offset = entry->StartingBlock*BMFS_BLOCK_SIZE;

	return 0;
}

void bmfs_entry_set_file_name(struct BMFSEntry *entry, const char *filename)
{
	uint64_t i;
	for (i = 0; (i < (BMFS_FILE_NAME_MAX - 1)) && (filename[i] != 0); i++)
		entry->FileName[i] = filename[i];

	for (; i < BMFS_FILE_NAME_MAX; i++)
		entry->FileName[i] = 0;
}

void bmfs_entry_set_starting_block(struct BMFSEntry *entry, uint64_t starting_block)
{
	entry->StartingBlock = starting_block;
}

void bmfs_entry_set_reserved_blocks(struct BMFSEntry *entry, uint64_t reserved_blocks)
{
	entry->ReservedBlocks = reserved_blocks;
}

void bmfs_entry_set_type(struct BMFSEntry *entry, enum BMFSEntryType type)
{
	switch (type)
	{
		case BMFS_TYPE_FILE:
		case BMFS_TYPE_DIRECTORY:
			entry->Type = type;
			break;
		default:
			break;
	}
}

int bmfs_entry_is_directory(const struct BMFSEntry *entry)
{
	return entry->Type == BMFS_TYPE_DIRECTORY;
}

int bmfs_entry_is_empty(const struct BMFSEntry *entry)
{
	return entry->FileName[0] == 1;
}

int bmfs_entry_is_file(const struct BMFSEntry *entry)
{
	return entry->Type == BMFS_TYPE_FILE;
}

int bmfs_entry_is_terminator(const struct BMFSEntry *entry)
{
	return entry->FileName[0] == 0;
}

