/*
	FatFormatProxy for PS Vita
	by SKGleba
*/

// Supported FAT types
#define F_TYPE_FAT32 0x5 // or 0x8 or 0xB
#define F_TYPE_FAT16 0x6
#define F_TYPE_EXFAT 0x7

// taihen's module_get_offset - user side
int module_get_offset(int uid, int seg, uint32_t in_off, void *out_off) {
	SceKernelModuleInfo info;
	if ((sceKernelGetModuleInfo(uid, &info) < 0) || (in_off > info.segments[seg].memsz))
		return -1;
	*(uint32_t *)out_off = (uint32_t)(info.segments[seg].vaddr + in_off);
	return 0;
}

/* 
 * patch FatFormat::devFdWrite args created by FatFormatTranslateArgs
 * 	this patch makes devFdWrite write the partition to device offset 0x0
 * 	instead of writing to blockAllign * 0x200
 * 
 * TODO: proper args RE
*/
static tai_hook_ref_t f_create_args_ref;
int f_create_args_patched(void *old_ff_args, void *ff_args, int mode) {
  int ret = TAI_CONTINUE(int, f_create_args_ref, old_ff_args, ff_args, mode);
  *(uint32_t *)(ff_args + (7 * 4)) = 0; // newFatFormatArgs->safeStartBlockAlligned
  return ret;
}

/*
 * formats [dev] to [fst]
 * set [external] to 1 if the partition should be written to master device offset 0x0
*/
static int formatBlkDev(char *dev, int fst, int external) {
	int (* FatFormatProxy)(char *devblk, int ptype, uint32_t unk2, uint32_t unk3, char *unk4, char *unk5) = NULL;
	uint32_t ffproxy_off, cargs_off;
	
	// search for modules with FatFormat & proxy embedded
	tai_module_info_t info;
	info.size = sizeof(info);
	if(taiGetModuleInfo("SceSettings", &info) < 0) {
		if(taiGetModuleInfo("SceDbRecovery", &info) < 0) {
			if(taiGetModuleInfo("SceBackupRestore", &info) < 0)
				return -1;
			else {
				cargs_off = 0x15026;
				ffproxy_off = 0xf7c4;
			}
		} else {
			cargs_off = 0x1ef5a;
			ffproxy_off = 0x17d68;
		}
		module_get_offset(info.modid, 0, ffproxy_off + 1, &FatFormatProxy);
	} else {
		if (module_get_offset(info.modid, 0, 0x15b20d, &FatFormatProxy) < 0)
			return -1;
		if (*(uint16_t *)FatFormatProxy == 0xf0e9) {
			cargs_off = 0x15e752;
			ffproxy_off = 0x15b20c;
		} else {
			cargs_off = 0x15e74e;
			ffproxy_off = 0x15b208;
			module_get_offset(info.modid, 0, ffproxy_off + 1, &FatFormatProxy);
		}
	}
	
	// prevent crashing on incompatible firmwares
	if ((FatFormatProxy == NULL) || (*(uint16_t *)FatFormatProxy != 0xf0e9))
			return -1;
	
	// external device patch
	int create_args_uid = 0;
	if (external)
		create_args_uid = taiHookFunctionOffset(&f_create_args_ref, info.modid, 0, cargs_off, 1, f_create_args_patched);
		
	// format the device
	int ret = FatFormatProxy(dev, fst, 0x8000, 0x8000, NULL, NULL);
	
	if (external)
		taiHookRelease(create_args_uid, f_create_args_ref);
	
	return ret;
}