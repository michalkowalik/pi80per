//
// Created by Michal Kowalik on 10.02.24.
//
#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "floppy.h"
#include "../intra_uart.h"

#include "f_util.h"
#include "ff.h"
#include "rtc.h"

FATFS fs;
FRESULT fs_res;
floppy_t floppy_drives[4];
uint8_t active_drive = 0;

char buffer[512];

void mount_fs() {
    fs_res = f_mount(&fs, "", 1);
    if (fs_res != FR_OK)
        panic("f_mount error: %s (%d)\n", FRESULT_str(fs_res), fs_res);

}

void floppy_init() {
    mount_fs();

    // check if there's media in the DISK0:
    DIR dir;
    fs_res = f_opendir(&dir, "DISK0");
    if (fs_res != FR_OK) panic("No media in DISK0\n");

    FILINFO file_info;
    while(f_readdir(&dir, &file_info) == FR_OK && file_info.fname[0]) {
        printf("%s\n", file_info.fname);
        if (strstr(file_info.fname, ".img")) {
            printf("Mounting %s as floppy drive\n", file_info.fname);
            char filename[32];
            snprintf(filename, 32, "DISK0/%s", file_info.fname);
            fs_res = f_open(&floppy_drives[0].file, filename, FA_OPEN_EXISTING | FA_WRITE | FA_READ);
            if (fs_res != FR_OK)
                panic("f_open error: %s (%d)\n", FRESULT_str(fs_res), fs_res);
            floppy_drives[0].track = 0;
            floppy_drives[0].sector = 0;
            floppy_drives[0].status = FLOPPY_OK;
            floppy_drives[0].data = 0;
            floppy_drives[0].error = 0;
            floppy_drives[0].cylinder = 0;
        }
    }

    // ignore the rest of the drives for now
    for (int i = 1; i < 4; i++) {
        floppy_drives[i].status = FLOPPY_NO_MEDIA;
    }
}

void floppy_write_sector(uint8_t *data) {
    if (floppy_drives[active_drive].status != FLOPPY_OK) {
        return;
    }
    // write data to the active drive at position determined by track and sector
    floppy_drives[active_drive].status = FLOPPY_BUSY;

    f_lseek(&floppy_drives[active_drive].file,
            (floppy_drives[active_drive].track - 1)* 0x1000 + floppy_drives[active_drive].sector * 0x80);

    uint bytes_written;
    fs_res = f_write(&floppy_drives[active_drive].file, data, 0x80, &bytes_written);
    if (fs_res != FR_OK) {
        floppy_drives[active_drive].status = FLOPPY_ERROR;
        return;
    }
    if (bytes_written != 0x80) {
        floppy_drives[active_drive].status = FLOPPY_ERROR;
        return;
    }

    // send confirmation back to host
    send_confirmation(0x06, 0x00);
    floppy_drives[active_drive].status = FLOPPY_OK;
}

void floppy_read_sector() {
    // read sector from active drive
    if (floppy_drives[active_drive].status != FLOPPY_OK) {
        return;
    }
    floppy_drives[active_drive].status = FLOPPY_BUSY;

    f_lseek(&floppy_drives[active_drive].file,
            (floppy_drives[active_drive].track - 1)* 0x1000 + floppy_drives[active_drive].sector * 0x80);

    fs_res = f_read(&floppy_drives[active_drive].file, buffer, 0x80, NULL);
    if (fs_res != FR_OK) {
        floppy_drives[active_drive].status = FLOPPY_ERROR;
        return;
    }

    f_sync(&floppy_drives[active_drive].file);

    // send the data back to the host
    send_data(0x07, buffer, 0x80);
    floppy_drives[active_drive].status = FLOPPY_OK;
}