// Thanks to theflow0 for vitashell

#include <psp2/promoterutil.h>
#include <psp2/sysmodule.h>
#include <psp2/io/fcntl.h>
#include "sha1.h"
#include "head_bin.h"

#define ntohl __builtin_bswap32

#define SFO_MAGIC 0x46535000

#define PSF_TYPE_BIN 0
#define PSF_TYPE_STR 2
#define PSF_TYPE_VAL 4

typedef struct SfoHeader {
    uint32_t magic;
    uint32_t version;
    uint32_t keyofs;
    uint32_t valofs;
    uint32_t count;
} __attribute__((packed)) SfoHeader;

typedef struct SfoEntry {
    uint16_t nameofs;
    uint8_t  alignment;
    uint8_t  type;
    uint32_t valsize;
    uint32_t totalsize;
    uint32_t dataofs;
} __attribute__((packed)) SfoEntry;

int getSfoString(void* buffer, char* name, char* string, int length) {
    SfoHeader* header = (SfoHeader*)buffer;
    SfoEntry* entries = (SfoEntry*)((uint32_t)buffer + sizeof(SfoHeader));

    if (header->magic != SFO_MAGIC)
        return -1;

    int i;
    for (i = 0; i < header->count; i++) {
        if (strcmp(buffer + header->keyofs + entries[i].nameofs, name) == 0) {
            memset(string, 0, length);
            strncpy(string, buffer + header->valofs + entries[i].dataofs, length);
            string[length - 1] = '\0';
            return 0;
        }
    }

    return -2;
}

static void fpkg_hmac(const uint8_t* data, unsigned int len, uint8_t hmac[16]) {
    SHA1_CTX ctx;
    uint8_t sha1[20];
    uint8_t buf[64];

    sha1_init(&ctx);
    sha1_update(&ctx, data, len);
    sha1_final(&ctx, sha1);

    memset(buf, 0, 64);
    memcpy(&buf[0], &sha1[4], 8);
    memcpy(&buf[8], &sha1[4], 8);
    memcpy(&buf[16], &sha1[12], 4);
    buf[20] = sha1[16];
    buf[21] = sha1[1];
    buf[22] = sha1[2];
    buf[23] = sha1[3];
    memcpy(&buf[24], &buf[16], 8);

    sha1_init(&ctx);
    sha1_update(&ctx, buf, 64);
    sha1_final(&ctx, sha1);
    memcpy(hmac, sha1, 16);
}

int makeHead(const char *path) {
    char tmp_path[1088];
    uint8_t hmac[16];
    uint32_t off;
    uint32_t len;
    uint32_t out;
    
    // Read param.sfo
    sceClibSnprintf(tmp_path, sizeof(tmp_path), "%s/sce_sys/param.sfo", path);
    SceUID fd = sceIoOpen(tmp_path, SCE_O_RDONLY, 0);
    if (fd < 0)
        return fd;
    int size = sceIoLseek32(fd, 0, SCE_SEEK_END);
    sceIoLseek32(fd, 0, SCE_SEEK_SET);
    void* sfo_buffer = malloc(size);
    if (!sfo_buffer || sceIoRead(fd, sfo_buffer, size) < 0) {
        free(sfo_buffer);
        sceIoClose(fd);
        return -1;
    }
    sceIoClose(fd);

    // Get title id
    char titleid[12];
    memset(titleid, 0, sizeof(titleid));
    getSfoString(sfo_buffer, "TITLE_ID", titleid, sizeof(titleid));

    // Get content id
    char contentid[48];
    memset(contentid, 0, sizeof(contentid));
    getSfoString(sfo_buffer, "CONTENT_ID", contentid, sizeof(contentid));

    // Free sfo buffer
    free(sfo_buffer);

    // Allocate head.bin buffer
    uint8_t* head_bin = malloc(tpl_head_bin_len);
    memcpy(head_bin, tpl_head_bin, tpl_head_bin_len);

    // Write full title id
    char full_title_id[48];
    sceClibSnprintf(full_title_id, sizeof(full_title_id), "EP9000-%s_00-0000000000000000", titleid);
    strncpy((char*)&head_bin[0x30], strlen(contentid) > 0 ? contentid : full_title_id, 48);

    // hmac of pkg header
    len = ntohl(*(uint32_t*)&head_bin[0xD0]);
    fpkg_hmac(&head_bin[0], len, hmac);
    memcpy(&head_bin[len], hmac, 16);

    // hmac of pkg info
    off = ntohl(*(uint32_t*)&head_bin[0x8]);
    len = ntohl(*(uint32_t*)&head_bin[0x10]);
    out = ntohl(*(uint32_t*)&head_bin[0xD4]);
    fpkg_hmac(&head_bin[off], len - 64, hmac);
    memcpy(&head_bin[out], hmac, 16);

    // hmac of everything
    len = ntohl(*(uint32_t*)&head_bin[0xE8]);
    fpkg_hmac(&head_bin[0], len, hmac);
    memcpy(&head_bin[len], hmac, 16);

    // Make dir
    sceClibSnprintf(tmp_path, sizeof(tmp_path), "%s/sce_sys/package", path);
    sceIoMkdir(tmp_path, 0777);

    // Write head.bin
    sceClibSnprintf(tmp_path, sizeof(tmp_path), "%s/sce_sys/package/head.bin", path);
    fd = sceIoOpen(tmp_path, SCE_O_WRONLY | SCE_O_TRUNC | SCE_O_CREAT, 0777);
    if (fd < 0) {
        free(head_bin);
        return fd;
    }
    int res = sceIoWrite(fd, head_bin, tpl_head_bin_len);
    sceIoClose(fd);

    free(head_bin);

    return res;
}

int promoteApp(const char* path) {
    sceClibPrintf("promote %s\n", path);
    int res = makeHead(path);
    if (res < 0)
        return res;

    sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_PROMOTER_UTIL);

    res = scePromoterUtilityInit();
    if (res < 0)
        return res;
    
    res = scePromoterUtilityPromotePkgWithRif(path, 1);
    if(res < 0)
        return res;
    
    res = scePromoterUtilityExit();
    if (res < 0)
        return res;

    sceSysmoduleUnloadModuleInternal(SCE_SYSMODULE_INTERNAL_PROMOTER_UTIL);
    return 0;
}