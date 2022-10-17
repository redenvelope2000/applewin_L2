/*********************************************************************

  unzip.c

  Support for retrieving files from zipfiles.

*********************************************************************/
#include "StdAfx.h"
#include "Core.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "wincompat.h"

#include <stdio.h>
#include "loadfile.h"


#define dword   UINT32
#define word    UINT16
#define byte    UINT8

#define BUFSIZE	0x100000

#define LSB_FIRST

#ifdef LSB_FIRST
#define intelWord(x) (x)
#define intelLong(x) (x)
#else
#define intelWord(x) ((x << 8) | (x >> 8))
#define intelLong(x) (((x << 24) | (((unsigned long) x) >> 24) | (( x & 0x0000ff00) << 8) | (( x & 0x00ff0000) >> 8)))
#endif

//#define ERRORMSG //
static void ERRORMSG (const char *str, ...)
{}

typedef struct
{
	dword	end_of_cent_dir_sig;
	word	number_of_this_disk;
	word	number_of_disk_start_cent_dir;
	word	total_entries_cent_dir_this_disk;
	word	total_entries_cent_dir;
	dword	size_of_cent_dir;
	dword	offset_to_start_of_cent_dir;
	word	zipfile_comment_length;
	char	zipfile_comment[1];
} t_end_of_cent_dir;

/* offsets in end of central directory structure */
#define ZIPESIG		0x00
#define ZIPEDSK		0x04
#define ZIPECEN		0x06
#define ZIPENUM		0x08
#define ZIPECENN	0x0a
#define ZIPECSZ		0x0c
#define ZIPEOFST	0x10
#define ZIPECOML	0x14
#define ZIPECOM		0x16

typedef struct
{
	dword	local_file_header_sig;
	word	version_needed_to_extract;
	word	general_purpose_bit_flag;
	word	compression_method;
	word	last_mod_file_time;
	word	last_mod_file_date;
	dword	crc32;
	dword	compressed_size;
	dword	uncompressed_size;
	word	filename_length;
	word	extra_field_length;
	char	*filename;
} t_local_file_hdr;

/* offsets in local file header structure */
#define ZIPLOCSIG	0x00
#define ZIPVER		0x04
#define ZIPGENFLG	0x06
#define ZIPMTHD		0x08
#define ZIPTIME		0x0a
#define ZIPDATE		0x0c
#define ZIPCRC		0x0e
#define ZIPSIZE		0x12
#define ZIPUNCMP	0x16
#define ZIPFNLN		0x1a
#define ZIPXTRALN	0x1c
#define ZIPNAME		0x1e

typedef struct
{
	dword	cent_file_header_sig;
	byte	version_made_by;
	byte	host_os;
	byte	version_needed_to_extract;
	byte	os_needed_to_extract;
	word	general_purpose_bit_flag;
	word	compression_method;
	word	last_mod_file_time;
	word	last_mod_file_date;
	dword	crc32;
	dword	compressed_size;
	dword	uncompressed_size;
	word	filename_length;
	word	extra_field_length;
	word	file_comment_length;
	word	disk_number_start;
	word	internal_file_attrib;
	dword	external_file_attrib;
	dword	offset_lcl_hdr_frm_frst_disk;
	char	*filename;
} t_central_dir_ent;

/* offsets in central directory entry structure */
#define ZIPCENSIG	0x0
#define ZIPCVER		0x4
#define ZIPCOS		0x5
#define	ZIPCVXT		0x6
#define	ZIPCEXOS	0x7
#define ZIPCFLG		0x8
#define ZIPCMTHD	0xa
#define ZIPCTIM		0xc
#define ZIPCDAT		0xe
#define ZIPCCRC		0x10
#define ZIPCSIZ		0x14
#define ZIPCUNC		0x18
#define ZIPCFNL		0x1c
#define ZIPCXTL		0x1e
#define ZIPCCML		0x20
#define ZIPDSK		0x22
#define ZIPINT		0x24
#define ZIPEXT		0x26
#define ZIPOFST		0x2a
#define ZIPCFN		0x2e

/* public functions */
int /* error */ load_zipped_file (const char *zipfile, const char *filename,
	unsigned char **buf, int *length);

/* private functions */
static int read_local_file_header (mem_FILE *fp, const t_central_dir_ent *cd, t_local_file_hdr *lfh);

static int find_matching_cd_entry (mem_FILE *fp, char *match,
	t_end_of_cent_dir *ecd, t_central_dir_ent *cd);
static int compare_filename (char *filename, char *match);
static void read_central_dir_entry (char *buf, t_central_dir_ent *cd);

static int read_end_of_cent_dir (mem_FILE *fp, t_end_of_cent_dir *ecd);
static int find_ecd_sig (char *buffer, int buflen, int *offset);
static int get_file_length (mem_FILE *fp, long *length);
static word read_word (char *buf);
static dword read_dword (char *buf);
static int create_input_buffer (mem_FILE *fp, const t_central_dir_ent *cd,
	const t_local_file_hdr *lfh, unsigned char **inbuf);

#if ZIPDEBUG
static debug_print_local_file_hdr (const t_local_file_hdr *lfh);
static debug_print_central_dir_entry (t_central_dir_ent *cd);
static debug_print_end_central_dir (t_end_of_cent_dir *ecd);
#endif

static long	gZipLen;			/* length of zipfile */
static char input_buffer[BUFSIZE];	/* small input buffer */

/*----------------------------------
        inflate.c support
----------------------------------*/
extern int _old_style_inflate(void);

unsigned char *slide;			/* 32K sliding window for inflate.c */
unsigned char *g_nextbyte;		/* pointer to next byte of input */
static unsigned char *g_outbuf;	/* pointer to next byte in output buffer */

void inflate_FLUSH (unsigned char *buffer, unsigned long n)
{
	memcpy (g_outbuf, buffer, n);
	g_outbuf += n;
}

/*----------------------------------
     end of inflate.c support
----------------------------------*/

/*	Pass the path to the zipfile and the name of the file within the zipfile.
	buf will be set to point to the uncompressed image of that zipped file.
	length will be set to the length of the uncompressed data. */
int /* error */ load_zipped_file (const char *zipfile, const char *filename,
	unsigned char **buf, int *length)
{
	mem_FILE 				*fp = NULL;
	t_end_of_cent_dir	ecd;
	t_central_dir_ent	cd;
	t_local_file_hdr	lfh;
	unsigned char		*inbuf = 0, *outbuf = 0;
	char				filenameUpper[32], *p;
	int 				err;

	/* open zipfile for binary read */
	if ((fp = mem_fopen (zipfile,"rb")) != NULL)
	{
		/* determine length of zip file */
		err = get_file_length (fp, &gZipLen);
		if (err!=0)
		{
			ERRORMSG ("Error in zipfile: get_file_length() failed\n");
			goto bail;
		}

		/* read end-of-central-directory */
		err = read_end_of_cent_dir (fp, &ecd);
		if (err!=0)
		{
			ERRORMSG ("Error reading 'end of central directory'\n");
			goto bail;
		}

		/* verify that we can work with this zipfile (no disk spanning allowed) */
		if ((ecd.number_of_this_disk != ecd.number_of_disk_start_cent_dir) ||
			(ecd.total_entries_cent_dir_this_disk != ecd.total_entries_cent_dir) ||
			(ecd.total_entries_cent_dir < 1))
		{
			err = -1;
			ERRORMSG ("Unsupported zipfile: zipfile cannot span disks\n");
			goto bail;
		}

		/* find matching file in central directory (force upper case) */
		for (p=filenameUpper; (*p++ = toupper(*filename++)) != '\0';){};

		err = find_matching_cd_entry (fp, filenameUpper, &ecd, &cd);
		if (err!=0)
		{
//         ERRORMSG("Could not find %s in zipfile %s\n", filenameUpper, zipfile);
			goto bail;
		}

		/* read in local file header */
		err = read_local_file_header (fp, &cd, &lfh);
		if (err!=0)
		{
			ERRORMSG ("Error reading 'local file header'\n");
			goto bail;
		}

		/* extract file based on compression method */
		if (lfh.compression_method == 0x0000)
		{
			/* file is not compressed, simply stored -- copy directly to output buffer */
			err = create_input_buffer (fp, &cd, &lfh, &outbuf);
			if (err!=0)
				ERRORMSG ("Couldn't extract uncompressed file\n");
		}
		else if (lfh.compression_method == 0x0008)
		{
			/* file is compressed using "Deflate" method */

			/* create input and output buffers */
			err = create_input_buffer (fp, &cd, &lfh, &inbuf);
			if (err==0)
			{
				g_nextbyte = inbuf;
				outbuf = (unsigned char*)malloc (lfh.uncompressed_size);
				if (outbuf!=0)
					g_outbuf = outbuf;
				else
				{
					ERRORMSG ("Couldn't allocate %d bytes for output buffer\n",
						lfh.uncompressed_size);
					err = -1;
				}
			}
			else
			{
            ERRORMSG ("Could not create input buffer\n");
			}

			/* create sliding window for inflate() */
			if (err==0)
			{
				slide = (unsigned char*)malloc (0x8000);
				if (slide==0)
				{
					ERRORMSG ("Could not create 32K sliding window\n");
					err = -1;
				}
			}

			/* inflate the compressed file (now in memory) */
			if (err==0)
			{
				err = _old_style_inflate ();
				if (err!=0)
				{
					ERRORMSG ("Error inflating compressed file: %d", err);
				}
			}
		}

		/* return pointer to uncompressed data */
		if (err==0)
		{
			*buf = outbuf;
			*length = lfh.uncompressed_size;
			outbuf = 0;		/* prevent data from being freed() */
		}
	}
	else
	{
		ERRORMSG ("Could not open zipfile %s\n", zipfile);
		err = -1;
	}

bail:
	if (fp) mem_fclose (fp);
	if (inbuf) free (inbuf);
	if (outbuf) free (outbuf);
	if (slide)
	{
		free (slide);
		slide = 0;
	}
	return err;
}


/* malloc() and fill input buffer with compressed file data (all of it) */
static int create_input_buffer (mem_FILE *fp, const t_central_dir_ent *cd,
	const t_local_file_hdr *lfh, unsigned char **inbuf)
{
	long	count, offset, read;
	int		err;

	/* create a buffer the same size as the compressed file */
	count = lfh->compressed_size;
	*inbuf = (unsigned char*)malloc (count);
	if (*inbuf==0)
	{
		ERRORMSG ("Couldn't allocate %ld bytes for input buffer\n", count);
		return -1;
	}

	/* calculate offset to data and fseek() there */
	offset = cd->offset_lcl_hdr_frm_frst_disk + ZIPNAME +
		lfh->filename_length + lfh->extra_field_length;
	err = mem_fseek (fp, offset, mem_SEEK_SET);

	/* fill input buffer with compressed file data */
	if (err==0)
	{
		read = mem_fread (*inbuf, sizeof (unsigned char), count, fp);
		if (read!=count)
		{
			ERRORMSG ("Error in zipfile: couldn't read %ld bytes of compressed data\n", count);
			err = -1;
		}
	}
	else
		ERRORMSG ("Error reading zipfile: fseek to compressed data failed\n");

	/* clean up if errors */
	if (err!=0 && *inbuf!=0)
	{
		free (*inbuf);
		*inbuf = 0;
	}
	return err;
}

static int read_local_file_header (mem_FILE *fp, const t_central_dir_ent *cd, t_local_file_hdr *lfh)
{
	long	count, read;
	int		err;

	/* we'll read entire local file header (up to BUFSIZE) or to end of file */
	if (BUFSIZE > (gZipLen - cd->offset_lcl_hdr_frm_frst_disk))
		count = gZipLen - cd->offset_lcl_hdr_frm_frst_disk;
	else
		count = BUFSIZE;

	err = mem_fseek (fp, cd->offset_lcl_hdr_frm_frst_disk, mem_SEEK_SET);
	if (err==0)
	{
		read = mem_fread (input_buffer, sizeof (unsigned char), count, fp);
		if (read!=count)
		{
			err = -1;
			ERRORMSG ("Error in zipfile: couldn't read %ld bytes from local file header", count);
		}
	}
	else
		ERRORMSG ("Error in zipfile: couldn't fseek to local file header\n");

	if (err==0)
	{
		lfh->local_file_header_sig = read_dword (input_buffer+ZIPLOCSIG);
		lfh->version_needed_to_extract = read_word (input_buffer+ZIPVER);
		lfh->general_purpose_bit_flag = read_word (input_buffer+ZIPGENFLG);
		lfh->compression_method = read_word (input_buffer+ZIPMTHD);
		lfh->last_mod_file_time = read_word (input_buffer+ZIPTIME);
		lfh->last_mod_file_date = read_word (input_buffer+ZIPDATE);
		lfh->crc32 = read_dword (input_buffer+ZIPCRC);
		lfh->compressed_size = read_dword (input_buffer+ZIPSIZE);
		lfh->uncompressed_size = read_dword (input_buffer+ZIPUNCMP);
		lfh->filename_length = read_word (input_buffer+ZIPFNLN);
		lfh->extra_field_length = read_word (input_buffer+ZIPXTRALN);
		lfh->filename = input_buffer+ZIPNAME;
	}
	return err;
}

#if ZIPDEBUG
static debug_print_local_file_hdr (const t_local_file_hdr *lfh)
{
	printf ("LOCAL FILE HEADER\n");
	printf ("Local file header signature: %08x\n", lfh->local_file_header_sig);
	printf ("Version needed to extract: %04x\n", lfh->version_needed_to_extract);
	printf ("General purpose bit flag: %04x\n", lfh->general_purpose_bit_flag);
	printf ("Compression method: %04x\n", lfh->compression_method);
	printf ("Last mod file time: %04x\n", lfh->last_mod_file_time);
	printf ("Last mod file date: %04x\n", lfh->last_mod_file_date);
	printf ("CRC-32: %08x\n", lfh->crc32);
	printf ("Compressed size: %08x\n", lfh->compressed_size);
	printf ("Uncompressed size: %08x\n", lfh->uncompressed_size);
	printf ("Filename length: %04x\n", lfh->filename_length);
	printf ("Extra field length: %04x\n", lfh->extra_field_length);
}
#endif

/* Known issue: central dir must be < BUFSIZE bytes */
static int find_matching_cd_entry (mem_FILE *fp, char *match,
	t_end_of_cent_dir *ecd, t_central_dir_ent *cd)
{
	int		i, j, found = 0, err;
	long	count, read;
	char 	*p;

	/* we'll read the entire central directory or BUFSIZE bytes, whichever is smaller */
	if (ecd->size_of_cent_dir > BUFSIZE)
		count = BUFSIZE;
	else
		count = ecd->size_of_cent_dir;

	/* read from start of central directory */
	err = mem_fseek (fp, ecd->offset_to_start_of_cent_dir, mem_SEEK_SET);
	if (err==0)
	{
		read = mem_fread (input_buffer, sizeof (unsigned char), count, fp);
		if (read!=count)
		{
			ERRORMSG ("Error in zipfile: couldn't read %ld bytes from central directory\n", count);
			err = -1;
		}
	}
	else
		ERRORMSG ("Error in zipfile: couldn't fseek to start of central directory\n");

	if (err==0)
	{
		/* loop through entries in central directory for suitable match */
		for (i=0, p=input_buffer; found==0 && i<ecd->total_entries_cent_dir; i++)
		{
			char filename[256];

			read_central_dir_entry (p, cd);

			/* copy filename and force to upper case */
			for (j=0; j<cd->filename_length && j<254; j++)
				filename[j] = toupper (cd->filename[j]);
			filename[j] = '\0';

			/* compare filename */
			if (!compare_filename (filename, match))
			{
				found = 1;

				/* check for suitability */
				if (cd->compression_method != 0x0000 &&
					cd->compression_method != 0x0008 )
				{
					found = 0;
					ERRORMSG ("Error in zipfile: compression method for file %s unsupported.\n",
						match);
					ERRORMSG ("Method: $%04x  must be $0000 (Stored) or $0008 (Deflated)\n",
						cd->compression_method);
				}
				if	(cd->version_needed_to_extract > 0x14)
				{
					found = 0;
					ERRORMSG ("Error in zipfile: version for file %s too new.\n", match);
					ERRORMSG ("Version: $%02x must be $14 or less\n",
						cd->version_needed_to_extract);
				}
				if (cd->os_needed_to_extract != 0x00)
				{
					found = 0;
					ERRORMSG ("Error in zipfile: OS for file %s not supported.\n",
						match);
					ERRORMSG ("OS: $%02x must be $00\n",cd->os_needed_to_extract);
				}
				if (cd->disk_number_start != ecd->number_of_this_disk)
				{
					found = 0;
					ERRORMSG ("Error in zipfile: zipfile cannot span disks\n");
				}
			}

			/* skip to next entry in central dir */
			p += ZIPCFN + cd->filename_length + cd->extra_field_length
				+ cd->file_comment_length;
		}
	}

	if (!found) err = -1;
	return err;
}

/* check match against filename -- check within subdirectory paths */
static int compare_filename (char *filename, char *match)
{
	char	*lasttoken, *token;
	char	temp[256];

  //nao+: "*" for matching anything
  if (strcmp ("*", match) == 0) {
    //printf ("* filename=%s\n", filename);
    return 0;
  }
  
	/* check for exact match first */
	if (strlen (filename) == strlen (match))
		return strcmp (filename, match);

	/* check that this file entry is not a directory */
	if (filename[strlen (filename) - 1] == '/')
		return 1;

	/* now check to see if file is stored with path */
	strcpy (temp, filename);

	/* spin to last word between forward slashes */
	for (lasttoken=token=strtok (temp, "/"); token!=NULL; token=strtok (NULL, "/"))
		lasttoken = token;

	if (lasttoken==NULL)
		return 1;
	else
		return strcmp (lasttoken, match);
}

#if ZIPDEBUG
static debug_print_central_dir_entry (t_central_dir_ent *cd)
{
	printf ("CENTRAL DIRECTORY ENTRY\n");
	printf ("Central file header sig: %08x\n", cd->cent_file_header_sig );
	printf ("Version made by: %02x\n", cd->version_made_by );
	printf ("Host OS: %02x\n", cd->host_os );
	printf ("Version needed to extract: %02x\n", cd->version_needed_to_extract );
	printf ("OS needed to extract: %02x\n", cd->os_needed_to_extract );
	printf ("General purpose bit flag: %04x\n", cd->general_purpose_bit_flag );
	printf ("Compression Method: %04x\n", cd->compression_method );
	printf ("Last mod file time: %04x\n", cd->last_mod_file_time );
	printf ("Last mod file date: %04x\n", cd->last_mod_file_date );
	printf ("CRC-32: %08x\n", cd->crc32 );
	printf ("Compressed size: %08x\n", cd->compressed_size );
	printf ("Uncompressed size: %08x\n", cd->uncompressed_size );
	printf ("Filename length: %04x\n", cd->filename_length );
	printf ("Extra field length: %04x\n", cd->extra_field_length );
	printf ("File comment length: %04x\n", cd->file_comment_length );
	printf ("Disk number start: %04x\n", cd->disk_number_start );
	printf ("Internal file attributes: %04x\n", cd->internal_file_attrib );
	printf ("External file attributes: %08x\n", cd->external_file_attrib );
	printf ("Offset to local header: %08x\n", cd->offset_lcl_hdr_frm_frst_disk );
	printf ("\n");
}
#endif

static void read_central_dir_entry (char *buf, t_central_dir_ent *cd)
{
	cd->cent_file_header_sig = read_dword (buf+ZIPCENSIG);
	cd->version_made_by = *(buf+ZIPCVER);
	cd->host_os = *(buf+ZIPCOS);
	cd->version_needed_to_extract = *(buf+ZIPCVXT);
	cd->os_needed_to_extract = *(buf+ZIPCEXOS);
	cd->general_purpose_bit_flag = read_word (buf+ZIPCFLG);
	cd->compression_method = read_word (buf+ZIPCMTHD);
	cd->last_mod_file_time = read_word (buf+ZIPCTIM);
	cd->last_mod_file_date = read_word (buf+ZIPCDAT);
	cd->crc32 = read_dword (buf+ZIPCCRC);
	cd->compressed_size = read_dword (buf+ZIPCSIZ);
	cd->uncompressed_size = read_dword (buf+ZIPCUNC);
	cd->filename_length = read_word (buf+ZIPCFNL);
	cd->extra_field_length = read_word (buf+ZIPCXTL);
	cd->file_comment_length = read_word (buf+ZIPCCML);
	cd->disk_number_start = read_word (buf+ZIPDSK);
	cd->internal_file_attrib = read_word (buf+ZIPINT);
	cd->external_file_attrib = read_dword (buf+ZIPEXT);
	cd->offset_lcl_hdr_frm_frst_disk = read_dword (buf+ZIPOFST);
	cd->filename = (buf+ZIPCFN);
}

static int read_end_of_cent_dir (mem_FILE *fp, t_end_of_cent_dir *ecd)
{
	long	count, read;
	int		sigoffset;
	int		err;

	/* determine how many bytes to read */
	if (gZipLen > BUFSIZE)
		count = BUFSIZE;
	else
		count = gZipLen;

	/* seek to end of file - count */
	err = mem_fseek (fp, -count, mem_SEEK_END);
	if (err!=0)
	{
		ERRORMSG ("Error in zipfile: fseek failed\n");
		return err;
	}

	/* read entire file or BUFSIZE bytes into buffer */
	read = mem_fread (input_buffer, sizeof (unsigned char), count, fp);
	if (read==count)
	{
		/* locate end-of-central-dir sig */
		err = find_ecd_sig (input_buffer, count, &sigoffset);
		if (err==0)
		{
			char *p = input_buffer+sigoffset;

			/* read end-of-central-dir */
			ecd->end_of_cent_dir_sig = read_dword (p+ZIPESIG);
			ecd->number_of_this_disk = read_word (p+ZIPEDSK);
			ecd->number_of_disk_start_cent_dir = read_word (p+ZIPECEN);
			ecd->total_entries_cent_dir_this_disk = read_word (p+ZIPENUM);
			ecd->total_entries_cent_dir = read_word (p+ZIPECENN);
			ecd->size_of_cent_dir = read_dword (p+ZIPECSZ);
			ecd->offset_to_start_of_cent_dir = read_dword (p+ZIPEOFST);
			ecd->zipfile_comment_length = read_word (p+ZIPECOML);
		}
		else
			ERRORMSG ("Error in zipfile: couldn't find 'end of central dir' signature\n");
	}
	else
	{
		ERRORMSG ("Error in zipfile: couldn't read %ld bytes from end of file\n", count);
		err = -1;
	}
	return err;
}

#if ZIPDEBUG
static debug_print_end_central_dir (t_end_of_cent_dir *ecd)
{
	printf ("END OF CENTRAL DIRECTORY\n");
	printf ("End of central dir signature: %08x \n", ecd->end_of_cent_dir_sig);
	printf ("Number of this disk: %04x \n", ecd->number_of_this_disk );
	printf ("Number of disk with start of central dir: %04x \n",
		ecd->number_of_disk_start_cent_dir );
	printf ("Total entries in central dir on this disk: %04x \n",
		ecd->total_entries_cent_dir_this_disk);
	printf ("Total entries in central dir: %04x \n", ecd->total_entries_cent_dir);
	printf ("Size of central directory: %08x \n", ecd->size_of_cent_dir);
	printf ("Offset to start of central dir: %08x \n", ecd->offset_to_start_of_cent_dir);
	printf ("Zipfile comment length: %04x \n", ecd->zipfile_comment_length);
	printf ("\n");
}
#endif

/* locate end-of-central-dir sig in buffer and return offset */
/* Known issue: ECD sig must be within the last 4K (BUFSIZE) of the zipfile */
static int find_ecd_sig (char *buffer, int buflen, int *offset)
{
	static char ecdsig[] = { 'P', 'K', 0x05, 0x06 };
	int	i, found = 0;

	for (i=buflen-22; i>=0; i--)
	{
		if (!strncmp (buffer+i, ecdsig, 4))
		{
			found = 1;
			*offset = i;
			break;
		}
	}
	return (!found);
}

static int get_file_length (mem_FILE *fp, long *length)
{
	int err;

	/* determine length of file */
	err = mem_fseek (fp, 0L, mem_SEEK_END);
	if (err==0)
	{
		*length = mem_ftell (fp);
		if (*length == -1L)
			err = -1;
	}
	return err;
}

/* use these to avoid structure padding and byte-ordering problems */
static word read_word (char *buf)
{
	return intelWord (*(word *)buf);
}

/* use these to avoid structure padding and byte-ordering problems */
static dword read_dword (char *buf)
{
	return intelLong (*(dword *)buf);
}
