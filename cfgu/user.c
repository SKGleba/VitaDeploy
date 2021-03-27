/*
 * Copyright (C) 2021 skgleba
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include <psp2/kernel/clib.h>
#include <psp2/kernel/modulemgr.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/registrymgr.h>
#include <psp2/system_param.h>
#include <psp2/power.h>
#include <taihen.h>
#include <string.h>
#include "FatFormatProxy.h"

// custom system settings menu
extern unsigned char _binary_system_settings_xml_start;
extern unsigned char _binary_system_settings_xml_size;

// storage sdstor0 paths
static char *stor_str[] = {
	"sdstor0:ext-lp-ign-entire",
	"sdstor0:uma-pp-act-a",
	"sdstor0:uma-lp-act-entire",
	"sdstor0:int-lp-ign-userext",
	"sdstor0:xmc-lp-ign-userext"
};

const char* app_id[] = {
  "vshl",
  "vhbb",
  "itls",
  "enso",
  "yamt",
  "adre",
  "pkgj",
  "save",
  "thme",
  "batf",
  "rege",
  "vidr"
};

const uint32_t fw_n[4] = { 0x03730011, 0x03600011, 0x03650011, 0x03680011 };

static int storno = 0, target_fs = F_TYPE_EXFAT;
static int fwv = 0, taiv = 0, repo = 0, mdr_enso = 0, tv = 0;
static SceUID g_hooks[7];
static char appi_cfg[16];

static tai_hook_ref_t g_sceRegMgrGetKeyInt_SceSystemSettingsCore_hook;
static int sceRegMgrGetKeyInt_SceSystemSettingsCore_patched(const char *category, const char *name, int *value) {
  if (sceClibStrncmp(category, "/CONFIG/FRDR", 12) == 0) {
    if (value) {
      if (sceClibStrncmp(name, "target", 6) == 0)
        *value = storno;
      else if (sceClibStrncmp(name, "fstype", 6) == 0)
        *value = target_fs;
    }
    return 0;
  } else if (sceClibStrncmp(category, "/CONFIG/CMDR", 12) == 0) {
    if (value) {
      if (sceClibStrncmp(name, "target", 6) == 0)
        *value = fwv;
      else if (sceClibStrncmp(name, "taicfg", 6) == 0)
        *value = taiv;
      else if (sceClibStrncmp(name, "backup", 6) == 0)
        *value = repo;
      else if (sceClibStrncmp(name, "ensonx", 6) == 0)
        *value = mdr_enso;
    }
    return 0;
  } else if (sceClibStrncmp(category, "/CONFIG/APPI", 12) == 0) {
    if (value) {
      for (int i = 0; i < 12; i -= -1) {
        if (sceClibStrncmp(name, app_id[i], 4) == 0)
          *value = appi_cfg[i];
      }
    }
    return 0;
  }
  return TAI_CONTINUE(int, g_sceRegMgrGetKeyInt_SceSystemSettingsCore_hook, category, name, value);
}

static tai_hook_ref_t g_sceRegMgrSetKeyInt_SceSystemSettingsCore_hook;
static int sceRegMgrSetKeyInt_SceSystemSettingsCore_patched(const char *category, const char *name, int value) {
  if (sceClibStrncmp(category, "/CONFIG/FRDR", 12) == 0) {
	  if (sceClibStrncmp(name, "fstype", 6) == 0)
	    target_fs = value;
    else if (sceClibStrncmp(name, "target", 6) == 0)
      storno = value;
    return 0;
  } else if (sceClibStrncmp(category, "/CONFIG/CMDR", 12) == 0) {
    if (sceClibStrncmp(name, "taicfg", 6) == 0) {
      if (tv && value == 4)
        value = 3;
      taiv = value;
    } else if (sceClibStrncmp(name, "target", 6) == 0)
      fwv = value;
    else if (sceClibStrncmp(name, "backup", 6) == 0)
      repo = value;
    else if (sceClibStrncmp(name, "ensonx", 6) == 0)
      mdr_enso = value;
    return 0;
  } else if (sceClibStrncmp(category, "/CONFIG/APPI", 12) == 0) {
    for(int i=0; i < 12; i-=-1) {
      if (sceClibStrncmp(name, app_id[i], 4) == 0)
        appi_cfg[i] = value;
    }
    return 0;
  }
  return TAI_CONTINUE(int, g_sceRegMgrSetKeyInt_SceSystemSettingsCore_hook, category, name, value);
}

static int (* g_OnButtonEventSettings_hook)(const char *id, int a2, void *a3);
static int OnButtonEventSettings_patched(const char *id, int a2, void *a3) {
  if (sceClibStrncmp(id, "id_frdr_format", 14) == 0)
    return formatBlkDev(stor_str[storno], target_fs, (storno < 3) ? 1 : 0);
  else if (sceClibStrncmp(id, "id_frdr_reboot", 14) == 0)
    return scePowerRequestColdReset();
  else if (sceClibStrncmp(id, "id_gc", 5) == 0)
    return vdKUcmd(1, 0);
  else if (sceClibStrncmp(id, "id_dm", 5) == 0)
    return vshSysconIduModeClear();
  else if (sceClibStrncmp(id, "id_pm", 5) == 0)
    return sceSblPmMgrSetProductModeOffForUser();
  else if (sceClibStrncmp(id, "id_ms", 5) == 0) {
    vdKUcmd(4, -1);
    return sceAppMgrLaunchAppByUri(0xFFFFF, (vdKUcmd(2, 0)) ? "near:" : "psgm:play?titleid=SKGD3PL0Y");
  } else if (sceClibStrncmp(id, "id_iu", 5) == 0) {
    vdKUcmd(4, 0);
    vdKUcmd(5, 1);
    return sceAppMgrLaunchAppByUri(0xFFFFF, (vdKUcmd(2, 0)) ? "near:" : "psgm:play?titleid=SKGD3PL0Y");
  } else if (sceClibStrncmp(id, "id_qi", 5) == 0) {
    if (vdKUcmd(3, fw_n[2]))
      return -1;
    vdKUcmd(4, 0);
    vdKUcmd(5, 2);
    vdKUcmd(6, 2);
    vdKUcmd(7, (tv) ? 3 : 4);
    vdKUcmd(10, repo);
    vdKUcmd(11, 1);
    return sceAppMgrLaunchAppByUri(0xFFFFF, (vdKUcmd(2, 0)) ? "near:" : "psgm:play?titleid=SKGD3PL0Y");
  } else if (sceClibStrncmp(id, "id_uf", 5) == 0) {
    if (formatBlkDev("sdstor0:int-lp-ign-updater", F_TYPE_FAT16, 0) < 0)
      return -1;
    return scePowerRequestColdReset();
  } else if (sceClibStrncmp(id, "id_ip", 5) == 0) {
    if (vdKUcmd(3, fw_n[fwv]))
      return -1;
    vdKUcmd(4, 0);
    vdKUcmd(5, 2);
    vdKUcmd(6, fwv);
    vdKUcmd(7, taiv);
    vdKUcmd(10, repo);
    if (fwv == 1 || fwv == 2)
      vdKUcmd(11, mdr_enso);
    return sceAppMgrLaunchAppByUri(0xFFFFF, (vdKUcmd(2, 0)) ? "near:" : "psgm:play?titleid=SKGD3PL0Y");
  } else if (sceClibStrncmp(id, "id_na", 5) == 0) {
    tai_module_args_t argg;
    sceClibMemset(&argg, 0, sizeof(argg));
    argg.size = sizeof(argg);
    argg.pid = KERNEL_PID;
    if (taiLoadStartKernelModuleForUser((vdKUcmd(2, 0)) ? "vs0:app/NPXS10000/plugins/naavls.skprx" : "ux0:app/SKGD3PL0Y/plugins/naavls.skprx", &argg) < 0)
      return -1;
    return 0;
  } else if (sceClibStrncmp(id, "id_ia", 5) == 0) {
    vdKUcmd(4, 0);
    vdKUcmd(5, 4);
    vdKUcmd(8, (void *)appi_cfg);
    vdKUcmd(10, repo);
    return sceAppMgrLaunchAppByUri(0xFFFFF, (vdKUcmd(2, 0)) ? "near:" : "psgm:play?titleid=SKGD3PL0Y");
  } else if (sceClibStrncmp(id, "id_vi", 5) == 0) {
    if (vdKUcmd(2, 0))
      return -1;
    vdKUcmd(4, 0);
    vdKUcmd(5, 5);
    return sceAppMgrLaunchAppByUri(0xFFFFF, "psgm:play?titleid=SKGD3PL0Y");
  }
  return g_OnButtonEventSettings_hook(id, a2, a3);
}

static tai_hook_ref_t g_scePafToplevelInitPluginFunctions_SceSettings_hook;
static int scePafToplevelInitPluginFunctions_SceSettings_patched(void *a1, int a2, uint32_t *funcs) {
  int res = TAI_CONTINUE(int, g_scePafToplevelInitPluginFunctions_SceSettings_hook, a1, a2, funcs);
  if (funcs[6] != (uint32_t)OnButtonEventSettings_patched) {
    g_OnButtonEventSettings_hook = (void *)funcs[6];
    funcs[6] = (uint32_t)OnButtonEventSettings_patched;
  }
  return res;
}

typedef struct {
  int size;
  const char *name;
  int type;
  int unk;
} SceRegMgrKeysInfo;

static tai_hook_ref_t g_sceRegMgrGetKeysInfo_SceSystemSettingsCore_hook;
static int sceRegMgrGetKeysInfo_SceSystemSettingsCore_patched(const char *category, SceRegMgrKeysInfo *info, int unk) {
  if (sceClibStrncmp(category, "/CONFIG/FRDR", 12) == 0 
    || sceClibStrncmp(category, "/CONFIG/CMDR", 12) == 0
    || sceClibStrncmp(category, "/CONFIG/APPI", 12) == 0) {
    if (info)
        info->type = 0x00040000; // type integer
    return 0;
  }
  return TAI_CONTINUE(int, g_sceRegMgrGetKeysInfo_SceSystemSettingsCore_hook, category, info, unk);
}

static tai_hook_ref_t g_scePafMiscLoadXmlLayout_SceSettings_hook;
static int scePafMiscLoadXmlLayout_SceSettings_patched(int a1, void *xml_buf, int xml_size, int a4) {
  if ((82+22) < xml_size && sceClibStrncmp(xml_buf+82, "system_settings_plugin", 22) == 0) {
    xml_buf = (void *)&_binary_system_settings_xml_start;
    xml_size = (int)&_binary_system_settings_xml_size;
  }
  return TAI_CONTINUE(int, g_scePafMiscLoadXmlLayout_SceSettings_hook, a1, xml_buf, xml_size, a4);
}

static SceUID g_system_settings_core_modid = -1;
static tai_hook_ref_t g_sceKernelLoadStartModule_SceSettings_hook;
static SceUID sceKernelLoadStartModule_SceSettings_patched(char *path, SceSize args, void *argp, int flags, SceKernelLMOption *option, int *status) {
  SceUID ret = TAI_CONTINUE(SceUID, g_sceKernelLoadStartModule_SceSettings_hook, path, args, argp, flags, option, status);
  if (ret >= 0 && sceClibStrncmp(path, "vs0:app/NPXS10015/system_settings_core.suprx", 44) == 0) {
    g_system_settings_core_modid = ret;
    g_hooks[2] = taiHookFunctionImport(&g_scePafMiscLoadXmlLayout_SceSettings_hook, 
                                        "SceSettings", 
                                        0x3D643CE8, // ScePafMisc
                                        0x19FE55A8, 
                                        scePafMiscLoadXmlLayout_SceSettings_patched);
    g_hooks[3] = taiHookFunctionImport(&g_sceRegMgrGetKeyInt_SceSystemSettingsCore_hook, 
                                        "SceSystemSettingsCore", 
                                        0xC436F916, // SceRegMgr
                                        0x16DDF3DC, 
                                        sceRegMgrGetKeyInt_SceSystemSettingsCore_patched);
    g_hooks[4] = taiHookFunctionImport(&g_sceRegMgrSetKeyInt_SceSystemSettingsCore_hook, 
                                        "SceSystemSettingsCore", 
                                        0xC436F916, // SceRegMgr
                                        0xD72EA399, 
                                        sceRegMgrSetKeyInt_SceSystemSettingsCore_patched);
    g_hooks[5] = taiHookFunctionImport(&g_sceRegMgrGetKeysInfo_SceSystemSettingsCore_hook, 
                                        "SceSystemSettingsCore", 
                                        0xC436F916, // SceRegMgr
                                        0x58421DD1, 
                                        sceRegMgrGetKeysInfo_SceSystemSettingsCore_patched);
	g_hooks[6] = taiHookFunctionImport(&g_scePafToplevelInitPluginFunctions_SceSettings_hook, 
                                        "SceSettings", 
                                        0x4D9A9DD0, // ScePafToplevel
                                        0xF5354FEF, 
                                        scePafToplevelInitPluginFunctions_SceSettings_patched);
  }
  return ret;
}

static tai_hook_ref_t g_sceKernelStopUnloadModule_SceSettings_hook;
static int sceKernelStopUnloadModule_SceSettings_patched(SceUID modid, SceSize args, void *argp, int flags, SceKernelULMOption *option, int *status) {
  if (modid == g_system_settings_core_modid) {
    g_system_settings_core_modid = -1;
    if (g_hooks[2] >= 0) taiHookRelease(g_hooks[2], g_scePafMiscLoadXmlLayout_SceSettings_hook);
    if (g_hooks[3] >= 0) taiHookRelease(g_hooks[3], g_sceRegMgrGetKeyInt_SceSystemSettingsCore_hook);
    if (g_hooks[4] >= 0) taiHookRelease(g_hooks[4], g_sceRegMgrSetKeyInt_SceSystemSettingsCore_hook);
    if (g_hooks[5] >= 0) taiHookRelease(g_hooks[5], g_sceRegMgrGetKeysInfo_SceSystemSettingsCore_hook);
	if (g_hooks[6] >= 0) taiHookRelease(g_hooks[6], g_scePafToplevelInitPluginFunctions_SceSettings_hook);
  }
  return TAI_CONTINUE(int, g_sceKernelStopUnloadModule_SceSettings_hook, modid, args, argp, flags, option, status);
}

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args) { 
  vdKUcmd(5, 0);
  memset(appi_cfg, 0, 16);
  tv = vshSblAimgrIsGenuineDolce();
  g_hooks[0] = taiHookFunctionImport(&g_sceKernelLoadStartModule_SceSettings_hook, 
                                      "SceSettings", 
                                      0xCAE9ACE6, // SceLibKernel
                                      0x2DCC4AFA, 
                                      sceKernelLoadStartModule_SceSettings_patched);
  g_hooks[1] = taiHookFunctionImport(&g_sceKernelStopUnloadModule_SceSettings_hook, 
                                      "SceSettings", 
                                      0xCAE9ACE6, // SceLibKernel
                                      0x2415F8A4, 
                                      sceKernelStopUnloadModule_SceSettings_patched);
  return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args) {
  // free hooks that didn't fail
  if (g_hooks[0] >= 0) taiHookRelease(g_hooks[0], g_sceKernelLoadStartModule_SceSettings_hook);
  if (g_hooks[1] >= 0) taiHookRelease(g_hooks[1], g_sceKernelStopUnloadModule_SceSettings_hook);
  if (g_hooks[2] >= 0) taiHookRelease(g_hooks[2], g_scePafMiscLoadXmlLayout_SceSettings_hook);
  if (g_hooks[3] >= 0) taiHookRelease(g_hooks[3], g_sceRegMgrGetKeyInt_SceSystemSettingsCore_hook);
  if (g_hooks[4] >= 0) taiHookRelease(g_hooks[4], g_sceRegMgrSetKeyInt_SceSystemSettingsCore_hook);
  if (g_hooks[5] >= 0) taiHookRelease(g_hooks[5], g_sceRegMgrGetKeysInfo_SceSystemSettingsCore_hook);
  if (g_hooks[6] >= 0) taiHookRelease(g_hooks[6], g_scePafToplevelInitPluginFunctions_SceSettings_hook);
  return SCE_KERNEL_STOP_SUCCESS;
}
