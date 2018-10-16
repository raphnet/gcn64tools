#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xferpak.h"
#include "xferpak_tools.h"
#include "zlib.h"
#include "uiio.h"

static int memcmp_lownibbles(const uint8_t *a, const uint8_t *b, int n_bytes)
{
	while(n_bytes--) {
		if ((*a&0xf) != (*b&0xf))
			return 1;
		a++;
		b++;
	}
	return 0;
}

int gcn64lib_xferpak_writeRAM_from_file(rnt_hdl_t hdl, int channel, const char *input_filename, int verify, uiio *u)
{
	xferpak *xpak;
	unsigned char *mem;
	int mem_size, read_size;
	gzFile fptr;
	struct gbcart_info cartinfo;
	int res;
	u = getUIIO(u);
	u->caption = "Writing RAM...",
	u->multi_progress = verify;

	/* Prepare xferpak */
	xpak = gcn64lib_xferpak_init(hdl, channel, u);
	if (!xpak) {
		return -1;
	}

	/* Verify cartridge presence, check header to confirm presence of RAM and
	 * (uncompressed) file size to expect. */
	res = xferpak_gb_readInfo(xpak, &cartinfo);
	if (res < 0) {
		u->error("Failed to read cartridge header");
		xferpak_free(xpak);
		return -1;
	}


	if (cartinfo.type != GB_TYPE_POCKET_CAMERA) {
		if (!(cartinfo.flags & GB_FLAG_RAM)) {
			u->error("Current cartridge does not have RAM");
			xferpak_free(xpak);
			return -1;
		}

		if (!(cartinfo.flags & GB_FLAG_BATTERY)) {
			u->error("Warning: Current cartridge does not have a battery. Writing probably makes no sense...");
		}
	}

	/* Allocate memory buffer */
	mem_size = cartinfo.ram_size;
	mem = malloc(mem_size);
	if (!mem) {
		u->perror("could not allocate buffer to load file");
		xferpak_free(xpak);
		return -1;
	}

	/* Load file */
	fptr = gzopen(input_filename, "rb");
	if (!fptr) {
		u->perror("fopen");
		free(mem);
		xferpak_free(xpak);
		return -1;
	}

	read_size = gzread(fptr, mem, mem_size);
	if (read_size < 0) {
		u->error("Failed to read file: %s\n", gzerror(fptr, NULL));
		gzclose(fptr);
		free(mem);
		xferpak_free(xpak);
		return -1;
	}

	gzclose(fptr);

	if (mem_size != cartinfo.ram_size) {
		u->error("File size does not match cartridge memory size\n");
		free(mem);
		xferpak_free(xpak);
		return -1;
	}

	printf("Loaded '%s' (%d bytes)\n", input_filename, mem_size);

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
		u->caption = "Verifying RAM...";
		u->multi_progress = 0;

		verify_size = xferpak_gb_readRAM(xpak, &cartinfo, &verify_buffer);
		if (verify_buffer < 0) {
			u->error("Verify failed\n");
			xferpak_free(xpak);
			free(mem);
			return verify_size;
		}
		if (verify_size != mem_size) {
			u->error("Verify read size mismatch\n");
			xferpak_free(xpak);
			free(mem);
			return -1;
		}
		if (GB_MBC_MASK(cartinfo.flags) == GB_FLAG_MBC2) {
			if (memcmp_lownibbles(verify_buffer, mem, mem_size)) {
				u->error("Verify failed\n");
				xferpak_free(xpak);
				free(mem);
				return -1;
			}
		} else {
			if (memcmp(verify_buffer, mem, mem_size)) {
				u->error("Verify failed\n");
				xferpak_free(xpak);
				free(mem);
				return -1;
			}
		}
		free(verify_buffer);
		printf("\nVerify ok\n");
	}

	xferpak_free(xpak);
	free(mem);

	return 0;
}

int gcn64lib_xferpak_readRAM_to_file(rnt_hdl_t hdl, int channel, const char *output_filename, uiio *u)
{
	xferpak *xpak;
	unsigned char *mem;
	int mem_size;
	struct gbcart_info cartinfo;
	u = getUIIO(u);
	u->caption = "Reading RAM...",

	xpak = gcn64lib_xferpak_init(hdl, channel, u);
	if (!xpak)
		return -1;

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
			u->perror("fopen");
		}
	}

	xferpak_free(xpak);

	return 0;
}

int gcn64lib_xferpak_readROM_to_file(rnt_hdl_t hdl, int channel, const char *output_filename, uiio *u)
{
	xferpak *xpak;
	unsigned char *mem;
	int mem_size;
	struct gbcart_info cartinfo;
	u = getUIIO(u);
	u->caption = "Reading ROM...",

	xpak = gcn64lib_xferpak_init(hdl, channel, u);
	if (!xpak)
		return -1;

	mem_size = xferpak_gb_readROM(xpak, &cartinfo, &mem);
	if (mem_size < 0) {
		xferpak_free(xpak);
		return mem_size;
	}

	if (mem_size > 0) {
		FILE *fptr;
		fptr = fopen(output_filename, "wb");
		if (fptr) {
			fwrite(mem, mem_size, 1, fptr);
			fclose(fptr);
		} else {
			u->perror("fopen");
		}
	}

	xferpak_free(xpak);

	return 0;
}

int gcn64lib_xferpak_printInfo(rnt_hdl_t hdl, int channel)
{
	xferpak *xpak;
	struct gbcart_info cartinfo;
	int res;

	xpak = gcn64lib_xferpak_init(hdl, channel, NULL);
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

