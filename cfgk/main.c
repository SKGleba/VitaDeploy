/*
 * Copyright (C) 2021 skgleba
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include <psp2kern/io/fcntl.h>
#include <psp2kern/kernel/cpu.h>
#include <psp2kern/kernel/modulemgr.h>
// #include <psp2kern/kernel/sysmem/data_transfers.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <taihen.h>

#include "cfgk.h"

#define ERNIE_SHUTDOWN_REBOOT 1

#define DACR_OFF(stmt)                                                        \
    do {                                                                      \
        unsigned prev_dacr;                                                   \
        __asm__ volatile("mrc p15, 0, %0, c3, c0, 0 \n" : "=r"(prev_dacr));   \
        __asm__ volatile("mcr p15, 0, %0, c3, c0, 0 \n" : : "r"(0xFFFF0000)); \
        stmt;                                                                 \
        __asm__ volatile("mcr p15, 0, %0, c3, c0, 0 \n" : : "r"(prev_dacr));  \
    } while (0)

typedef struct {
    const char* dev;
    const char* dev2;
    const char* blkdev;
    const char* blkdev2;
    int id;
} SceIoDevice;

typedef struct {
    int id;
    const char* dev_unix;
    int unk;
    int dev_major;
    int dev_minor;
    const char* dev_filesystem;
    int unk2;
    SceIoDevice* dev;
    int unk3;
    SceIoDevice* dev2;
    int unk4;
    int unk5;
    int unk6;
    int unk7;
} SceIoMountPoint;

static int patched = 0, loc = 0, larg = 0, minor = 0, fwdll = 0, taidll = 0, apex = 0, mdr_enso = 0;
static uint32_t fw = 0, minfw = 0;
static char appi_cfg[16];

// load our user module asap
static tai_hook_ref_t g_start_preloaded_modules_hook;
static int start_preloaded_modules_patched(SceUID pid) {
    int ret = TAI_CONTINUE(int, g_start_preloaded_modules_hook, pid);
    if (!patched) {
        if (ksceKernelLoadStartModuleForPid(pid, "ux0:app/SKGD3PL0Y/plugins/cfgu_v05.suprx", 0, NULL, 0, NULL, NULL) < 0) {
            ksceKernelLoadStartModuleForPid(pid, "vs0:app/NPXS10000/plugins/cfgu_v05.suprx", 0, NULL, 0, NULL, NULL);
            loc = 1;
        }
        patched = 1;
    }
    return ret;
}

// mount [blkn] as grw0:
static SceIoDevice custom;
int cmount_part(const char* blkn) {
    SceIoMountPoint* (*sceIoFindMountPoint)(int id) = NULL;
    uint32_t iofindmp_off = (fw > 0x03630000) ? 0x182f5 : 0x138c1;
    if (fw > 0x03680011)
        iofindmp_off = 0x18735;
    if (module_get_offset(KERNEL_PID, ksceKernelSearchModuleByName("SceIofilemgr"), 0, iofindmp_off, (uintptr_t*)&sceIoFindMountPoint) < 0 ||
        sceIoFindMountPoint == NULL)
        return -1;
    SceIoMountPoint* mountp;
    mountp = sceIoFindMountPoint(0xA00);
    if (mountp == NULL)
        return -1;
    custom.dev = mountp->dev->dev;
    custom.dev2 = mountp->dev->dev2;
    custom.blkdev = custom.blkdev2 = blkn;
    custom.id = 0xA00;
    DACR_OFF(mountp->dev = &custom;);
    ksceIoUmount(0xA00, 0, 0, 0);
    ksceIoUmount(0xA00, 1, 0, 0);
    return ksceIoMount(0xA00, NULL, 0, 0, 0, 0);
}

int abby_reset_nocalib(void) {
    int status = 0;

    // end all pending transactions
    for (int i = 0; i < 4; i -= -1) {
        ksceSysconAbbySync(&status);
        if (status << 0x19 < 0)
            break;
    }

    // Fake the post-update reset
    ksceSysconBatterySWReset();

    // Reboot via ernie, it should shut down instead if abby was reset
    // NOTE: race, we need to do that before abby requests calibration data from ernie
    ksceSysconErnieShutdown(ERNIE_SHUTDOWN_REBOOT);

    while (1) {
    };
}

int vdKUcmd(int cmd, uint32_t arg) {
    int ret = 0;
    switch (cmd) {
        case CFGK_BRIDGE_SET_PATCHED:
            patched = arg;
            break;
        case CFGK_BRIDGE_EXT2GRW:
            ret = cmount_part("sdstor0:ext-lp-act-entire");
            break;
        case CFGK_BRIDGE_GET_LOC:
            ret = loc;
            break;
        case CFGK_BRIDGE_GET_MINFW:
            if (arg == 0)
                ret = (int)minfw;
            else
                ret = (minfw > arg);
            break;
        case CFGK_BRIDGE_GET_SET_LAUNCHARG:
            ret = larg;
            larg = arg;
            break;
        case CFGK_BRIDGE_GET_SET_SUBMAIN:
            ret = minor;
            minor = arg;
            break;
        case CFGK_BRIDGE_GET_SET_FW_DL_LINK:
            ret = fwdll;
            fwdll = arg;
            break;
        case CFGK_BRIDGE_GET_SET_TAI_DL_LINK:
            ret = taidll;
            taidll = arg;
            break;
        case CFGK_BRIDGE_SET_APP_INSTALLER_CFG:
            ENTER_SYSCALL(ret);
            ksceKernelMemcpyUserToKernel(appi_cfg, arg, 16);
            EXIT_SYSCALL(ret);
            ret = 0;
            break;
        case CFGK_BRIDGE_GET_APP_INSTALLER_CFG:
            ENTER_SYSCALL(ret);
            ksceKernelMemcpyKernelToUser(arg, appi_cfg, 16);
            EXIT_SYSCALL(ret);
            ret = 0;
            break;
        case CFGK_BRIDGE_GET_SET_REMOTE:
            ret = apex;
            apex = arg;
            break;
        case CFGK_BRIDGE_GET_SET_ENSO_FLAG:
            ret = mdr_enso;
            mdr_enso = arg;
            break;
        case CFGK_BRIDGE_ABBY_RESET:
            ENTER_SYSCALL(ret);
            abby_reset_nocalib();
            EXIT_SYSCALL(ret);
            ret = 0;
            break;
    }
    return ret;
}

void _start() __attribute__((weak, alias("module_start")));
int module_start(SceSize args, void* argp) {
    fw = *(uint32_t*)(*(int*)(ksceSysrootGetSysroot() + 0x6c) + 4);
    minfw = *(uint32_t*)(*(int*)(ksceSysrootGetSysroot() + 0x6c) + 8);
    larg = 1;
    memset(appi_cfg, 0, sizeof(appi_cfg));

    // allow GC-SD
    char movsr01[2] = {0x01, 0x20};
    int sysmem_modid = ksceKernelSearchModuleByName("SceSysmem");
    if (taiInjectDataForKernel(KERNEL_PID, sysmem_modid, 0, 0x21610, movsr01, sizeof(movsr01)) < 0)
        return SCE_KERNEL_START_FAILED;

    // patch start_loaded_modules_for_pid to loadstart our umodule
    int starthook = taiHookFunctionExportForKernel(KERNEL_PID, &g_start_preloaded_modules_hook, "SceKernelModulemgr", 0xC445FA63, 0x432DCC7A,
                                                   start_preloaded_modules_patched);
    if (starthook < 0) {  // probs 3.65
        starthook = taiHookFunctionExportForKernel(KERNEL_PID, &g_start_preloaded_modules_hook, "SceKernelModulemgr", 0x92C9FFC2, 0x998C7AE9,
                                                   start_preloaded_modules_patched);
        if (starthook < 0)  // :shrug:
            return SCE_KERNEL_START_FAILED;
    }

    return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize args, void* argp) {
    return SCE_KERNEL_STOP_SUCCESS;
}
