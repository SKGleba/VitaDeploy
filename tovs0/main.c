#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/modulemgr.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/devctl.h>
#include <psp2/ctrl.h>
#include <psp2/shellutil.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/io/stat.h>
#include <psp2/io/dirent.h>

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <taihen.h>
#include "graphics.h"

#define printf psvDebugScreenPrintf
#define CHUNK_SIZE 64 * 1024
#define hasEndSlash(path) (path[strlen(path) - 1] == '/')

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
	printf("\nVD2VS0 stopped with the following error:\n\n> %s\n", msg);
	psvDebugScreenSetFgColor(COLOR_WHITE);
	printf("you can now exit this app\n");
	while (1) {};
}

int main(int argc, char *argv[]) {
	psvDebugScreenInit();
	psvDebugScreenSetFgColor(COLOR_CYAN);
	printf("Replacing NEAR with VitaDeploy...\n\n");
	psvDebugScreenSetFgColor(COLOR_YELLOW);
	void* buf = malloc(0x100);
	vshIoUmount(0x300, 0, 0, 0);
	vshIoUmount(0x300, 1, 0, 0);
	_vshIoMount(0x300, 0, 2, buf);
	if (copyDir("ux0:app/SKGD3PL0Y", "vs0:app/NPXS10000") < 0)
		dead("Could not copyDir!\n");
	if (sceIoRemove("vs0:app/NPXS10000/sce_sys/param.sfo") < 0 || sceIoRename("vs0:app/NPXS10000/sce_sys/vs.sfo", "vs0:app/NPXS10000/sce_sys/param.sfo") < 0)
		dead("Could not update the SFO!");
	if (sceIoRemove("ur0:shell/db/app.db") < 0)
		dead("app.db remove failed\n");
	scePowerRequestColdReset();
	sceKernelDelayThread(2 * 1000 * 1000);
	dead("You really should not be seeing this\n");
	return 0;
}
