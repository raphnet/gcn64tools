#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xferpak.h"
#include "xferpak_tools.h"

static int showProgress(xferpak_progress *progress)
{
	printf("%s : 0x%04x / 0x%04x (%.2f%%)\r",
			progress->caption,
			progress->cur_addr,
			progress->max_addr,
			progress->cur_addr / (float)progress->max_addr * 100.0);
	fflush(stdout);

	return 0;
}

int gcn64lib_xferpak_writeRAM_from_file(gcn64_hdl_t hdl, int channel, const char *input_filename, int verify)
{
	xferpak *xpak;
	unsigned char *mem;
	long mem_size;
	FILE *fptr;
	struct gbcart_info cartinfo;
	int res;
	xferpak_progress progress = {
		.caption = "Writing RAM...",
		.update = showProgress,
	};

	/* Load file */
	fptr = fopen(input_filename, "rb");
	if (!fptr) {
		perror("fopen");
		return -1;
	}

	fseek(fptr, 0, SEEK_END);
	mem_size = ftell(fptr);
	fseek(fptr, 0, SEEK_SET);

	mem = malloc(mem_size);
	if (!mem) {
		perror("could not allocate buffer to load file");
		fclose(fptr);
		return -1;
	}
	if (1 != fread(mem, mem_size, 1, fptr)) {
		perror("could not read file");
		free(mem);
		fclose(fptr);
		return -1;
	}
	fclose(fptr);

	printf("Loaded '%s' (%ld bytes)\n", input_filename, mem_size);

	/* Prepare xferpak */
	xpak = gcn64lib_xferpak_init(hdl, channel);
	if (!xpak) {
		free(mem);
		return -1;
	}

	/* Verify cartridge presence, check header to confirm presence of RAM and
	 * make sure the size matches the file size. */
	res = xferpak_gb_readInfo(xpak, &cartinfo);
	if (res < 0) {
		fprintf(stderr, "Failed to read cartridge header\n");
		xferpak_free(xpak);
		free(mem);
		return -1;
	}

	if (!(cartinfo.flags & GB_FLAG_RAM)) {
		fprintf(stderr, "Current cartridge does not have RAM\n");
		xferpak_free(xpak);
		free(mem);
		return -1;
	}

	if (!(cartinfo.flags & GB_FLAG_BATTERY)) {
		fprintf(stderr, "Warning: Current cartridge does not have a battery. Writing probably makes no sense...\n");
	}

	if (mem_size != cartinfo.ram_size) {
		// TODO : Detect gz files (for mednafen .sav format)
		fprintf(stderr, "File size does not match cartridge memory size\n");
		xferpak_free(xpak);
		free(mem);
		return -1;
	}

	xferpak_setProgressStruct(xpak, &progress);

	/* Do the writing */
	res = xferpak_gb_writeRAM(xpak, mem_size, mem);
	if (res < 0) {
		xferpak_free(xpak);
		free(mem);
		return res;
	}
	printf("\n");

	/* Optionally read it back to verify */
	if (verify) {
		unsigned char *verify_buffer;
		int verify_size;
		progress.caption = "Verifying RAM...";

		verify_size = xferpak_gb_readRAM(xpak, &cartinfo, &verify_buffer);
		if (verify_buffer < 0) {
			fprintf(stderr, "Verify failed\n");
			xferpak_free(xpak);
			free(mem);
			return verify_size;
		}
		if (verify_size != mem_size) {
			fprintf(stderr, "Verify read size mismatch\n");
			xferpak_free(xpak);
			free(mem);
			return -1;
		}
		if (memcmp(verify_buffer, mem, mem_size)) {
			fprintf(stderr, "Verify failed\n");
			xferpak_free(xpak);
			free(mem);
			return -1;
		}
		free(verify_buffer);
		printf("\nVerify ok\n");
	}

	xferpak_free(xpak);
	free(mem);

	return 0;
}

int gcn64lib_xferpak_readRAM_to_file(gcn64_hdl_t hdl, int channel, const char *output_filename)
{
	xferpak *xpak;
	unsigned char *mem;
	int mem_size;
	struct gbcart_info cartinfo;
	xferpak_progress progress = {
		.caption = "Reading RAM...",
		.update = showProgress,
	};

	xpak = gcn64lib_xferpak_init(hdl, channel);
	if (!xpak)
		return -1;

	xferpak_setProgressStruct(xpak, &progress);

	mem_size = xferpak_gb_readRAM(xpak, &cartinfo, &mem);
	if (mem_size < 0) {
		xferpak_free(xpak);
		return mem_size;
	}
	printf("\n");

	if (mem_size > 0) {
		FILE *fptr;
		fptr = fopen(output_filename, "wb");
		if (fptr) {
			fwrite(mem, mem_size, 1, fptr);
			fclose(fptr);
		} else {
			perror("fopen");
		}
	}

	xferpak_free(xpak);

	return 0;
}

int gcn64lib_xferpak_readROM_to_file(gcn64_hdl_t hdl, int channel, const char *output_filename)
{
	xferpak *xpak;
	unsigned char *mem;
	int mem_size;
	struct gbcart_info cartinfo;
	xferpak_progress progress = {
		.caption = "Reading ROM...",
		.update = showProgress,
	};

	xpak = gcn64lib_xferpak_init(hdl, channel);
	if (!xpak)
		return -1;

	xferpak_setProgressStruct(xpak, &progress);

	mem_size = xferpak_gb_readROM(xpak, &cartinfo, &mem);
	if (mem_size < 0) {
		xferpak_free(xpak);
		return mem_size;
	}
	printf("\n");

	if (mem_size > 0) {
		FILE *fptr;
		fptr = fopen(output_filename, "wb");
		if (fptr) {
			fwrite(mem, mem_size, 1, fptr);
			fclose(fptr);
		} else {
			perror("fopen");
		}
	}

	xferpak_free(xpak);

	return 0;
}

int gcn64lib_xferpak_printInfo(gcn64_hdl_t hdl, int channel)
{
	xferpak *xpak;
	struct gbcart_info cartinfo;
	int res;

	xpak = gcn64lib_xferpak_init(hdl, channel);
	if (!xpak)
		return -1;

	res = xferpak_gb_readInfo(xpak, &cartinfo);
	if (res < 0) {
		xferpak_free(xpak);
		return -1;
	}

	gbcart_printInfo(&cartinfo);

	xferpak_free(xpak);

	return 0;
}

