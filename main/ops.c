/*
 * Copyright (C) 2021 skgleba
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

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

void net(int init) {
    if (init) {
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
    } else {
        sceHttpTerm();
        sceSysmoduleUnloadModule(SCE_SYSMODULE_HTTP);
        sceNetCtlTerm();
        sceNetTerm();
        sceSysmoduleUnloadModule(SCE_SYSMODULE_NET);
    }
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
    draw_rect(0, SCREEN_YC - PROGRESS_BAR_HEIGHT, PROGRESS_BAR_WIDTH, PROGRESS_BAR_HEIGHT, 0xFF666666);
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
        draw_rect(1, SCREEN_YC - PROGRESS_BAR_HEIGHT + 1, ((uint64_t)(PROGRESS_BAR_WIDTH - 2)) * total_read / length, PROGRESS_BAR_HEIGHT - 2, COLOR_GREEN);
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