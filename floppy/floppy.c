//
// Created by Michal Kowalik on 10.02.24.
//
#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include "pico/stdlib.h"
#include "floppy.h"
#include "../intra_uart.h"

#include "f_util.h"
#include "ff.h"
#include "sd_card.h"
#include "../config.h"
#include "hw_config.h"

static FATFS fs;
FRESULT fs_res;
struct floppy_t floppy_drives[4];
static uint8_t active_drive = 0;
char const *sd_card_prefix;
char buffer[512];

void process_floppy_command(int command, uint8_t data) {
    uint8_t uart_buffer[256];
    uint8_t index = 0;
    uint8_t length;

    switch (command) {
        case 0x02:
            // set active Drive
            printf("Command: 0x02, drive: %02x \r\n", data);
            active_drive = data;
            send_confirmation(0x02, 0x00);
            break;
        case 0x03:
            // set active sector on active drive
            printf("Command: 0x03, sector: %02x \r\n", data);
            if (data > 0 && data < 32) {
                floppy_drives[active_drive].sector = data;
                send_confirmation(0x03, 0x00);
            } else {
                send_confirmation(0x03, 0x01);
            }
            break;
        case 0x04:
            // set active track
            printf("Command: 0x04, track: %02x \r\n", data);
            if (data >= 0 && data <= 31) {
                floppy_drives[active_drive].track = data;
                send_confirmation(0x04, 0);
            } else {
                send_confirmation(0x04, 1);
            }
            break;
        case 0x06:
            // write data to active drive
            length = data;
            printf("Command: 0x06, length: %02x \r\n", length);
            while (index < length) {
                uart_buffer[index++] = uart_getc(UART_INTRA);
            }
            floppy_write_sector(uart_buffer);
            break;
        case 0x07:
            // read sector from active drive
            length = data;
            printf("Command: 0x07, length: %02x \r\n", length);
            floppy_read_sector();
            break;
        default:
            printf("Unknown command: 0x%02x\r\n", command);
    }
}


void mount_fs() {
    sd_card_t *sd_card_p = sd_get_by_num(0);
    assert(sd_card_p);

    sd_card_prefix = sd_get_drive_prefix(sd_card_p);
    printf("Mounting %s\n", sd_card_prefix);
    fs = sd_card_p->state.fatfs;

    fs_res = f_mount(&fs, sd_card_prefix, 1);
    if (fs_res != FR_OK)
        panic("f_mount error: %s (%d)\n", FRESULT_str(fs_res), fs_res);
    sd_card_p->state.mounted = true;
}

void init_drive(int drive) {
    char drive_name[8];
    snprintf(drive_name, 8, "DISK%d", drive);

    DIR dir;
    fs_res = f_opendir(&dir, drive_name);
    if (fs_res != FR_OK) {
        printf("No media in %s\n", drive_name);
        return;
    }

    FILINFO file_info;
    while(f_readdir(&dir, &file_info) == FR_OK && file_info.fname[0]) {
        if (strstr(file_info.fname, ".img")) {
            printf("Mounting %s as floppy drive %d \n", file_info.fname, drive);
            char filename[32];
            snprintf(filename, 32, "0:/%s/%s", drive_name, file_info.fname);
            //printf("Opening %s\n", filename);
            //fs_res = f_open(&floppy_drives[drive].file, filename, FA_OPEN_EXISTING | FA_WRITE | FA_READ);
            //if (fs_res != FR_OK)
            //    panic("f_open error: %s (%d)\n", FRESULT_str(fs_res), fs_res);
            floppy_drives[drive].track = 0;
            floppy_drives[drive].sector = 1;
            strncpy(floppy_drives[drive].filename, filename, sizeof filename);
            //fs_res = f_close(&floppy_drives[drive].file);
            //if (fs_res != FR_OK)
            //    panic("f_close error: %s (%d)\n", FRESULT_str(fs_res), fs_res);
            floppy_drives[drive].status = FLOPPY_OK;
/*
            // do some light reading
            printf("Reading first sector from drive %d..\n", drive);
            fs_res = f_lseek(&floppy_drives[drive].file, 0);
            if (fs_res != FR_OK) {
                printf("f_lseek error: %s (%d)\n", FRESULT_str(fs_res), fs_res);
            }
            fs_res = f_read(&floppy_drives[drive].file, buffer, 512, NULL);
            if (fs_res != FR_OK) {
                printf("f_read error: %s (%d)\n", FRESULT_str(fs_res), fs_res);
            }

            for (int i = 0; i<512; i++) {
                floppy_drives[drive].data[i] = buffer[i];
            }
            fs_res = f_close(&floppy_drives[drive].file);
            if (fs_res != FR_OK) {
                printf("f_close error: %s (%d)\n", FRESULT_str(fs_res), fs_res);
            }
*/

        } else {
            printf("Ignoring %s\n", file_info.fname);
            floppy_drives[drive].status = FLOPPY_NO_MEDIA;
        }
    }
    fs_res = f_closedir(&dir);
    if (fs_res != FR_OK)
        panic("f_closedir error: %s (%d)\n", FRESULT_str(fs_res), fs_res);
    printf("\n");
}

void floppy_init() {
    sd_init_driver();
    mount_fs();

    for (int i = 0; i<4; i++) {
        init_drive(i);
    }
}

static void floppy_write_sector(uint8_t *data) {
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

static void floppy_read_sector() {
    FIL fil;
    sd_card_t *sd_card = sd_get_by_drive_prefix("0:");
    if(!sd_card->state.mounted) {
        printf("SD card not mounted\n");
        return;
    }
    printf("opening file: %s\n", floppy_drives[active_drive].filename);
    fs_res = f_open(&fil, "/d0.img", FA_OPEN_EXISTING | FA_READ);
    if (fs_res != FR_OK) {
        printf("f_open error: %s (%d)\n", FRESULT_str(fs_res), fs_res);
        floppy_drives[active_drive].status = FLOPPY_ERROR;
        return;
    }
    // read sector from active drive
    if (floppy_drives[active_drive].status != FLOPPY_OK) {
        printf("Floppy %d not ready\n", active_drive);
        return;
    }
    floppy_drives[active_drive].status = FLOPPY_BUSY;
    printf("Reading sector %02x from track %02x\n", floppy_drives[active_drive].sector, floppy_drives[active_drive].track);
    fs_res = f_lseek(&floppy_drives[active_drive].file,
                     floppy_drives[active_drive].track * 0x1000 + (floppy_drives[active_drive].sector - 1) * 0x80);
    if (fs_res != FR_OK) {
        printf("f_lseek error: %s (%d)\n", FRESULT_str(fs_res), fs_res);
        floppy_drives[active_drive].status = FLOPPY_ERROR;
        return;
    }

    uint8_t *file_buffer = malloc(0x80);

    uint bytes_read;
    fs_res = f_read(&floppy_drives[active_drive].file, file_buffer, 0x80, &bytes_read);
    if (fs_res != FR_OK) {
        printf("f_read error: %s (%d)\n", FRESULT_str(fs_res), fs_res);
        floppy_drives[active_drive].status = FLOPPY_ERROR;
        return;
    }

    if (bytes_read != 0x80) {
        printf("f_read error: read %d bytes, expected 128\n", bytes_read);
        floppy_drives[active_drive].status = FLOPPY_ERROR;
        return;
    }
    f_sync(&floppy_drives[active_drive].file);

    // send the data back to the host
    send_data(0x07, file_buffer, 0x80);
    floppy_drives[active_drive].status = FLOPPY_OK;
    free(file_buffer);
    f_close(&fil);
}
