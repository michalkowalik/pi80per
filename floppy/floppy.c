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
#include "../debug.h"
#include "hw_config.h"

#define QUEUE_SIZE 32

int floppy_read_queue[QUEUE_SIZE];
uint floppy_queue_head = 0;
uint floppy_queue_tail = 0;

static FATFS fs;
FRESULT fs_res;
struct floppy_t floppy_drives[4];
static uint8_t active_drive = 0;
char const *sd_card_prefix;
uint const sector_length = 0x80;
uint8_t data_buffer[256];

void enqueue_floppy_request(uint8_t command) {
    if ((floppy_queue_tail + 1) % QUEUE_SIZE == floppy_queue_head) {
        printf("Floppy queue full, dropping read_request:\r\n");
        return;
    }
    floppy_read_queue[floppy_queue_tail] = command;
    floppy_queue_tail = (floppy_queue_tail + 1) % QUEUE_SIZE;
}

int dequeue_floppy_request() {
    if (floppy_queue_head == floppy_queue_tail) {
        return -1;
    }
    int data = floppy_read_queue[floppy_queue_head];
    floppy_queue_head = (floppy_queue_head + 1) % QUEUE_SIZE;
    return data;
}

void process_floppy_write(const uint8_t *data) {
    for (int i = 0; i < sector_length; i++) {
        data_buffer[i] = data[i];
    }
    enqueue_floppy_request(0x06);
}

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
        //case 0x06:
        //    // write data to active drive
        //    enqueue_floppy_request(0x06);
        //    break;
        case 0x07:
            // read sector from active drive
            enqueue_floppy_request(0x07);
            break;
        default:
            printf("Unknown command: 0x%02x\r\n", command);
    }
}

void check_floppy_queue() {
    uint index = 0;
    uint8_t *buffer = malloc(0x80);
    if (floppy_queue_head != floppy_queue_tail) {
        int command = dequeue_floppy_request();
        debug_printf("Command: 0x%02x, length: 0x80 \r\n", command);

        // read sector
        if (command == 0x07) {
            floppy_read_sector();
        }
        // write sector
        else if (command == 0x06) {
            // read data from the host
            floppy_write_sector(data_buffer);
        } else {
            debug_printf("Unknown command: %02x\r\n", command);
        }
    }
    free(buffer);
}

void mount_fs() {
    sd_card_t *sd_card = sd_get_by_num(0);
    assert(sd_card);
    sd_card_prefix = sd_get_drive_prefix(sd_card);

    if (!sd_card->state.mounted) {
        debug_printf("SD card not mounted\n");

        fs_res = f_mount(&fs, sd_card_prefix, 1); // mount the file system with re-mounting enabled
        if (fs_res != FR_OK) {
            debug_printf("f_mount error: %s (%d)\n", FRESULT_str(fs_res), fs_res);
            return;
        } else {
            debug_printf("f_mount OK\n");
            sd_card->state.mounted = true;
        }
    } else {
        debug_printf("SD card mounted\n");
    }
}

void init_drive(int drive) {
    char drive_name[8];
    snprintf(drive_name, 8, "DISK%d", drive);
    printf("\n %s: ", drive_name);

    DIR dir;
    fs_res = f_opendir(&dir, drive_name);
    if (fs_res != FR_OK) {
        printf("No media in %s\n", drive_name);
        return;
    }

    FILINFO file_info;
    while (f_readdir(&dir, &file_info) == FR_OK && file_info.fname[0]) {
        if (strstr(file_info.fname, ".img")) {
            printf("Mounting %s as floppy drive %d ", file_info.fname, drive);
            char filename[32];
            snprintf(filename, 32, "0:/%s/%s", drive_name, file_info.fname);
            floppy_drives[drive].track = 0;
            floppy_drives[drive].sector = 1;
            strncpy(floppy_drives[drive].filename, filename, sizeof filename);
            floppy_drives[drive].status = FLOPPY_OK;
            break;
        }
    }
    fs_res = f_closedir(&dir);
    if (fs_res != FR_OK)
        panic("f_closedir error: %s (%d)\n", FRESULT_str(fs_res), fs_res);
}

void floppy_init() {
    sd_init_driver();
    mount_fs();

    for (int i = 0; i < 4; i++) {
        init_drive(i);
    }
    printf("\n--\n");
}

static void floppy_write_sector(uint8_t *data) {
    gpio_put(LED_PIN, 1);
    FIL fil;

    for (int i = 0; i < 0x80; i++) {
        if (i % 16 == 0) {
            debug_printf("\n");
        }
        debug_printf("%02x ", data[i]);
    }

    mount_fs();

    debug_printf("opening file: %s\n", floppy_drives[active_drive].filename);
    fs_res = f_open(&fil, floppy_drives[active_drive].filename, FA_WRITE);
    if (fs_res != FR_OK) {
        debug_printf("f_open error: %s (%d)\n", FRESULT_str(fs_res), fs_res);
        floppy_drives[active_drive].status = FLOPPY_ERROR;
        return;
    }
    if (floppy_drives[active_drive].status != FLOPPY_OK) {
        return;
    }

    // write data to the active drive at position determined by track and sector
    floppy_drives[active_drive].status = FLOPPY_BUSY;
    fs_res = f_lseek(&fil,
            (floppy_drives[active_drive].track - 1) * 0x1000 + floppy_drives[active_drive].sector * 0x80);
    if (fs_res != FR_OK) {
        debug_printf("f_lseek error: %s (%d)\n", FRESULT_str(fs_res), fs_res);
        floppy_drives[active_drive].status = FLOPPY_ERROR;
        return;
    }
    debug_printf("Writing sector %02x, track %02x\n",
                 floppy_drives[active_drive].sector, floppy_drives[active_drive].track);

    uint bytes_written;
    fs_res = f_write(&fil, data, 0x80, &bytes_written);
    if (fs_res != FR_OK) {
        debug_printf("f_write error: %s (%d)\n", FRESULT_str(fs_res), fs_res);
        floppy_drives[active_drive].status = FLOPPY_ERROR;
        return;
    }
    if (bytes_written != 0x80) {
        debug_printf("f_write error: wrote %d bytes, expected 128\n", bytes_written);
        floppy_drives[active_drive].status = FLOPPY_ERROR;
        return;
    }
    f_sync(&fil);

    debug_printf("Wrote %d bytes to %s\n", bytes_written, floppy_drives[active_drive].filename);

    // send confirmation back to host
    send_confirmation(0x06, 0x00);
    floppy_drives[active_drive].status = FLOPPY_OK;
    f_close(&fil);
    gpio_put(LED_PIN, 0);
}

static void floppy_read_sector() {
    gpio_put(LED_PIN, 1);
    FIL fil;

    mount_fs();

    debug_printf("opening file: %s\n", floppy_drives[active_drive].filename);
    fs_res = f_open(&fil, floppy_drives[active_drive].filename, FA_OPEN_EXISTING | FA_READ);
    if (fs_res != FR_OK) {
        debug_printf("f_open error: %s (%d)\n", FRESULT_str(fs_res), fs_res);
        floppy_drives[active_drive].status = FLOPPY_ERROR;
        return;
    }
    // read sector from active drive
    if (floppy_drives[active_drive].status != FLOPPY_OK) {
        debug_printf("Floppy %d not ready\n", active_drive);
        return;
    }
    floppy_drives[active_drive].status = FLOPPY_BUSY;
    debug_printf("Reading sector %02x from track %02x\n", floppy_drives[active_drive].sector,
                 floppy_drives[active_drive].track);
    fs_res = f_lseek(&fil,
                     floppy_drives[active_drive].track * 0x1000 + (floppy_drives[active_drive].sector - 1) * 0x80);
    if (fs_res != FR_OK) {
        debug_printf("f_lseek error: %s (%d)\n", FRESULT_str(fs_res), fs_res);
        floppy_drives[active_drive].status = FLOPPY_ERROR;
        return;
    }

    uint8_t *file_buffer = malloc(0x80);

    uint bytes_read;
    fs_res = f_read(&fil, file_buffer, 0x80, &bytes_read);
    if (fs_res != FR_OK) {
        debug_printf("f_read error: %s (%d)\n", FRESULT_str(fs_res), fs_res);
        floppy_drives[active_drive].status = FLOPPY_ERROR;
        return;
    }

    if (bytes_read != 0x80) {
        debug_printf("f_read error: read %d bytes, expected 128\n", bytes_read);
        floppy_drives[active_drive].status = FLOPPY_ERROR;
        return;
    }
    f_sync(&fil);

    // send the data back to the host
    send_data(0x07, file_buffer, 0x80);
    floppy_drives[active_drive].status = FLOPPY_OK;
    free(file_buffer);
    f_close(&fil);
    gpio_put(LED_PIN, 0);
}
