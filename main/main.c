/*
 * Copyright (C) 2021-2023 skgleba
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

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
#include "unzip.h"
#include "crc32.c"

#define printf psvDebugScreenPrintf
#define COLORPRINTF(color, ...)                \
	do {                                       \
		psvDebugScreenSetFgColor(color);       \
		printf(__VA_ARGS__);     \
		psvDebugScreenSetFgColor(COLOR_YELLOW); \
	} while (0)

#define CHUNK_SIZE 64 * 1024
#define hasEndSlash(path) (path[strlen(path) - 1] == '/')

#define SKG_DOMAIN '@w@'
#define HMV_DOMAIN (((('srv' + '0') ^ SKG_DOMAIN) << 0b10) * 0x18) + (('Tu@' - '0') >> 2)
#define DEV_DOMAIN "917hu8k0n73n7.psp2.dev"

#define LOCALNET 0
#define PC_IP_STRING "127.0.0.1"

#define VERSION_STRING "VitaDeploy v1.2.1 by SKGleba"

#define DLC_APP_COUNT 12

static int mlink = 0;
static char appi_cfg[16], dl_link_buf[128];
const char minor[4][32] = {"app0:vshell.self","app0:imcunlock.self","app0:tiny_modoru.self","app0:fwtool_portable.self"};
const char kernel[4][32] = {"","app0:plugins/imcunlock.skprx","app0:plugins/tiny_modoru.skprx","app0:plugins/fwtool.skprx"};
const char taizips[3][16] = { "v_tai.zip","r_tai.zip","y_tai.zip" };
const char* ensozip = "e2x_def.zip";
const char* swu = "SWU.00";

const char fwparts[3][2][8] = {
	{ "360.01","360.02"},
	{ "365.01","365.02"},
	{ "368.01","368.02"}
};

const uint32_t fwcrc[3][2] = {
	{0x71BF741C, 0x3359C77B},
	{0xA475D0BD, 0x647EDCDF},
	{0xA1C5DE91, 0x90931C42}
};

const char apps[3][DLC_APP_COUNT][32] = {
{ // names on the server
	"shell.vpk",
	"vdbdl.vpk",
	"itls.vpk",
	"enso.vpk",
	"yamt.vpk",
	"adre.vpk",
	"pkgj.vpk",
	"savemgr.vpk",
	"thememgr.vpk",
	"battery.vpk",
	"reg.vpk",
	"ident.vpk",
},
{ // names to display
	"VitaShell",
	"VitaDB Downloader",
	"the iTLS installer",
	"the enso installer",
	"the YAMT installer",
	"Adrenaline",
	"PKGj",
	"vita-savemgr",
	"Custom Themes Manager",
	"batteryFixer",
	"registry editor",
	"PSVident",
},
{ // names on vita
	"ux0:downloads/VitaShell.vpk",
	"ux0:downloads/vdbdl.vpk",
	"ux0:downloads/iTLS.vpk",
	"ux0:downloads/enso.vpk",
	"ux0:downloads/YAMT.vpk",
	"ux0:downloads/Adrenaline.vpk",
	"ux0:downloads/PKGj.vpk",
	"ux0:downloads/vita-savemgr.vpk",
	"ux0:downloads/ThemeManager.vpk",
	"ux0:downloads/batteryFixer.vpk",
	"ux0:downloads/regEdit.vpk",
	"ux0:downloads/PSVident.vpk",
}
};

#include "ops.c" // too clogged otherwise

void dead(const char* msg) {
	psvDebugScreenSetFgColor(COLOR_RED);
	printf("\nVitaDeploy stopped with the following error:\n\n> %s\n", msg);
	psvDebugScreenSetFgColor(COLOR_WHITE);
	printf("you can now exit this app\n");
	while (1) {};
}

int launchKernel(const char* kpath) {
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

void setShellDir(const char* dir, uint32_t dirlen) {
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
	sceClibMemset(dl_link_buf, 0, 128);
#if LOCALNET
	sceClibSnprintf(dl_link_buf, 128, "http://" PC_IP_STRING "/bin/%s", swu);
#else
	if (mlink)
		sceClibSnprintf(dl_link_buf, 128, "http://bkp.%x.xyz/bin/%s", HMV_DOMAIN, swu);
	else
		sceClibSnprintf(dl_link_buf, 128, "http://%s/bin/%s", DEV_DOMAIN, swu);
#endif
dl_swu:
	if (download_file(dl_link_buf, "ud0:PSP2UPDATE/psp2swu.self", "ud0:PSP2UPDATE/psp2swu.self.TMP", 0x8853B7BD) < 0) {
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
		sceClibMemset(dl_link_buf, 0, 128);
#if LOCALNET
		sceClibSnprintf(dl_link_buf, 128, "http://" PC_IP_STRING "/bin/%s", fwparts[ulink - 1][0]);
#else
		if (mlink)
			sceClibSnprintf(dl_link_buf, 128, "http://bkp.%x.xyz/bin/%s", HMV_DOMAIN, fwparts[ulink - 1][0]);
		else
			sceClibSnprintf(dl_link_buf, 128, "http://%s/bin/%s", DEV_DOMAIN, fwparts[ulink - 1][0]);
#endif
dl_part1:
		printf("Downloading the update package part 1...\n");
		if (download_file(dl_link_buf, "ud0:PSP2UPDATE/PSP2UPDAT.PUP.TMP", "ud0:PSP2UPDATE/PSP2UPDAT.PUP.P1", fwcrc[ulink - 1][0]) < 0) {
			if (ret == 0) {
				printf("Interrupted, retrying...\n");
				ret = -1;
				goto dl_part1;
			}
			printf("Failed to download part 1 of the update file.\n");
			goto syncexit;
		}
		ret = 0;
		sceClibMemset(dl_link_buf, 0, 128);
#if LOCALNET
		sceClibSnprintf(dl_link_buf, 128, "http://" PC_IP_STRING "/bin/%s", fwparts[ulink - 1][1]);
#else
		if (mlink)
			sceClibSnprintf(dl_link_buf, 128, "http://bkp.%x.xyz/bin/%s", HMV_DOMAIN, fwparts[ulink - 1][1]);
		else
			sceClibSnprintf(dl_link_buf, 128, "http://%s/bin/%s", DEV_DOMAIN, fwparts[ulink - 1][1]);
#endif
dl_part2:
		printf("Downloading the update package part 2...\n");
		if (download_file(dl_link_buf, "ud0:PSP2UPDATE/PSP2UPDAT.PUP.TMP", NULL, fwcrc[ulink - 1][1]) < 0) {
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
		sceClibMemset(dl_link_buf, 0, 128);
#if LOCALNET
		sceClibSnprintf(dl_link_buf, 128, "http://" PC_IP_STRING "/tai/%s", taizips[plink - 2]);
#else
		if (mlink)
			sceClibSnprintf(dl_link_buf, 128, "http://bkp.%x.xyz/tai/%s", HMV_DOMAIN, taizips[plink - 2]);
		else
			sceClibSnprintf(dl_link_buf, 128, "http://%s/tai/%s", DEV_DOMAIN, taizips[plink - 2]);
#endif
dl_urpatch:
		if (download_file(dl_link_buf, "ud0:ur0-patch.zip", "ud0:ur0-patch.zip.TMP", 0) < 0) {
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
	return extract("ud0:ur0-patch.zip", (puppy) ? "ur0:" : "ud0:ur0-patch");
}

int getApps(void) {
	if (mlink > 1)
		return -1;
	if (load_sce_paf() < 0)
		COLORPRINTF(COLOR_RED, "Could not load the PAF module!");
	sceIoMkdir("ux0:downloads", 0777);
	int retry;
	for(int i=0; i<DLC_APP_COUNT; i-=-1) {
		if (appi_cfg[i]) {
			printf("%s : ", apps[1][i]);
			retry = 1;
			sceClibMemset(dl_link_buf, 0, 128);
#if LOCALNET
			sceClibSnprintf(dl_link_buf, 128, "http://" PC_IP_STRING "/vpk/%s", apps[0][i]);
#else
			if (mlink)
				sceClibSnprintf(dl_link_buf, 128, "http://bkp.%x.xyz/vpk/%s", HMV_DOMAIN, apps[0][i]);
			else
				sceClibSnprintf(dl_link_buf, 128, "http://%s/vpk/%s", DEV_DOMAIN, apps[0][i]);
#endif
dl_app:
			COLORPRINTF(COLOR_PURPLE, "download..");
			if (download_file(dl_link_buf, apps[2][i], "ux0:downloads/tmp.vpk", 0) < 0) {
				if (retry) {
					COLORPRINTF(COLOR_RED, "failed, retry ");
					retry--;
					goto dl_app;
				}
				COLORPRINTF(COLOR_RED, "failed\nFailed to download %s.\n", apps[1][i]);
				return -1;
			}
			COLORPRINTF(COLOR_PURPLE, "extract..");
			removeDir("ux0:temp/app");
			sceIoMkdir("ux0:temp/app", 0777);
			if (extract(apps[2][i], "ux0:temp/app") < 0) {
				COLORPRINTF(COLOR_RED, "failed\nFailed to extract %s.\n", apps[2][i]);
				return -1;
			}
			COLORPRINTF(COLOR_PURPLE, "promote..");
			if (promoteApp("ux0:temp/app") < 0) {
				COLORPRINTF(COLOR_RED, "failed\nFailed to promote ux0:temp/app\n");
				return -1;
			}
			printf(" : ");
			COLORPRINTF(COLOR_GREEN, "OK\n");
		}
	}
	printf("\nAll apps have been successfully installed\n");
	unload_sce_paf();
	return 0;
}

void vs0install(void) {
	COLORPRINTF(COLOR_WHITE, "This replaces the discontinued \"near\" app with VitaDeploy.\nRequires reboot to take effect.\n\n");
	COLORPRINTF(COLOR_YELLOW, "WARNING: this will cause your bubble layout to be reset\n\n");
	COLORPRINTF(COLOR_CYAN, "SQUARE: Replace NEAR with VitaDeploy\nTRIANGLE: Restore NEAR\nCIRCLE: Exit\n");
	sceKernelDelayThread(0.5 * 1000 * 1000);
	SceCtrlData pad;
	while (1) {
		sceCtrlPeekBufferPositive(0, &pad, 1);
		if (pad.buttons == SCE_CTRL_SQUARE)
			break;
		else if (pad.buttons == SCE_CTRL_CIRCLE)
			return;
		else if (pad.buttons == SCE_CTRL_TRIANGLE) {
			printf("Preparing to restore NEAR..\n");
			void* buf = malloc(0x100);
			vshIoUmount(0x300, 0, 0, 0);
			vshIoUmount(0x300, 1, 0, 0);
			_vshIoMount(0x300, 0, 2, buf);
			sceIoMkdir("ux0:temp/", 0777);
			removeDir("ux0:temp/app");
			sceIoMkdir("ux0:temp/app", 0777);
			int ret = copyDir("vs0:app/NPXS10000/near_backup", "ux0:temp/app");
			if (ret < 0)
				dead("Could not prepare NEAR for restore!\n");
			printf("Removing VitaDeploy..\n");
			ret = removeDir("vs0:app/NPXS10000");
			if (ret < 0) {
				printf("Failed 0x%08X, rebooting in 5s\n", ret);
				sceKernelDelayThread(5 * 1000 * 1000);
				scePowerRequestColdReset();
				sceKernelDelayThread(2 * 1000 * 1000);
				dead("You really should not be seeing this\n");
			}
			printf("Restoring NEAR..\n");
			ret = copyDir("ux0:temp/app", "vs0:app/NPXS10000");
			if (ret < 0)
				dead("NEAR restore failed!\n");
			printf("Removing the application database\n");
			sceIoRemove("ur0:shell/db/app.db");
			printf("Cleaning up..\n");
			removeDir("ux0:temp/app");
			printf("All done, rebooting in 5s\n");
			sceKernelDelayThread(5 * 1000 * 1000);
			scePowerRequestColdReset();
			sceKernelDelayThread(2 * 1000 * 1000);
			dead("You really should not be seeing this\n");
		}
	}

	printf("Backing up VitaDeploy\n");

	void* buf = malloc(0x100);
	vshIoUmount(0x300, 0, 0, 0);
	vshIoUmount(0x300, 1, 0, 0);
	_vshIoMount(0x300, 0, 2, buf);
	
	sceIoMkdir("ux0:temp/", 0777);
	removeDir("ux0:temp/app");
	sceIoMkdir("ux0:temp/app", 0777);
	int res = copyDir("ux0:app/SKGD3PL0Y", "ux0:temp/app");
	if (res < 0)
		dead("VitaDeploy backup failed!\n");

	printf("Backing up NEAR\n");
	res = copyDir("vs0:app/NPXS10000", "ux0:temp/app/near_backup");
	if (res < 0)
		dead("NEAR backup failed!\n");

	printf("Preparing to replace near..\n");
	sceIoRemove("ux0:temp/app/sce_sys/param.sfo");
	sceIoRename("ux0:temp/app/sce_sys/vs.sfo", "ux0:temp/app/sce_sys/param.sfo");

	printf("Replacing NEAR\n");
	removeDir("vs0:app/NPXS10000");
	res = copyDir("ux0:temp/app", "vs0:app/NPXS10000");
	if (res < 0)
		dead("Could not replace NEAR!\n");

	printf("Removing the application database\n");
	sceIoRemove("ur0:shell/db/app.db");

	printf("Cleaning up..\n");
	removeDir("ux0:temp/app");

	printf("All done, rebooting in 5s\n");
	sceKernelDelayThread(5 * 1000 * 1000);
	scePowerRequestColdReset();
	sceKernelDelayThread(2 * 1000 * 1000);
	dead("You really should not be seeing this\n");
}

int tryLocalUdZip(int puppy) {
	sceIoRemove("ur0:DELETE_ME.VDTMP");
	if (sceIoRename("ur0:vd-udl.zip", "ur0:DELETE_ME.VDTMP") >= 0) {
		printf("Skipping update and tai download!\nExtracting ur0:vd-udl.zip...\n");
		if (extract("ur0:DELETE_ME.VDTMP", "ud0:") < 0)
			dead("invalid zip file!");
		printf("Extract tai configuration: %s\n", (extract("ud0:ur0-patch.zip", (puppy) ? "ur0:" : "ud0:ur0-patch") < 0) ? "failed!" : "ok");
		sceIoRename("ur0:DELETE_ME.VDTMP", "ur0:vd-udl.zip");
		return 0;
	}
	return -1;
}

int main(int argc, char *argv[]) {
	psvDebugScreenInit();
	psvDebugScreenSetFgColor(COLOR_CYAN);
	printf(VERSION_STRING "\n\n");
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
			if (tryLocalUdZip(1) < 0) {
				net(1);
				if (pupDL(tfw) < 0)
					dead("Update download failed!\n");
				if (taiDL(vdKUcmd(7, 0), 1) < 0)
					dead("Tai patch install failed!\n");
				net(0);
			}
			if (vshSblAimgrIsGenuineDolce() && fcp("app0:rdparty/tv-cfg.txt", "ur0:tai/boot_config.txt") < 0)
				dead("PSTV bootconfig install failed!\n");
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
			net(1);
			if (getApps() < 0)
				dead("app installer failed!\n");
			net(0);
			psvDebugScreenSetFgColor(COLOR_WHITE);
			printf("you can now exit this app\n");
			while (1) {};
			mode = 0;
			break;
		case 5:
			vs0install();
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
