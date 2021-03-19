#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/modulemgr.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/devctl.h>
#include <psp2/ctrl.h>
#include <psp2/shellutil.h>
#include <psp2/net/http.h>
#include <psp2/net/net.h>
#include <psp2/sysmodule.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/net/netctl.h>
#include <psp2/io/stat.h>
#include <psp2/io/dirent.h>

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <taihen.h>
#include "graphics.h"
#include "Archives.h"

static uint32_t crc32_tab[] = {
	0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
	0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
	0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
	0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
	0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
	0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
	0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
	0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
	0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
	0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
	0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
	0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
	0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
	0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
	0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
	0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
	0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
	0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
	0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
	0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
	0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
	0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
	0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
	0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
	0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
	0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
	0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
	0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
	0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
	0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
	0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
	0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
	0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
	0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
	0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
	0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
	0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
	0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
	0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
	0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
	0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
	0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
	0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d };

#define printf psvDebugScreenPrintf
#define CHUNK_SIZE 64 * 1024
#define hasEndSlash(path) (path[strlen(path) - 1] == '/')

enum {
	SCREEN_WIDTH = 960,
	SCREEN_HEIGHT = 544,
	PROGRESS_BAR_WIDTH = SCREEN_WIDTH,
	PROGRESS_BAR_HEIGHT = 20,
	LINE_SIZE = SCREEN_WIDTH,
};

static int mlink = 0;
static char appi_cfg[16];
const char minor[4][32] = {"app0:mshell.self","app0:imcunlock.self","app0:tiny_modoru.self","app0:fwtool_portable.self"};
const char kernel[4][32] = {"","app0:plugins/imcunlock.skprx","app0:plugins/tiny_modoru.skprx","app0:plugins/fwtool.skprx"};
const char taidll[2][3][64] = {
	{ "http://hackmyvita.13375384.xyz/tai/v_tai.zip","http://hackmyvita.13375384.xyz/tai/r_tai.zip","http://hackmyvita.13375384.xyz/tai/y_tai.zip" },
	{ "http://files.13375384.xyz/psp2/v_tai.zip","http://files.13375384.xyz/psp2/r_tai.zip","http://files.13375384.xyz/psp2/y_tai.zip" }
};

const char ensodll[2][64] = { "http://hackmyvita.13375384.xyz/e2x/e2x_def.zip","http://files.13375384.xyz/psp2/e2x_def.zip" };

const char fwdll[2][3][2][64] = {
{
	{ "http://hackmyvita.13375384.xyz/bin/360.01","http://hackmyvita.13375384.xyz/bin/360.02"},
	{ "http://hackmyvita.13375384.xyz/bin/365.01","http://hackmyvita.13375384.xyz/bin/365.02"},
	{ "http://hackmyvita.13375384.xyz/bin/368.01","http://hackmyvita.13375384.xyz/bin/368.02"}
},
{
	{ "http://files.13375384.xyz/psp2/360.01","http://files.13375384.xyz/psp2/360.02"},
	{ "http://files.13375384.xyz/psp2/365.01","http://files.13375384.xyz/psp2/365.02"},
	{ "http://files.13375384.xyz/psp2/368.01","http://files.13375384.xyz/psp2/368.02"}
}
};

const char swudll[2][64] = { "http://hackmyvita.13375384.xyz/bin/SWU.00", "http://files.13375384.xyz/psp2/SWU.00", };

const char appdll[2][12][64] = {
{
	"http://hackmyvita.13375384.xyz/vpk/shell.vpk",
	"http://hackmyvita.13375384.xyz/vpk/vhbb.vpk",
	"http://hackmyvita.13375384.xyz/vpk/itls.vpk",
	"http://hackmyvita.13375384.xyz/vpk/enso.vpk",
	"http://hackmyvita.13375384.xyz/vpk/yamt.vpk",
	"http://hackmyvita.13375384.xyz/vpk/adre.vpk",
	"http://hackmyvita.13375384.xyz/vpk/pkgj.vpk",
	"http://hackmyvita.13375384.xyz/vpk/savemgr.vpk",
	"http://hackmyvita.13375384.xyz/vpk/thememgr.vpk",
	"http://hackmyvita.13375384.xyz/vpk/battery.vpk",
	"http://hackmyvita.13375384.xyz/vpk/reg.vpk",
	"http://hackmyvita.13375384.xyz/vpk/ident.vpk"
},
{
	"http://files.13375384.xyz/psp2/shell.vpk",
	"http://files.13375384.xyz/psp2/vhbb.vpk",
	"http://files.13375384.xyz/psp2/itls.vpk",
	"http://files.13375384.xyz/psp2/enso.vpk",
	"http://files.13375384.xyz/psp2/yamt.vpk",
	"http://files.13375384.xyz/psp2/adre.vpk",
	"http://files.13375384.xyz/psp2/pkgj.vpk",
	"http://files.13375384.xyz/psp2/savemgr.vpk",
	"http://files.13375384.xyz/psp2/thememgr.vpk",
	"http://files.13375384.xyz/psp2/battery.vpk",
	"http://files.13375384.xyz/psp2/reg.vpk",
	"http://files.13375384.xyz/psp2/ident.vpk"
}
};

const char appnames[12][24] = {
	"VitaShell",
	"VHBB",
	"the iTLS installer",
	"the enso installer",
	"the YAMT installer",
	"Adrenaline",
	"PKGj",
	"vita-savemgr",
	"Custom Themes Manager",
	"batteryFixer",
	"registry editor",
	"PSVident"
};

const char appvpks[12][32] = {
	"ux0:downloads/VitaShell.vpk",
	"ux0:downloads/VHBB.vpk",
	"ux0:downloads/iTLS.vpk",
	"ux0:downloads/enso.vpk",
	"ux0:downloads/YAMT.vpk",
	"ux0:downloads/Adrenaline.vpk",
	"ux0:downloads/PKGj.vpk",
	"ux0:downloads/vita-savemgr.vpk",
	"ux0:downloads/ThemeManager.vpk",
	"ux0:downloads/batteryFixer.vpk",
	"ux0:downloads/regEdit.vpk",
	"ux0:downloads/PSVident.vpk"
};

const uint32_t fwcrc[3][2] = {
	{0x71BF741C, 0x3359C77B},
	{0xA475D0BD, 0x647EDCDF},
	{0xA1C5DE91, 0x90931C42}
};

int fcp(const char* src, const char* dst) {
	sceClibPrintf("Copying %s -> %s (file)... ", src, dst);
	int res;
	SceUID fdsrc = -1, fddst = -1;
	void* buf = NULL;

	res = fdsrc = sceIoOpen(src, SCE_O_RDONLY, 0);
	if (res < 0)
		goto err;

	res = fddst = sceIoOpen(dst, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	if (res < 0)
		goto err;

	buf = memalign(4096, CHUNK_SIZE);
	if (!buf) {
		res = -1;
		goto err;
	}

	do {
		res = sceIoRead(fdsrc, buf, CHUNK_SIZE);
		if (res > 0)
			res = sceIoWrite(fddst, buf, res);
	} while (res > 0);

err:
	if (buf)
		free(buf);
	if (fddst >= 0)
		sceIoClose(fddst);
	if (fdsrc >= 0)
		sceIoClose(fdsrc);
	sceClibPrintf("%s\n", (res < 0) ? "FAILED" : "OK");
	return res;
}

int copyDir(const char* src_path, const char* dst_path) {
	SceUID dfd = sceIoDopen(src_path);
	if (dfd >= 0) {
		sceClibPrintf("Copying %s -> %s (dir)\n", src_path, dst_path);
		SceIoStat stat;
		sceClibMemset(&stat, 0, sizeof(SceIoStat));
		sceIoGetstatByFd(dfd, &stat);

		stat.st_mode |= SCE_S_IWUSR;

		sceIoMkdir(dst_path, 6);

		int res = 0;

		do {
			SceIoDirent dir;
			sceClibMemset(&dir, 0, sizeof(SceIoDirent));

			res = sceIoDread(dfd, &dir);
			if (res > 0) {
				char* new_src_path = malloc(strlen(src_path) + strlen(dir.d_name) + 2);
				sceClibSnprintf(new_src_path, 1024, "%s%s%s", src_path, hasEndSlash(src_path) ? "" : "/", dir.d_name);

				char* new_dst_path = malloc(strlen(dst_path) + strlen(dir.d_name) + 2);
				sceClibSnprintf(new_dst_path, 1024, "%s%s%s", dst_path, hasEndSlash(dst_path) ? "" : "/", dir.d_name);

				int ret = 0;

				if (SCE_S_ISDIR(dir.d_stat.st_mode)) {
					ret = copyDir(new_src_path, new_dst_path);
				} else {
					ret = fcp(new_src_path, new_dst_path);
				}

				free(new_dst_path);
				free(new_src_path);

				if (ret < 0) {
					sceIoDclose(dfd);
					return ret;
				}
			}
		} while (res > 0);

		sceIoDclose(dfd);
	} else
		return fcp(src_path, dst_path);

	return 1;
}

void dead(const char* msg) {
	psvDebugScreenSetFgColor(COLOR_RED);
	printf("\nVitaDeploy stopped with the following error:\n\n> %s\n", msg);
	psvDebugScreenSetFgColor(COLOR_WHITE);
	printf("you can now exit this app\n");
	while (1) {};
}

uint32_t crcb(uint32_t crc, const void* buf, size_t size) {
	const uint8_t* p;

	p = buf;
	crc = crc ^ ~0U;

	while (size--)
		crc = crc32_tab[(crc ^ *p++) & 0xFF] ^ (crc >> 8);

	return crc ^ ~0U;
}

void draw_rect(int x, int y, int width, int height, uint32_t color) {
	void* base = psvDebugScreenGetVram();

	for (int j = y; j < y + height; ++j)
		for (int i = x; i < x + width; ++i)
			((uint32_t*)base)[j * LINE_SIZE + i] = color;
}

int download_file(const char* link, const char* file, const char* tmp, uint32_t exp_crc) {

	sceClibPrintf("DL %s->%s\n", link, file);

	uint32_t res_crc = 0;
	static unsigned char buf[16 * 1024];

	int tpl = sceHttpCreateTemplate("VitaDeploy DL", 1, 1);
	if (tpl < 0) {
		printf("tpl error 0x%X\n", tpl);
		return -1;
	}

	int conn = sceHttpCreateConnectionWithURL(tpl, link, 0);
	if (conn < 0) {
		printf("sceHttpCreateConnectionWithURL: 0x%x\n", conn);
		return conn;
	}
	int req = sceHttpCreateRequestWithURL(conn, 0, link, 0);
	if (req < 0) {
		printf("sceHttpCreateRequestWithURL: 0x%x\n", req);
		sceHttpDeleteConnection(conn);
		return req;
	}
	int ret = sceHttpSendRequest(req, NULL, 0);
	if (ret < 0) {
		printf("sceHttpSendRequest: 0x%x\n", ret);
		goto end;
	}

	uint64_t length = 0;
	ret = sceHttpGetResponseContentLength(req, &length);
	if (ret < 0) {
		printf("sceHttpGetResponseContentLength error! 0x%X\n", ret);
		goto end;
	}

	int total_read = 0;
	int fd = (tmp == NULL) ? sceIoOpen(file, SCE_O_APPEND | SCE_O_WRONLY, 6) : sceIoOpen(tmp, SCE_O_TRUNC | SCE_O_CREAT | SCE_O_WRONLY, 6);
	if (fd < 0) {
		printf("sceIoOpen: 0x%x\n", fd);
		ret = fd;
		goto end;
	}

	// draw progress bar background
	draw_rect(0, SCREEN_HEIGHT - PROGRESS_BAR_HEIGHT, PROGRESS_BAR_WIDTH, PROGRESS_BAR_HEIGHT, 0xFF666666);
	while (1) {
		int read = sceHttpReadData(req, buf, sizeof(buf));
		if (read < 0) {
			printf("sceHttpReadData error! 0x%x\n", read);
			ret = read;
			goto end2;
		}
		if (read == 0)
			break;

		if (exp_crc > 0)
			res_crc = crcb(res_crc, buf, read);

		ret = sceIoWrite(fd, buf, read);
		if (ret < 0 || ret != read) {
			printf("sceIoWrite error! 0x%x\n", ret);
			goto end2;
		}
		total_read += read;
		draw_rect(1, SCREEN_HEIGHT - PROGRESS_BAR_HEIGHT + 1, ((uint64_t)(PROGRESS_BAR_WIDTH - 2)) * total_read / length, PROGRESS_BAR_HEIGHT - 2, COLOR_GREEN);
	}
end2:
	sceIoClose(fd);
end:
	sceHttpDeleteRequest(req);
	sceHttpDeleteConnection(conn);

	if (res_crc != exp_crc) {
		printf("file checksum(CRC32) error!\n");
		return -1;
	}

	if (tmp != NULL) {
		sceIoRemove(file);
		ret = sceIoRename(tmp, file);
		if (ret < 0)
			printf("ud0 perms error!\n");
	}
	
	return ret;
}

void init_net(void) {
	sceSysmoduleLoadModule(SCE_SYSMODULE_NET);
	SceNetInitParam netInitParam;
	int size = 1 * 1024 * 1024;
	netInitParam.memory = malloc(size);
	netInitParam.size = size;
	netInitParam.flags = 0;
	sceNetInit(&netInitParam);
	sceNetCtlInit();
	sceSysmoduleLoadModule(SCE_SYSMODULE_HTTP);
	sceHttpInit(1 * 1024 * 1024);
}

void term_net(void) {
	sceHttpTerm();
	sceSysmoduleUnloadModule(SCE_SYSMODULE_HTTP);
	sceNetCtlTerm();
	sceNetTerm();
	sceSysmoduleUnloadModule(SCE_SYSMODULE_NET);
}

int pupDL(int ulink) {
	if (ulink > 4 || mlink > 1)
		return -1;
	sceKernelPowerLock(0); // don't want the screen to turn off during download
	sceIoMkdir("ud0:PSP2UPDATE", 0777);
	sceIoRemove("ud0:PSP2UPDATE/psp2swu.self");
	if (ulink != 0)
		sceIoRemove("ud0:PSP2UPDATE/PSP2UPDAT.PUP");
	int ret = 0;
	printf("Downloading the updater...\n");
dl_swu:
	if (download_file(swudll[mlink], "ud0:PSP2UPDATE/psp2swu.self", "ud0:PSP2UPDATE/psp2swu.self.TMP", 0x8853B7BD) < 0) {
		if (ret == 0) {
			printf("Interrupted, retrying...\n");
			ret = -1;
			goto dl_swu;
		}
		printf("Failed to download the updater.\n");
		goto syncexit;
	}
	ret = 0;
	if (ulink != 0) {
dl_part1:
		printf("Downloading the update package part 1...\n");
		if (download_file(fwdll[mlink][ulink - 1][0], "ud0:PSP2UPDATE/PSP2UPDAT.PUP.TMP", "ud0:PSP2UPDATE/PSP2UPDAT.PUP.P1", fwcrc[ulink - 1][0]) < 0) {
			if (ret == 0) {
				printf("Interrupted, retrying...\n");
				ret = -1;
				goto dl_part1;
			}
			printf("Failed to download part 1 of the update file.\n");
			goto syncexit;
		}
		ret = 0;
dl_part2:
		printf("Downloading the update package part 2...\n");
		if (download_file(fwdll[mlink][ulink - 1][1], "ud0:PSP2UPDATE/PSP2UPDAT.PUP.TMP", NULL, fwcrc[ulink - 1][1]) < 0) {
			if (ret == 0) {
				printf("Interrupted, retrying...\n");
				ret = -1;
				goto dl_part2;
			}
			printf("Failed to download part 2 of the update file.\n");
			goto syncexit;
		}
		if (sceIoRename("ud0:PSP2UPDATE/PSP2UPDAT.PUP.TMP", "ud0:PSP2UPDATE/PSP2UPDAT.PUP") < 0) {
			printf("final rename error!\n");
			ret = -1;
			goto syncexit;
		}
	}
	ret = 0;
syncexit:
	sceIoSync("ud0:", 0);
	sceIoSync("ud0:PSP2UPDATE/PSP2UPDAT.PUP", 0);
	sceIoSync("ud0:PSP2UPDATE/psp2swu.self", 0);
	sceKernelPowerUnlock(0);
	return ret;
}

int unzip(const char *src, const char *dst) {
	Zip* handle = ZipOpen(src);
	int ret = ZipExtract(handle, NULL, dst);
	ZipClose(handle);
	return ret;
}

int taiDL(int plink, int puppy) {
	if (plink > 4 || mlink > 1)
		return -1;
	printf("Backing up ur0:tai to ur0:tai_old...\n");
	sceIoRmdir("ur0:tai_old");
	sceIoRemove("ux0:tai/config.txt");
	if (plink == 1) {
		sceIoRename("ur0:tai", "ur0:tai_old");
		return 0;
	}
	copyDir("ur0:tai", "ur0:tai_old");
	sceIoRemove("ur0:tai/boot_config.txt");
	int retry = 1;
	if (plink != 0) {
		sceIoRemove("ud0:ur0-patch.zip");
		printf("Downloading the tai configuration...\n");
dl_urpatch:
		if (download_file(taidll[mlink][plink - 2], "ud0:ur0-patch.zip", "ud0:ur0-patch.zip.TMP", 0) < 0) {
			if (retry) {
				printf("Failed! Retrying...\n");
				retry = 0;
				goto dl_urpatch;
			}
			printf("Failed to download the tai config.\n");
			return -1;
		}
		sceIoSync("ud0:", 0);
		sceIoSync("ud0:ur0-patch.zip", 0);
	}
	printf("Extracting the tai configuration...\n");
	return unzip("ud0:ur0-patch.zip", (puppy) ? "ur0:" : "ud0:ur0-patch");
}

int e2xDL(void) {
	int retry = 1;
	sceIoRmdir("ud0:os0-patch");
	sceIoRemove("ud0:os0-patch.zip");
	printf("Downloading the enso_ex files...\n");
dl_ospatch:
	if (download_file(ensodll[mlink], "ud0:os0-patch.zip", "ud0:os0-patch.zip.TMP", 0) < 0) {
		if (retry) {
			printf("Failed! Retrying...\n");
			retry = 0;
			goto dl_ospatch;
		}
		printf("Failed to download the enso_ex files.\n");
		return -1;
	}
	sceIoSync("ud0:", 0);
	sceIoSync("ud0:os0-patch.zip", 0);
	printf("Extracting the enso_ex files...\n");
	return unzip("ud0:os0-patch.zip", "ud0:os0-patch");
}

int getApps(void) {
	if (mlink > 1)
		return -1;
	sceIoMkdir("ux0:downloads", 0777);
	int retry;
	for(int i=0; i<12; i-=-1) {
		if (appi_cfg[i]) {
			printf("Downloading %s...\n", appnames[i]);
			retry = 1;
dl_app:
			if (download_file(appdll[mlink][i], appvpks[i], "ux0:downloads/tmp.vpk", 0) < 0) {
				if (retry) {
					printf("Failed! Retrying...\n");
					retry = 0;
					goto dl_app;
				}
				printf("Failed to download %s.\n", appnames[i]);
				return -1;
			}
		}
	}
	printf("\nSaved app packages to ux0:downloads/\n");
	return 0;
}

void setShellDir(const char *dir, uint32_t dirlen) {
	if (dirlen > 127)
		return;
	char path[128];
	memset(path, 0, 128);
	memcpy(path, dir, dirlen);
	sceIoMkdir("ux0:VitaShell", 0777);
	sceIoMkdir("ux0:VitaShell/internal", 0777);
	int fd = sceIoOpen("ux0:VitaShell/internal/lastdir.txt", SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	if (fd < 0)
		return;
	sceIoWrite(fd, path, dirlen + 1);
	sceIoClose(fd);
	return;
}

int launchKernel(const char *kpath) {
	printf("Loading the kernel module...\n");
	if (fcp(kpath, "ud0:vd_ktmp.skprx") < 0)
		return -1;
	tai_module_args_t argg;
	memset(&argg, 0, sizeof(argg));
	argg.size = sizeof(argg);
	argg.pid = KERNEL_PID;
	int ret = taiLoadStartKernelModuleForUser("ud0:vd_ktmp.skprx", &argg);
	sceClibPrintf("kmodule: 0x%X\n", ret);
	return ret;
}

int main(int argc, char *argv[]) {
	psvDebugScreenInit();
	psvDebugScreenSetFgColor(COLOR_CYAN);
	printf("VitaDeploy v0.5 by SKGleba\n\n");
	psvDebugScreenSetFgColor(COLOR_YELLOW);
	sceIoSync("ud0:", 0);
	sceIoRemove("ud0:enso.eo");
	sceIoRemove("ud0:vd_kmtp.skprx");
	memset(appi_cfg, 0, 16);
	mlink = vdKUcmd(10, 0);
	int mode = vdKUcmd(5, 0);
	int tfw = vdKUcmd(6, 0);
	sceClibPrintf("mode %d\n", mode);
	switch(mode) {
		case 1:
			launchKernel(kernel[1]);
			break;
		case 2:
			init_net();
			if (pupDL(tfw) < 0)
				dead("Update download failed!\n");
			if (taiDL(vdKUcmd(7, 0), 1) < 0)
				dead("Tai patch install failed!\n");
			term_net();
			if (vdKUcmd(11, 0)) {
				if (tfw == 1 && fcp("app0:rdparty/enso_360.eo", "ud0:enso.eo") < 0)
					dead("Could not copy the enso 3.60 exploit!");
				else if (tfw == 2 && fcp("app0:rdparty/enso_365.eo", "ud0:enso.eo") < 0)
					dead("Could not copy the enso 3.65 exploit!");
			}
			if (launchKernel(kernel[2]) < 0)
				dead("Could not load the kernel module!\n");
			if (fcp("app0:rdparty/tiny_modoru.suprx", "ud0:tiny_modoru_user.suprx") < 0)
				dead("Could not copy the user module!\n");
			break;
		case 4:
			vdKUcmd(9, (void*)appi_cfg);
			init_net();
			if (getApps() < 0)
				dead("app downloader failed!\n");
			term_net();
			setShellDir("ux0:downloads/", sizeof("ux0:downloads/"));
			mode = 0;
			break;
		default:
			mode = 0;
			break;
			
	}
	printf("Launching target...\n");
	sceAppMgrLoadExec(minor[mode], NULL, NULL);
	sceKernelDelayThread(0.5 * 1000 * 1000);
	sceAppMgrLoadExec(minor[mode], NULL, NULL);
	dead("You really should not be seeing this\n");
	return 0;
}
