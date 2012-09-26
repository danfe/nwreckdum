/* nwreckdum Quake .pak manipulator for linux.
*/
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "shhopt.h"

#define kBufferSize    10000

#ifdef DEBUG
# define D(x)	(x)
#else
# define D(x)
#endif

typedef struct {
	char	name[56];
	uint32_t	offset, length;
} dirrec;

int	asmpak(FILE *);
int	extrpak(FILE *, int);
int	write_rec(dirrec, FILE *);
int	makepath(char *);

void	usage(void) {
	printf(
"Usage: nwreckdum [argument ...]\n"
"\n"
"  -c, --create		create PACK instead of extracting\n"
"  -d, --dir=DIR	use DIR as the input/output directory\n"
"  -l, --list		list files stored in PACK\n"
"  -h, --help		display this help\n"
"  -p, --pak=FILE	read FILE for PACK information\n"
"\n"
"Copyright 1998 by n, aka Daniel Reed.\n"
	);
	exit(0);
}

int	main(int argc, char **argv) {
	FILE	*pakhand = NULL;
	int	makepak = 0, listpak = 0;
	char	*pakfile = NULL, *outdir = NULL;
	const char	defpakfile[] = "pak0.pak",
			defoutdir[] = "pak0";
	optStruct opt[] = {
		{ 'p', "pak",	OPT_STRING,	&pakfile,	0 },
		{ 'd', "dir",	OPT_STRING,	&outdir,	0 },
		{ 'c', "create", OPT_FLAG,	&makepak,	0 },
		{ 'l', "list",	OPT_FLAG,	&listpak,	0 },
		{ 'h', "help",	OPT_FLAG,	usage,	OPT_CALLFUNC },
		{ 0,   NULL,	OPT_END,	0,		0 }
	};

	optParseOptions(&argc, argv, opt, 0);
	if (pakfile == NULL)
		pakfile = (char *)defpakfile;
	if (outdir == NULL)
		outdir = (char *)defoutdir;

	printf("Using pak file: %s\n", pakfile);
	if (!listpak)
		printf("Input/output directory: %s\n", outdir);
	printf("Operation: %s\n", (makepak?"create":listpak?"list":"extract"));

	if (makepak)
		pakhand = fopen(pakfile, "wb");
	else
		pakhand = fopen(pakfile, "rb");
	if (pakhand == NULL) {
		printf("Error opening %s: %s\n", pakfile,
			strerror(errno));
		return(-1);
	}

	if (!listpak) {

	if (!makepak && (mkdir(outdir, 493 /*octal=755*/) == -1)) {
		printf("Error making %s: %s\n", outdir, strerror(errno));
		fclose(pakhand);
		return(-1);
	}
	if (chdir(outdir) == -1) {
		printf("Error changing directories to %s: %s\n", outdir,
			strerror(errno));
		fclose(pakhand);
		return(-1);
	} }

	if (makepak)
		asmpak(pakhand);
	else
		extrpak(pakhand, listpak);

	fclose(pakhand);
	return(0);
}

int	handledir(char *base, dirrec **dir, unsigned long *filecnt) {
	DIR	*workdir = NULL;
	struct dirent	*direntry = NULL;
	struct stat	buf;

	if ((workdir = opendir(".")) == NULL) {
		printf("Unable to open work directory: %s\n",
			strerror(errno));
		return(-1);
	}
	while ((direntry = readdir(workdir)) != NULL) {
		char	basebuf[56];

		if (direntry->d_name[0] == '.')
			continue;
		if (stat(direntry->d_name, &buf) == -1)
			buf.st_mode = 0;

		if ((base != NULL) && (*base != 0))
#ifndef DOS
			snprintf(basebuf, 56, "%s/%s", base,
				direntry->d_name);
#else
		{
			strncpy(basebuf, base, 55);
			strncat(basebuf, "/", 55-strlen(basebuf));
			strncat(basebuf, direntry->d_name,
				55-strlen(basebuf));
		}
#endif /* DOS */
		else {
			strncpy(basebuf, direntry->d_name, 55);
			basebuf[55] = 0;
		}

		if (S_ISDIR(buf.st_mode)) {
			chdir(direntry->d_name);
			handledir(basebuf, dir, filecnt);
			chdir("..");
		} else {
			(*filecnt)++;
			if (*dir == NULL)
				*dir = malloc(sizeof(dirrec));
			else
				*dir = realloc(*dir, sizeof(dirrec) *
					(*filecnt));
			if (*dir == NULL) {
				printf("Memory failure: %s\n",
					strerror(errno));
				return(-1);
			}
#define CURDIRENT	((*dir)[(*filecnt)-1])
			strncpy(CURDIRENT.name, basebuf, 55);
			(CURDIRENT.name)[55] = CURDIRENT.offset =
				CURDIRENT.length = 0;
		}
	}
	closedir(workdir);
	return(1);
}

int	asmpak(FILE *out) {
	unsigned long	filecnt = 0, i = 0;
	dirrec	*dir = NULL;
	FILE	*curfile = NULL;

	fseek(out, 0, SEEK_SET);
	fwrite("PACK", 4, 1, out);
	handledir(NULL, &dir, &filecnt);
	fseek(out, 12, SEEK_SET);
	printf("Num Size     File name\n");
	printf("--- -------- -------------------------------------------------------\n");
	for (i = 0; i < filecnt; i++) {
		char	inc = 0;

		if ((curfile = fopen(dir[i].name, "r")) == NULL) {
			printf("Error opening %s: %s\n", dir[i].name,
				strerror(errno));
			exit(-1);
		}
		dir[i].offset = ftell(out);
		fseek(curfile, 0, SEEK_END);
		dir[i].length = ftell(curfile);
		printf("%3lu %8lu %s\n", i, dir[i].length, 
			dir[i].name);
		fseek(curfile, 0, SEEK_SET);
		while (!feof(curfile)) {
			fread(&inc, 1, 1, curfile);
			fwrite(&inc, 1, 1, out);
		}
		fclose(curfile);
	}
	{
		int	cnt = 0;
		unsigned long	offtmp = ftell(out);

		D(printf("offtmp = %lu\n", offtmp));
		for (cnt = 0; cnt < filecnt; cnt++)
			fwrite(&(dir[cnt]), sizeof(dirrec), 1, out);
		fseek(out, 4, SEEK_SET);
		fwrite(&offtmp, 4, 1, out);
		offtmp = filecnt*sizeof(dirrec);
		D(printf("offtmp = %lu\n", offtmp));
		fwrite(&offtmp, 4, 1, out);
	}
	free(dir);
	return(1);
}

int	extrpak(FILE *in, int listpak) {
	dirrec	*dir = NULL;
	char	signature[4];
	unsigned long	dir_start = 0, num_ents = 0;
	int	i = 0;

	fread((void *)signature, 4, 1, in);
	if (strncmp(signature, "PACK", 4) != 0) {
		fprintf(stderr, "Error: not a PAK file\n");
		fclose(in);
		return(-1);
	}

	fread((void *)&dir_start, 4, 1, in);
	D(printf("dir_start: %lu\n", dir_start));
	fread((void *)&num_ents, 4, 1, in);
	D(printf("num_ents: %lu\n", num_ents));
	num_ents /= sizeof(dirrec);
	D(printf("num_ents: %lu\n", num_ents));
	fseek(in, dir_start, SEEK_SET);
	D(printf("ftell: %lu\n", ftell(in)));

	if ((dir = calloc(num_ents, sizeof(dirrec))) == NULL) {
		printf("Error allocating memory: %s\n", strerror(errno));
		fclose(in);
		exit(-1);
	}

	fread((void *)dir, sizeof(dirrec), num_ents, in);

	printf("Num Size     File name\n");
	printf("--- -------- -------------------------------------------------------\n");
	for (i = 0; i < num_ents; i++) {
		printf("%3i %8lu %s\n", i, dir[i].length, 
			dir[i].name);
		if (listpak)
			continue;
		if (write_rec(dir[i], in))
			printf("Error writing record: %s\n",
				strerror(errno));
	}

	free(dir);
	return(0);
}

int	write_rec(dirrec dir, FILE *in) {
	char	buf[kBufferSize];
	FILE	*f = NULL;
	long	l = 0;
	int	blocks = 0, count = 0, s = 0;

	makepath(dir.name);
	if ((f = fopen(dir.name,"wb")) == NULL) {
		printf("Error opening %s: %s\n", dir.name,
			strerror(errno));
		return(-1);
	}
	fseek(in, dir.offset, SEEK_SET);
	blocks = dir.length / (kBufferSize-1);
	if (dir.length%(kBufferSize-1))
		blocks++;
	for (count = 0; count < blocks; count++) {
		fread((void *)buf, 1, (kBufferSize-1), in);
		if ((l = dir.length - (long)count * (kBufferSize-1)) >
			(kBufferSize-1))
			l = (kBufferSize-1);
		s = l;
		fwrite((void *)buf, 1, s, f);
	}
	fclose(f);
	return(0);
}

int	makepath(char *fname) {
/* given "some/path/to/file", makes all dirs (ie "some/", "some/path/", */
/* and "some/path/to/", so that file can be created                     */
	char	*c = fname,
		*o = (char *)malloc(strlen(fname)+1),
		*n = o;

	while(*c) {
		while(*c && (*c != '/'))
			*n++ = *c++;
		if(*c) {
			*n = '\0';
			mkdir(o, 493 /*octal=755*/);
			*n = '/';
			n++;
			c++;
		}
	}
	free(o);
	return(0);
}
