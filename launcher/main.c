/*
 * Copyright (C) 2021 skgleba
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include <psp2/io/fcntl.h>
#include <psp2/kernel/processmgr.h>

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <taihen.h>

#include "../cfgk/cfgk.h"

int main(int argc, char *argv[]) {
	
	tai_module_args_t argg;
	argg.size = sizeof(argg);
	argg.pid = KERNEL_PID;
	argg.args = 0;
	argg.argp = NULL;
	argg.flags = 0;
	uint32_t ls = taiLoadStartKernelModuleForUser("ux0:app/SKGD3PL0Y/plugins/cfgk_v05.skprx", &argg); // first try ux0
	if (ls == 0x80010002 || ls == 0x80010013) // not found
		ls = taiLoadStartKernelModuleForUser("vs0:app/NPXS10000/plugins/cfgk_v05.skprx", &argg); // try vs0
	
	sceClibPrintf("load_kernel: 0x%X\n", ls);
	
	if (ls == 0x8002D013 || ls == 0x800D2000) { // already loaded
        ls = vdKUcmd(CFGK_BRIDGE_GET_SET_LAUNCHARG, 1);
        if ((int)ls > 0)
            vdKUcmd(CFGK_BRIDGE_SET_PATCHED, 0);
    }
	
	if ((int)ls < 0) // other error
		sceAppMgrLoadExec("app0:vshell.self", NULL, NULL);
	else if (ls == 0) // VitaDeploy
		sceAppMgrLoadExec("app0:main.self", NULL, NULL);
	else // in-app settings menu
		sceAppMgrLaunchAppByUri(0xFFFFF, "settings_dlg:");
	
	sceKernelExitProcess(0);
	return 0;
}
