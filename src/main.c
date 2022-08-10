/*	gc_n64_usb : Gamecube/N64 to USB adapter management tools
	Copyright (C) 2007-2020  Raphael Assenat <raph@raphnet.net>

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <wchar.h>

#include "hexdump.h"
#include "raphnetadapter.h"
#include "gcn64lib.h"
#include "x2gcn64_adapters.h"
#include "mempak.h"
#include "mempak_gcn64usb.h"
#include "requests.h"
#include "gcn64_protocol.h"
#include "perftest.h"
#include "usbtest.h"
#include "biosensor.h"
#include "xferpak.h"
#include "xferpak_tools.h"
#include "mempak_stresstest.h"
#include "mempak_fill.h"
#include "wusbmotelib.h"
#include "pcelib.h"
#include "pollraw.h"
#include "psxlib.h"

static void printUsage(void)
{
	printf("./gcn64_ctl [OPTION]... [COMMAND]....\n");
	printf("Command-line management tool for raphnet USB adapters. Version %s\n", VERSION_STR);
	printf("\n");
	printf("Options:\n");
	printf("  -h, --help            Print help\n");
	printf("  -l, --list            List devices\n");
	printf("  -s serial             Operate on specified device (required unless -f is specified)\n");
	printf("  -f, --force           If no serial is specified, use first device detected.\n");
	printf("  -o, --outfile file    Output file for read operations (eg: --n64-mempak-dump)\n");
	//printf("  -i, --infile file     Input file for write operations (eg: --gc_to_n64_update)\n");
	printf("      --nonstop         Continue testing forever or until an error occurs.\n");
	printf("      --noconfirm       Skip asking the user for confirmation.\n");
	printf("  -c, --channel chn     Specify channel to use where applicable (for multi-player adapters\n");
	printf("                        and raw commands, development commands and GC2N64 I/O)\n");
	printf("\n");
	printf("Configuration commands:\n");
	printf("  --get_version                      Read adapter firmware version\n");
	printf("  --set_serial serial                Assign a new device serial number\n");
	printf("  --get_serial                       Read serial from eeprom\n");
	printf("  --set_mode mode                    Set the adapter personality\n");
	printf("                                         0: Standard 1 player (eg: GC/N64 to USB)\n");
	printf("                                         1: N64-only\n");
	printf("                                         2: GC-only\n");
	printf("                                         3: Standard 3 player\n");
	printf("                                         4: Standard 4 player\n");
	printf("                                         5: Standard 5 player\n");
	printf("                                         16: Standard 2 player (eg: WUSBMote 2port)\n");
	printf("                                         17: Dual N64-only\n");
	printf("                                         18: Dual GC-only\n");
	printf("                                         32: Mouse mode\n");
	printf("  --get_mode                         Read the current adapter mode\n");
	printf("  --set_poll_rate ms                 Set time between controller polls in milliseconds\n");
	printf("  --get_poll_rate                    Read configured poll rate\n");
	printf("  --get_controller_type              Display the type of controller currently connected\n");
	printf("\n");
	printf("Advanced commands:\n");
	printf("  --bootloader                       Re-enumerate in bootloader mode\n");
	printf("  --reset                            Cause a firmware reset and USB reconnect\n");
	printf("  --suspend_polling                  Stop polling the controller\n");
	printf("  --resume_polling                   Re-start polling the controller\n");
	printf("  --get_signature                    Get the firmware signature\n");
	printf("\n");
	printf("Raw N64/Gamecube controller commands:\n");
	printf("  --gc_getid                         Get controller ID\n");
	printf("  --gc_getstatus                     Read GC controller status now (turns rumble OFF)\n");
	printf("  --gc_getstatus_rumble              Read GC controller status now (turns rumble ON)\n");
	printf("  --gc_getorigins                    Send 8-bit command 0x41 to a gamecube controller\n");
	printf("  --gc_calibrate                     Send 8-bit command 0x42 to a gamecube controller\n");
	printf("  --n64_getstatus                    Read N64 controller status now\n");
	printf("  --n64_getcaps                      Get N64 controller capabilities (or status such as pak present)\n");
	printf("  --n64_mempak_dump                  Dump N64 mempak contents (Use with --outfile to write to file)\n");
	printf("  --n64_mempak_write file            Write file to N64 mempak\n");
	printf("  --n64_init_rumble                  Send rumble pack init command\n");
	printf("  --n64_control_rumble value         Turn rumble on when value != 0\n");
	printf("  --biosensor                        Display heart beat using bio sensor\n");
	printf("  --perftest                         Do a performance test (raw IO timing)\n");
	printf("\n");
	printf("Raw Wiimote extension commands (for WUSBMote v2 adapters):\n");
	printf("  --disable_encryption               Perform the steps to disable encryption on a controller\n");
	printf("  --dump_wiimote_extmem              Display the current value of the 256 wiimote extension registers\n");
	printf("\n");
	printf("Transfer PAK commands:\n");
	printf("  --xfer_info                        Display information on the inserted gameboy cartridge\n");
	printf("  --xfer_dump_rom file               Dump a gameboy cartridge ROM to a file.\n");
	printf("  --xfer_dump_ram file               Dump a gameboy cartridge RAM to a file.\n");
	printf("  --xfer_write_ram file              Write file to a gameboy cartridge RAM.\n");
	printf("\n");

	printf("x2gcn64 Adapter commands: (For SNES, Gamecube, Classic to GC or N64 adapters, connected through\n");
	printf("                                        a raphnet-tech GC/N64 to USB adapter V3)\n");
	printf("  --x2gcn64_info                     Display adapter information (version, configuration, etc)\n");
	printf("  --x2gcn64_update file.hex          Update the adapter firmware (auto-detects and validate file\n");
	printf("                                     compatiblity with adapter type)\n");
	printf("  --x2gcn64_echotest                 Perform a communication test (usable with --nonstop)\n");
	printf("  --x2gcn64_fw_dump                  Display the firmware content in hex.\n");
	printf("  --x2gcn64_enter_bootloader         Jump to the bootloader.\n");
	printf("  --x2gcn64_boot_application         Exit bootloader and start application.\n");
	printf("\n");

	printf("GC to N64 adapter commands: (For GC to N64 adapter connected to GC/N64 to USB adapter)\n");
	printf("  --gc_to_n64_read_mapping id        Dump a mapping (Use with --outfile to write to file)\n");
	printf("  --gc_to_n64_load_mapping file      Load a mapping from a file and send it to the adapter\n");
	printf("  --gc_to_n64_store_current_mapping slot    Store the current mapping to one of the D-Pad slots.\n");
	printf("\n");

	printf("PSX controller and memory card commands:\n");
	printf("  --psx_mc_dump                      Dump a memory card (Use with --outfile to write to a file)\n");
	printf("  --psx_mc_write file                Write a file to a memory card\n");
	printf("\n");

	printf("Development/Experimental/Research commands: (use at your own risk)\n");
	printf("  --si_8bit_scan                     Try all possible 1-byte commands, to see which one a controller responds to.\n");
	printf("  --si_16bit_scan                    Try all possible 2-byte commands, to see which one a controller responds to.\n");
	printf("  --si_txrx hexbytes                 Send specified bytes and maybe receive something. Ex: --si_txrx 400000 (read GC controller)\n");
	printf("  --n64_crca address                 Calculate the CRC for a N64 PAK address\n");
	printf("  --n64_crcd hexbytes                Calculate the CRC for a block of N64 PAK data (normally 32 bytes)\n");
	printf("  --n64_mempak_stresstest            Perform a set of controller pak tests (WARNING: Erases the pack with random data)\n");
	printf("  --n64_mempak_fill_with_ff          Fill a controller pak with 0xFF (WARNING: Erases your data)\n");
	printf("  --i2c_detect                       Try reading one byte from each I2C address (For WUSBMote v2)\n");
	printf("  --gc_pollraw mode                  Read and display raw values from a gamecube controller (mode range is 0-7)\n");
	printf("  --gc_pollraw_keyboard              Read and display raw values from a gamecube keyboard\n");
	printf("  --n64_pollraw                      Read and display raw values from a N64 controller\n");
	printf("  --n64_pollraw_keyboard             Read and display raw values from a N64 keyboard\n");
	printf("  --psx_pollraw                      Read and display raw values from a Playstation controller\n");
	printf("  --wii_pollraw                      Read and display raw values from a Wii Classic Controller\n");
	printf("  --enable_highres                   Enable high resolution analog for Wii Classic controllers\n");
	printf("  --db9_pollraw                      Read and display raw values from a DB9 adapter\n");
	printf("  --dc_pollraw                       Read and display raw values from a Dreamcast controller\n");
	printf("  --dc_pollraw_mouse                 Read and display raw values from a Dreamcast mouse\n");
	printf("  --usbtest                          Perform a test transfer between host and adapter\n");
	printf("  --debug                            Read debug values from adapter.\n");
}


#define OPT_OUTFILE					'o'
#define OPT_INFILE					'i'
#define OPT_CHANNEL					'c'
#define OPT_SET_SERIAL				257
#define OPT_GET_SERIAL				258
#define OPT_BOOTLOADER				300
#define OPT_N64_GETSTATUS			301
#define OPT_GC_GETSTATUS			302
#define OPT_GC_GETSTATUS_RUMBLE		303
#define OPT_N64_MEMPAK_DUMP			304
#define OPT_N64_GETCAPS				305
#define OPT_SUSPEND_POLLING			306
#define OPT_RESUME_POLLING			307
#define OPT_SET_POLL_INTERVAL			308
#define OPT_GET_POLL_INTERVAL			309
#define OPT_N64_MEMPAK_WRITE			310
#define OPT_SI8BIT_SCAN					311
#define OPT_SI16BIT_SCAN				312
#define OPT_GC_TO_N64_INFO				313
#define OPT_GC_TO_N64_TEST				314
#define OPT_GC_TO_N64_UPDATE			315
#define OPT_GC_TO_N64_DUMP				316
#define OPT_GC_TO_N64_ENTER_BOOTLOADER	317
#define OPT_GC_TO_N64_BOOT_APPLICATION	318
#define OPT_NONSTOP						319
#define OPT_GC_TO_N64_READ_MAPPING		320
#define OPT_GC_TO_N64_LOAD_MAPPING		321
#define OPT_GC_TO_N64_STORE_CURRENT_MAPPING	322
#define OPT_GET_VERSION					323
#define OPT_GET_SIGNATURE				324
#define OPT_GET_CTLTYPE					325
#define OPT_SET_MODE					326
#define OPT_GET_MODE					327
#define OPT_N64_INIT_RUMBLE				328
#define OPT_N64_CONTROL_RUMBLE			329
#define OPT_PERFTEST					330
#define OPT_BIOSENSOR					331
#define OPT_XFERPAK_INFO				332
#define OPT_XFERPAK_DUMP_ROM			333
#define OPT_XFERPAK_DUMP_RAM			334
#define OPT_XFERPAK_WRITE_RAM			335
#define OPT_N64_MEMPAK_DETECT			336
#define OPT_N64_MEMPAK_STRESSTEST		337
#define OPT_RESET						338
#define OPT_I2C_DETECT					339
#define OPT_DISABLE_ENCRYPTION			340
#define OPT_DUMP_WIIMOTE_EXTENSION_MEMORY	341
#define OPT_N64_MEMPAK_FF_FILL			342
#define OPT_NO_CONFIRM					343
#define OPT_GC_GETORIGINS				344
#define OPT_GC_GETID					345
#define OPT_GC_CALIBRATE				346
#define OPT_GC_POLLRAW					347
#define OPT_PCE_RAWTEST					348
#define OPT_USB_TEST					349
#define OPT_GC_POLLRAW_KEYBOARD			350
#define OPT_N64_POLLRAW_KEYBOARD		351
#define OPT_PSX_POLLRAW					352
#define OPT_WII_POLLRAW					353
#define OPT_SITXRX						354
#define OPT_PSX_MC_DUMP					355
#define OPT_DB9_POLLRAW					356
#define OPT_PSX_MC_WRITE				357
#define OPT_N64_CRCA					358
#define OPT_N64_CRCD					359
#define OPT_HIGHRES						360
#define OPT_DEBUG						361
#define OPT_N64_POLLRAW					362
#define OPT_DC_POLLRAW					363
#define OPT_DC_POLLRAW_MOUSE			364

struct option longopts[] = {
	{ "help", 0, NULL, 'h' },
	{ "list", 0, NULL, 'l' },
	{ "force", 0, NULL, 'f' },
	{ "channel", 1, NULL, OPT_CHANNEL },
	{ "set_serial", 1, NULL, OPT_SET_SERIAL },
	{ "get_serial", 0, NULL, OPT_GET_SERIAL },
	{ "set_mode", 1, NULL, OPT_SET_MODE },
	{ "get_mode", 0, NULL, OPT_GET_MODE },
	{ "bootloader", 0, NULL, OPT_BOOTLOADER },
	{ "reset", 0, NULL, OPT_RESET },
	{ "n64_getstatus", 0, NULL, OPT_N64_GETSTATUS },
	{ "gc_getstatus", 0, NULL, OPT_GC_GETSTATUS },
	{ "gc_getorigins", 0, NULL, OPT_GC_GETORIGINS },
	{ "gc_calibrate", 0, NULL, OPT_GC_CALIBRATE },
	{ "gc_getstatus_rumble", 0, NULL, OPT_GC_GETSTATUS_RUMBLE },
	{ "wii_pollraw", 0, NULL, OPT_WII_POLLRAW },
	{ "gc_pollraw", 1, NULL, OPT_GC_POLLRAW },
	{ "gc_pollraw_keyboard", 0, NULL, OPT_GC_POLLRAW_KEYBOARD },
	{ "n64_pollraw_keyboard", 0, NULL, OPT_N64_POLLRAW_KEYBOARD },
	{ "n64_pollraw", 0, NULL, OPT_N64_POLLRAW },
	{ "psx_pollraw", 0, NULL, OPT_PSX_POLLRAW },
	{ "dc_pollraw", 0, NULL, OPT_DC_POLLRAW },
	{ "dc_pollraw_mouse", 0, NULL, OPT_DC_POLLRAW_MOUSE },
	{ "n64_getcaps", 0, NULL, OPT_N64_GETCAPS },
	{ "gc_getid", 0, NULL, OPT_GC_GETID },
	{ "n64_mempak_dump", 0, NULL, OPT_N64_MEMPAK_DUMP },
	{ "suspend_polling", 0, NULL, OPT_SUSPEND_POLLING },
	{ "resume_polling", 0, NULL, OPT_RESUME_POLLING },
	{ "outfile", 1, NULL, OPT_OUTFILE },
	{ "infile", 1, NULL, OPT_INFILE },
	{ "set_poll_rate", 1, NULL, OPT_SET_POLL_INTERVAL },
	{ "get_poll_rate", 0, NULL, OPT_GET_POLL_INTERVAL },
	{ "n64_mempak_write", 1, NULL, OPT_N64_MEMPAK_WRITE },
	{ "si_8bit_scan", 0, NULL, OPT_SI8BIT_SCAN },
	{ "si_16bit_scan", 0, NULL, OPT_SI16BIT_SCAN },
	{ "si_txrx", 1, NULL, OPT_SITXRX },
	{ "x2gcn64_info", 0, NULL, OPT_GC_TO_N64_INFO },
	{ "x2gcn64_echotest", 0, NULL, OPT_GC_TO_N64_TEST },
	{ "x2gcn64_update", 1, NULL, OPT_GC_TO_N64_UPDATE },
	{ "x2gcn64_fw_dump", 0, NULL, OPT_GC_TO_N64_DUMP },
	{ "x2gcn64_enter_bootloader", 0, NULL, OPT_GC_TO_N64_ENTER_BOOTLOADER },
	{ "x2gcn64_boot_application", 0, NULL, OPT_GC_TO_N64_BOOT_APPLICATION },
	{ "gc_to_n64_read_mapping", 1, NULL, OPT_GC_TO_N64_READ_MAPPING },
	{ "gc_to_n64_load_mapping", 1, NULL, OPT_GC_TO_N64_LOAD_MAPPING },
	{ "gc_to_n64_store_current_mapping", 1, NULL, OPT_GC_TO_N64_STORE_CURRENT_MAPPING },
	{ "nonstop", 0, NULL, OPT_NONSTOP },
	{ "get_version", 0, NULL, OPT_GET_VERSION },
	{ "get_signature", 0, NULL, OPT_GET_SIGNATURE },
	{ "get_controller_type", 0, NULL, OPT_GET_CTLTYPE },
	{ "n64_init_rumble", 0, NULL, OPT_N64_INIT_RUMBLE },
	{ "n64_control_rumble", 1, NULL, OPT_N64_CONTROL_RUMBLE },
	{ "perftest", 0, NULL, OPT_PERFTEST },
	{ "biosensor", 0, NULL, OPT_BIOSENSOR },
	{ "xfer_info", 0, NULL, OPT_XFERPAK_INFO },
	{ "xfer_dump_rom", required_argument, NULL, OPT_XFERPAK_DUMP_ROM },
	{ "xfer_dump_ram", required_argument, NULL, OPT_XFERPAK_DUMP_RAM },
	{ "xfer_write_ram", required_argument, NULL, OPT_XFERPAK_WRITE_RAM },
	{ "n64_mempak_detect", 0, NULL, OPT_N64_MEMPAK_DETECT },
	{ "n64_mempak_stresstest", 0, NULL, OPT_N64_MEMPAK_STRESSTEST },
	{ "n64_mempak_fill_with_ff", 0, NULL, OPT_N64_MEMPAK_FF_FILL },
	{ "i2c_detect", 0, NULL, OPT_I2C_DETECT },
	{ "disable_encryption", 0, NULL, OPT_DISABLE_ENCRYPTION },
	{ "dump_wiimote_extmem", 0, NULL, OPT_DUMP_WIIMOTE_EXTENSION_MEMORY },
	{ "noconfirm", 0, NULL, OPT_NO_CONFIRM },
	{ "pce_rawtest", 0, NULL, OPT_PCE_RAWTEST },
	{ "usbtest", 0, NULL, OPT_USB_TEST },
	{ "psx_mc_dump", 0, NULL, OPT_PSX_MC_DUMP },
	{ "psx_mc_write", required_argument, NULL, OPT_PSX_MC_WRITE },
	{ "db9_pollraw", 0, NULL, OPT_DB9_POLLRAW },
	{ "n64_crca", required_argument, NULL, OPT_N64_CRCA },
	{ "n64_crcd", required_argument, NULL, OPT_N64_CRCD },
	{ "enable_highres", 0, NULL, OPT_HIGHRES },
	{ "debug", 0, NULL, OPT_DEBUG },
	{ },
};

static int mempak_progress_cb(int addr, void *ctx)
{
	printf("\r%s 0x%04x / 0x%04x  ", (char*)ctx, addr, MEMPAK_MEM_SIZE); fflush(stdout);
	return 0;
}

static int listDevices(void)
{
	int n_found = 0;
	struct rnt_adap_list_ctx *listctx;
	struct rnt_adap_info inf;

	listctx = rnt_allocListCtx();
	if (!listctx) {
		fprintf(stderr, "List context could not be allocated\n");
		return -1;
	}
	while (rnt_listDevices(&inf, listctx))
	{
		n_found++;
		printf("Found device '%ls', serial '%ls'. Supports %d channel(s), %d raw channel(s), Block io: %s\n",
			inf.str_prodname, inf.str_serial, inf.caps.n_channels,
				inf.caps.n_raw_channels,
				inf.caps.features & RNTF_BLOCK_IO ? "Yes":"No");

	}
	rnt_freeListCtx(listctx);
	printf("%d device(s) found\n", n_found);

	return n_found;
}

int main(int argc, char **argv)
{
	rnt_hdl_t hdl;
	struct rnt_adap_list_ctx *listctx;
	int opt, retval = 0;
	struct rnt_adap_info inf;
	struct rnt_adap_info *selected_device = NULL;
	int verbose = 0, use_first = 0, serial_specified = 0;
	int nonstop = 0;
	int enable_highres = 0;
	int noconfirm = 0;
	int cmd_list = 0;
#define TARGET_SERIAL_CHARS 128
	wchar_t target_serial[TARGET_SERIAL_CHARS];
	const char *short_optstr = "hls:vfo:c:";
	const char *outfile = NULL;
	const char *infile = NULL;
	int channel = 0;
	int res;

	while((opt = getopt_long(argc, argv, short_optstr, longopts, NULL)) != -1) {
		switch(opt)
		{
			case 's':
				{
					mbstate_t ps;
					memset(&ps, 0, sizeof(ps));
					if (mbsrtowcs(target_serial, (const char **)&optarg, TARGET_SERIAL_CHARS, &ps) < 1) {
						fprintf(stderr, "Invalid serial number specified\n");
						return -1;
					}
					serial_specified = 1;
				}
				break;
			case 'f':
				use_first = 1;
				break;
			case 'v':
				verbose = 1;
				break;
			case 'h':
				printUsage();
				return 0;
			case 'l':
				cmd_list = 1;
				break;
			case 'o':
				outfile = optarg;
				printf("Output file: %s\n", outfile);
				break;
			case 'i':
				infile = optarg;
				printf("Input file: %s\n", infile);
				break;
			case OPT_CHANNEL:
				channel = atoi(optarg);
				printf("SI channel: %d\n", channel);
				break;
			case OPT_NONSTOP:
				nonstop = 1;
				break;
			case OPT_NO_CONFIRM:
				noconfirm = 1;
				break;
			case '?':
				fprintf(stderr, "Unrecognized argument. Try -h\n");
				return -1;

			case OPT_N64_CRCA:
				{
					long addr;
					char *e;

					addr = strtol(optarg, &e, 0);
					if (e==optarg) {
						fprintf(stderr, "Invalid address\n");
						return 1;
					}
					if (addr < 0 || addr > 0xffff) {
						fprintf(stderr, "Address out of range\n");
						return 1;
					}

					printf("Computing pak address crc for 0x%04x\n", (uint16_t)addr);
					printf("CRCA: 0x%04x\n", pak_address_crc(addr));
					return 0;
				}
				break;

			case OPT_N64_CRCD:
				{
					uint8_t inbuf[64];
					int inlen;
					int v, i;
					uint8_t crc;

					if (strlen(optarg) % 2) {
						fprintf(stderr, "Error: An even number of nibbles must be specified, and no space between bytes. Ex: 1301 not 13 01\n");
						return -1;
					}

					inlen = strlen(optarg)/2;
					if (inlen > sizeof(inbuf)) {
						fprintf(stderr, "Error: Too many bytes. Max %d\n", (int)sizeof(inbuf));
						return -1;
					}

					for (i=0; i<inlen; i++) {
						sscanf(optarg + (i*2), "%02x", &v);
						inbuf[i] = v;
					}

					printf("Computing pak data CRC[%d] : ", inlen);
					printHexBuf(inbuf, inlen);

					crc = pak_data_crc(inbuf, inlen);

					printf("CRCD: 0x%02x\n", crc);
					return 0;
				}
				break;

			case OPT_HIGHRES:
				enable_highres = 1;
				break;
		}
	}

	rnt_init(verbose);

	if (cmd_list) {
		printf("Simply listing the devices...\n");
		res = listDevices();
		if (res > 0) {
			printf("Found %d devices\n", res);
			return 0;
		} else {
			printf("No device found\n");
			return 1;
		}
	}

	if (!serial_specified && !use_first) {
		fprintf(stderr, "A serial number or -f must be used. Try -h for more information.\n");
		return 1;
	}

	listctx = rnt_allocListCtx();
	while ((selected_device = rnt_listDevices(&inf, listctx)))
	{
		if (serial_specified) {
			if (0 == wcscmp(inf.str_serial, target_serial)) {
				break;
			}
		}
		else {
			// use_first == 1
			printf("Will use device '%ls' serial '%ls'\n", inf.str_prodname, inf.str_serial);
			break;
		}
	}
	rnt_freeListCtx(listctx);

	if (!selected_device) {
		if (serial_specified) {
			fprintf(stderr, "Device not found\n");
		} else {
			fprintf(stderr, "No device found\n");
		}
		return 1;
	}

	hdl = rnt_openDevice(selected_device);
	if (!hdl) {
		printf("Error opening device. (Do you have permissions?)\n");
		return 1;
	}

	optind = 1;
	while((opt = getopt_long(argc, argv, short_optstr, longopts, NULL)) != -1)
	{
		unsigned char cmd[64] = { };
		int n;
		int cmdlen = 0;
		int do_exchange = 0;

		switch (opt)
		{
			case OPT_SET_MODE:
				cmd[0] = atoi(optarg);
				printf("Setting mode to %d\n", cmd[0]);
				rnt_setConfig(hdl, CFG_PARAM_MODE, cmd, 1);
				break;

			case OPT_GET_MODE:
				n = rnt_getConfig(hdl, CFG_PARAM_MODE, cmd, sizeof(cmd));
				if (n == 1) {
					printf("Current mode: %d\n", cmd[0]);
				}
				break;

			case OPT_SET_POLL_INTERVAL:
				cmd[0] = atoi(optarg);
				printf("Setting poll interval to %d ms\n", cmd[0]);
				rnt_setConfig(hdl, CFG_PARAM_POLL_INTERVAL0, cmd, 1);
				break;

			case OPT_GET_POLL_INTERVAL:
				n = rnt_getConfig(hdl, CFG_PARAM_POLL_INTERVAL0, cmd, sizeof(cmd));
				if (n == 1) {
					printf("Poll interval: %d ms\n", cmd[0]);
				}
				break;

			case OPT_SET_SERIAL:
				printf("Setting serial...");
				if (strlen(optarg) != 6) {
					fprintf(stderr, "Serial number must be 6 characters\n");
					return -1;
				}
				rnt_setConfig(hdl, CFG_PARAM_SERIAL, (void*)optarg, 6);
				break;

			case OPT_GET_SERIAL:
				n = rnt_getConfig(hdl, CFG_PARAM_SERIAL, cmd, sizeof(cmd));
				if (n==6) {
					cmd[6] = 0;
					printf("Serial: %s\n", cmd);
				}
				break;

			case OPT_BOOTLOADER:
				printf("Sending 'jump to bootloader' command...");
				rnt_bootloader(hdl);
				break;

			case OPT_RESET:
				printf("Sending 'reset firmware' command...");
				rnt_reset(hdl);
				break;

			case OPT_SUSPEND_POLLING:
				rnt_suspendPolling(hdl, 1);
				break;

			case OPT_RESUME_POLLING:
				rnt_suspendPolling(hdl, 0);
				break;

			case OPT_N64_INIT_RUMBLE:
				{
					unsigned char cmdbuf[32] = {
						[0 ... 31] = 0x80
					};

					n = gcn64lib_mempak_writeBlock(hdl, channel, 0x8000, cmdbuf);
					if (n < 0) {
						printHexBuf(cmd, n);
					} else {
						printf("Error %d\n", n);
					}

				}
				break;

			case OPT_N64_CONTROL_RUMBLE:
				{
					int on = atoi(optarg);
					unsigned char cmdbuf[32] = {
						[0 ... 31] = on ? 0x01 : 0x00,
					};

					n = gcn64lib_mempak_writeBlock(hdl, channel, 0xC000, cmdbuf);
					if (n < 0) {
						printf("Error %d\n", n);
					}
				}
				break;

			case OPT_BIOSENSOR:
				gcn64lib_biosensorMonitor(hdl, channel);
				break;

			case OPT_XFERPAK_INFO:
				gcn64lib_xferpak_printInfo(hdl, channel);
				break;

			case OPT_XFERPAK_DUMP_ROM:
				rnt_suspendPolling(hdl, 1);
				res = gcn64lib_xferpak_readROM_to_file(hdl, channel, optarg, NULL);
				rnt_suspendPolling(hdl, 0);

				if (res == 0) {
					printf("Wrote %s\n", optarg);
				}
				break;

			case OPT_XFERPAK_DUMP_RAM:
				rnt_suspendPolling(hdl, 1);
				res = gcn64lib_xferpak_readRAM_to_file(hdl, channel, optarg, NULL);
				rnt_suspendPolling(hdl, 0);

				if (res == 0) {
					printf("Wrote %s\n", optarg);
				}
				break;

			case OPT_XFERPAK_WRITE_RAM:
				rnt_suspendPolling(hdl, 1);
				res = gcn64lib_xferpak_writeRAM_from_file(hdl, channel, optarg, 1, NULL);
				rnt_suspendPolling(hdl, 0);

				if (res == 0) {
					printf("Wrote %s to cartridge\n", optarg);
				}
				break;

			case OPT_N64_GETSTATUS:
				cmd[0] = N64_GET_STATUS;
				n = gcn64lib_rawSiCommand(hdl, channel, cmd, 1, cmd, sizeof(cmd));
				if (n >= 0) {
					printf("N64 Get status[%d]: ", n);
					printHexBuf(cmd, n);
				}
				break;

			case OPT_GC_GETORIGINS:
				cmd[0] = GC_GET_ORIGINS;
				n = gcn64lib_rawSiCommand(hdl, channel, cmd, 1, cmd, sizeof(cmd));
				if (n >= 0) {
					printf("GC Get origins answer[%d]: ", n);
					printHexBuf(cmd, n);
				}
				break;

			case OPT_GC_CALIBRATE:
				cmd[0] = GC_CALIBRATE;
				cmd[1] = 0;
				cmd[2] = 0;
				n = gcn64lib_rawSiCommand(hdl, channel, cmd, 3, cmd, sizeof(cmd));
				if (n >= 0) {
					printf("GC Calibrate command answer[%d]: ", n);
					printHexBuf(cmd, n);
				}
				break;

			case OPT_DB9_POLLRAW:
				retval = pollraw_db9(hdl, channel);
				break;

			case OPT_WII_POLLRAW:
				retval = pollraw_wii(hdl, channel, enable_highres);
				break;

			case OPT_GC_POLLRAW:
				retval = pollraw_gamecube(hdl, channel, atoi(optarg));
				break;

			case OPT_GC_POLLRAW_KEYBOARD:
				retval = pollraw_gamecube_keyboard(hdl, channel);
				break;

			case OPT_N64_POLLRAW_KEYBOARD:
				retval = pollraw_randnet_keyboard(hdl, channel);
				break;

			case OPT_N64_POLLRAW:
				retval = pollraw_n64(hdl, channel);
				break;

			case OPT_PSX_POLLRAW:
				retval = pollraw_psx(hdl, channel);
				break;

			case OPT_DC_POLLRAW:
				retval = pollraw_dreamcast_controller(hdl, channel);
				break;

			case OPT_DC_POLLRAW_MOUSE:
				retval = pollraw_dreamcast_mouse(hdl, channel);
				break;

			case OPT_GC_GETSTATUS_RUMBLE:
			case OPT_GC_GETSTATUS:
				cmd[0] = GC_GETSTATUS1;
				cmd[1] = GC_GETSTATUS2;
				cmd[2] = GC_GETSTATUS3(opt == OPT_GC_GETSTATUS_RUMBLE);
				n = gcn64lib_rawSiCommand(hdl, channel, cmd, 3, cmd, sizeof(cmd));
				if (n >= 0) {
					printf("GC Get status[%d]: ", n);
					printHexBuf(cmd, n);
				}
				break;

			case OPT_GC_GETID:
				cmd[0] = GC_GETID;
				//cmd[0] = 0xff;
				n = gcn64lib_rawSiCommand(hdl, channel, cmd, 1, cmd, sizeof(cmd));
				if (n >= 0) {
					printf("GC Get ID[%d]: ", n);
					printHexBuf(cmd, n);
				}

				if (n != GC_GETID_REPLY_LENGTH) {
					fprintf(stderr, "Invalid response (expected 3 bytes)\n");
					retval = 1;
				} else {
					retval = 0;
				}
				break;

			case OPT_N64_GETCAPS:
				cmd[0] = N64_GET_CAPABILITIES;
				//cmd[0] = 0xff;
				n = gcn64lib_rawSiCommand(hdl, channel, cmd, 1, cmd, sizeof(cmd));
				if (n >= 0) {
					printf("N64 Get caps[%d]: ", n);
					printHexBuf(cmd, n);
				}

				if (n != 3) {
					fprintf(stderr, "Invalid response (expected 3 bytes)\n");
					retval = 1;
				} else {
					retval = 0;
				}
				break;

			case OPT_N64_MEMPAK_DETECT:
				{
					printf("Detecting mempak...\n");
					res = gcn64lib_mempak_detect(hdl, channel);
					if (res == 0) {
						printf("Mempak detected\n");
						retval = 0;
					} else {
						printf("No mempak detected\n");
						retval = 1;
					}
				}
				break;

			case OPT_N64_MEMPAK_STRESSTEST:
				{
					res = mempak_stresstest(hdl, channel, 0);
					printf("Test returned %d\n", res);
					if (res != 0)
						retval = 1;
				}
				break;

			case OPT_N64_MEMPAK_FF_FILL:
				{
					res = mempak_fill(hdl, channel, 0xFF, noconfirm, NULL);
					if (res != 0)
						retval = 1;
				}
				break;

			case OPT_N64_MEMPAK_DUMP:
				{
					mempak_structure_t *pak;
					int res;

					printf("Reading mempak...\n");
					res = gcn64lib_mempak_download(hdl, channel, &pak, mempak_progress_cb, "Reading address");
					printf("\n");
					switch (res)
					{
						case 0:
							if (outfile) {
								int file_format;

								file_format = mempak_getFilenameFormat(outfile);
								if (file_format == MPK_FORMAT_INVALID) {
									fprintf(stderr, "Unknown file format (neither .MPK nor .N64). Not saving.\n");
								} else {
									if (0 == mempak_saveToFile(pak, outfile, file_format)) {
										printf("Wrote file '%s' in %s format\n", outfile, mempak_format2string(file_format));
									} else {
										fprintf(stderr, "error writing file\n");
									}
								}
							} else { // No outfile
								mempak_hexdump(pak);
							}
							mempak_free(pak);
							break;
						case -1:
							fprintf(stderr, "No mempak detected\n");
							break;
						case -2:
							fprintf(stderr, "I/O error reading pak\n");
							break;
						default:
						case -3:
							fprintf(stderr, "Error\n");
							break;

					}

				}
				break;

			case OPT_N64_MEMPAK_WRITE:
				{
					mempak_structure_t *pak;
					int res;
					printf("Input file: %s\n", optarg);

					pak = mempak_loadFromFile(optarg);
					if (!pak) {
						fprintf(stderr, "Failed to load mempak\n");
						return -1;
					}

					printf("Writing to mempak...\n");
					res = gcn64lib_mempak_upload(hdl, channel, pak, mempak_progress_cb, "Writing address");
					printf("\n");
					if (res) {
						switch(res)
						{
							case -1:
								fprintf(stderr, "Error: No mempak detected.\n");
								break;
							case -2:
								fprintf(stderr, "I/O error writing to pak.\n");
								break;
							default:
								fprintf(stderr, "Error uploading mempak\n");
						}
					} else {
						printf("Mempak uploaded\n");
					}
					mempak_free(pak);
				}
				break;

			case OPT_SITXRX:
				{
					uint8_t txbuf[64];
					int txlen;
					uint8_t rxbuf[64];
					int v, i;

					if (strlen(optarg) % 2) {
						fprintf(stderr, "Error: An even number of nibbles must be specified, and no space between bytes. Ex: 1301 not 13 01\n");
						return -1;
					}

					txlen = strlen(optarg)/2;
					if (txlen > sizeof(txbuf)) {
						fprintf(stderr, "Too many bytes. Max %d\n", (int)sizeof(txbuf));
						return -1;
					}

					for (i=0; i<txlen; i++) {
						sscanf(optarg + (i*2), "%02x", &v);
						txbuf[i] = v;
					}

					printf("Data to transmit[%d] : ", txlen);
					printHexBuf(txbuf, txlen);

					res = gcn64lib_rawSiCommand(hdl, channel, txbuf, txlen, rxbuf, sizeof(rxbuf));
					if (res < 0) {
						return -1;
					}

					printf("Data received[%d] : ", res);
					printHexBuf(rxbuf, res);
				}
				break;

			case OPT_SI8BIT_SCAN:
				gcn64lib_8bit_scan(hdl, channel, 0, 255);
				break;

			case OPT_SI16BIT_SCAN:
				gcn64lib_16bit_scan(hdl, channel, 0, 0xffff);
				break;

			case OPT_I2C_DETECT:
				wusbmotelib_i2c_detect(hdl, channel, NULL, 1);
				break;

			case OPT_GC_TO_N64_INFO:
				{
					struct x2gcn64_adapter_info inf;

					res = x2gcn64_adapter_getInfo(hdl, channel, &inf);
					if (res == 0) {
						x2gcn64_adapter_printInfo(&inf);
					} else {
						retval = 1;
					}
				}
				break;

			case OPT_GC_TO_N64_TEST:
				{
					int i=0;

					do {
						n = x2gcn64_adapter_echotest(hdl, channel, 1);
						if (n != 0) {
							printf("Test failed\n");
							return -1;
						}
						usleep(1000 * (i));
						i++;
						if (i>100)
							i=0;
						printf("."); fflush(stdout);
					} while (nonstop);
					printf("Test ok\n");
				}
				break;

			case OPT_GC_TO_N64_UPDATE:
				x2gcn64_adapter_updateFirmware(hdl, channel, optarg, NULL);
				break;

			case OPT_GC_TO_N64_DUMP:
				x2gcn64_adapter_dumpFlash(hdl, channel);
				break;

			case OPT_GC_TO_N64_ENTER_BOOTLOADER:
				x2gcn64_adapter_enterBootloader(hdl, channel);
				x2gcn64_adapter_waitForBootloader(hdl, channel, 5);
				break;

			case OPT_GC_TO_N64_BOOT_APPLICATION:
				x2gcn64_adapter_bootApplication(hdl, channel);
				break;

			case OPT_GC_TO_N64_READ_MAPPING:
				{
					struct x2gcn64_adapter_info inf;
					int map_id;

					map_id = atoi(optarg);
					if ((map_id <= 0) || (map_id > GC2N64_NUM_MAPPINGS)) {
						fprintf(stderr, "Invalid mapping id (1 to 4)\n");
						return -1;
					}

					x2gcn64_adapter_getInfo(hdl, channel, &inf);
					printf("Mapping %d : { ", map_id);
					gc2n64_adapter_printMapping(&inf.app.gc2n64.mappings[map_id-1]);
					printf(" }\n");
					if (outfile) {
						printf("Writing mapping to file '%s'\n", outfile);
						gc2n64_adapter_saveMapping(&inf.app.gc2n64.mappings[map_id-1], outfile);
					}
				}
				break;

			case OPT_GC_TO_N64_LOAD_MAPPING:
				{
					struct gc2n64_adapter_mapping *mapping;

					printf("Reading mapping from file '%s'\n", optarg);
					mapping = gc2n64_adapter_loadMapping(optarg);
					if (!mapping) {
						fprintf(stderr, "Failed to load mapping\n");
						return -1;
					}

					printf("Mapping : { ");
					gc2n64_adapter_printMapping(mapping);
					printf(" }\n");

					gc2n64_adapter_setMapping(hdl, channel, mapping);

					free(mapping);
				}
				break;

			case OPT_GC_TO_N64_STORE_CURRENT_MAPPING:
				{
					int slot;

					slot = atoi(optarg);

					if (slot < 1 || slot > 4) {
						fprintf(stderr, "Mapping out of range (1-4)\n");
						return -1;
					}

					if (0 == gc2n64_adapter_storeCurrentMapping(hdl, channel, slot)) {
						printf("Stored mapping to slot %d (%s)\n", slot, gc2n64_adapter_getMappingSlotName(slot, 0));
					} else {
						printf("Error storing mapping\n");
					}
				}
				break;

			case OPT_GET_VERSION:
				{
					char version[64];

					if (0 == rnt_getVersion(hdl, version, sizeof(version))) {
						printf("Firmware version: %s\n", version);
					}
				}
				break;

			case OPT_GET_SIGNATURE:
				{
					char sig[64];

					if (0 == rnt_getSignatureCompat(hdl, sig, sizeof(sig))) {
						printf("Signature: %s\n", sig);
					}
				}
				break;

			case OPT_GET_CTLTYPE:
				{
					int type;
					type = rnt_getControllerType(hdl, channel);
					printf("Controller type 0x%02x: %s\n", type, rnt_controllerName(type));
				}
				break;

			case OPT_PERFTEST:
				perftest1(hdl, channel);
				break;

			case OPT_DISABLE_ENCRYPTION:
				retval = wusbmotelib_disableEncryption(hdl, channel);
				break;

			case OPT_DUMP_WIIMOTE_EXTENSION_MEMORY:
				retval = wusbmotelib_dumpMemory(hdl, channel, NULL, 1);
				break;

			case OPT_PCE_RAWTEST:
				pcelib_rawpoll(hdl);
				break;

			case OPT_USB_TEST:
				do {
					retval = usbtest(hdl, verbose);
					if (retval) {
						break;
					}
				} while (nonstop);
				break;

			case OPT_DEBUG:
				{
					uint8_t cmd[1] = { RQ_RNT_GET_DEBUG_BUF };
					uint8_t rxbuf[32];

					res = rnt_exchange(hdl, cmd, 1, rxbuf, sizeof(rxbuf));
					if (res < 2) {
						printf("No data\n");
					} else {
						printf("Debug data[%d] : ", res-1);
						printHexBuf(rxbuf+1, res-1);
					}
				}
				break;

			case OPT_PSX_MC_DUMP:
				{
					struct psx_memorycard mc_data;
					int res;

					rnt_suspendPolling(hdl, 1);
					res = psxlib_readMemoryCard(hdl, channel, &mc_data, NULL);
					rnt_suspendPolling(hdl, 0);

					if (res == 0) {
						// Todo: filename-based format selection
						psxlib_writeMemoryCardToFile(&mc_data, outfile, PSXLIB_FILE_FORMAT_RAW);
					}
					else {
						fprintf(stderr, "%s\n", psxlib_getErrorString(res));
					}

				}
				break;

			case OPT_PSX_MC_WRITE:
				{
					struct psx_memorycard mc_data;

					retval = psxlib_loadMemoryCardFromFile(optarg, PSXLIB_FILE_FORMAT_AUTO, &mc_data);
					if (retval < 0) {
						fprintf(stderr, "%s\n", psxlib_getErrorString(retval));
						break;
					}

					rnt_suspendPolling(hdl, 1);
					retval = psxlib_writeMemoryCard(hdl, channel, &mc_data, NULL);
					rnt_suspendPolling(hdl, 0);

					if (retval < 0) {
						fprintf(stderr, "%s\n", psxlib_getErrorString(retval));
						break;
					}
				}
				break;
		}

		if (do_exchange) {
			int i;
			n = rnt_exchange(hdl, cmd, cmdlen, cmd, sizeof(cmd));
			if (n<0)
				break;

			printf("Result: %d bytes: ", n);
			for (i=0; i<n; i++) {
				printf("%02x ", cmd[i]);
			}
			printf("\n");
		}
	}

	rnt_closeDevice(hdl);
	rnt_shutdown();

	return retval;
}
