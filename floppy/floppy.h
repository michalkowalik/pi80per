//
// Created by Michal Kowalik on 10.02.24.
//

#ifndef PI80PER_FLOPPY_H
#define PI80PER_FLOPPY_H

#include "pico/stdlib.h"
#include "ff.h"


enum floppy_status {
    FLOPPY_OK = 0,
    FLOPPY_ERROR = 1,
    FLOPPY_BUSY = 2,
    FLOPPY_NO_MEDIA = 3
};

struct floppy_t {
    FIL file;
    uint8_t cylinder;
    uint8_t track;
    uint8_t sector;
    enum floppy_status status;
    char filename[32];
    uint8_t data[512];
    uint8_t error;
};

// we want 4 floppy drives
// extern floppy_t floppy_drives[4];
// extern uint8_t active_drive;

void process_floppy_command(int command, uint8_t data);
void floppy_init();
static void floppy_write_sector(uint8_t *data);
static void floppy_read_sector();

#endif //PI80PER_FLOPPY_H
