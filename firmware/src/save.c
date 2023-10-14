/*
 * Controller Config Save and Load
 * WHowe <github.com/whowechina>
 * 
 * Config is stored in last sector of flash
 */

#include "save.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <memory.h>


#include "bsp/board.h"
#include "pico/bootrom.h"
#include "pico/stdio.h"

#include "hardware/flash.h"
#include "pico/multicore.h"
#include "pico/unique_id.h"

static struct {
    size_t size;
    size_t offset;
    void (*after_load)();
} modules[8] = {0};
static int module_num = 0;

static uint32_t my_magic = 0xcafecafe;

#define SAVE_TIMEOUT_US 5000000

#define SAVE_SECTOR_OFFSET (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)

typedef struct __attribute ((packed)) {
    uint32_t magic;
    uint8_t data[FLASH_PAGE_SIZE - 4];
} page_t;

static page_t old_data = {0};
static page_t new_data = {0};
static page_t default_data = {0};
static int data_page = -1;

static bool requesting_save = false;
static uint64_t requesting_time = 0;

static mutex_t *io_lock;

static void save_program()
{
    old_data = new_data;

    data_page = (data_page + 1) % (FLASH_SECTOR_SIZE / FLASH_PAGE_SIZE);
    printf("\nProgram Flash %d %8lx\n", data_page, old_data.magic);
    if (mutex_enter_timeout_us(io_lock, 100000)) {
        sleep_ms(10); /* wait for all io operations to finish */
        uint32_t ints = save_and_disable_interrupts();
        if (data_page == 0) {
            flash_range_erase(SAVE_SECTOR_OFFSET, FLASH_SECTOR_SIZE);
        }
        flash_range_program(SAVE_SECTOR_OFFSET + data_page * FLASH_PAGE_SIZE,
                            (uint8_t *)&old_data, FLASH_PAGE_SIZE);
        restore_interrupts(ints);
        mutex_exit(io_lock);
    } else {
        printf("Program Flash Failed.\n");
    }
}

static void load_default()
{
    printf("Load Default\n");
    new_data = default_data;
    new_data.magic = my_magic;
}

static const page_t *get_page(int id)
{
    int addr = XIP_BASE + SAVE_SECTOR_OFFSET;
    return (page_t *)(addr + FLASH_PAGE_SIZE * id);
}

static void save_load()
{
    for (int i = 0; i < FLASH_SECTOR_SIZE / FLASH_PAGE_SIZE; i++) {
        if (get_page(i)->magic != my_magic) {
            break;
        }
        data_page = i;
    }

    if (data_page < 0) {
        load_default();
        save_request(false);
        return;
    }

    old_data = *get_page(data_page);
    new_data = old_data;
    printf("Page Loaded %d %8lx\n", data_page, new_data.magic);
}

static void save_loaded()
{
    for (int i = 0; i < module_num; i++) {
        modules[i].after_load();
    }
}

static union __attribute__((packed)) {
    pico_unique_board_id_t id;
    struct {
        uint32_t id32h;
        uint32_t id32l;
    };
    uint64_t id64;
} board_id;

uint32_t board_id_32()
{
    pico_get_unique_board_id(&board_id.id);
    return board_id.id32h ^ board_id.id32l;
}

uint64_t board_id_64()
{
    pico_get_unique_board_id(&board_id.id);
    return board_id.id64;
}

void save_init(uint32_t magic, mutex_t *locker)
{
    my_magic = magic;
    io_lock = locker;
    save_load();
    save_loop();
    save_loaded();
}

void save_loop()
{
    if (requesting_save && (time_us_64() - requesting_time > SAVE_TIMEOUT_US)) {
        requesting_save = false;
        /* only when data is actually changed */
        if (memcmp(&old_data, &new_data, sizeof(old_data)) == 0) {
            return;
        }
        save_program();
    }
}

void *save_alloc(size_t size, void *def, void (*after_load)())
{
    modules[module_num].size = size;
    size_t offset = module_num > 0 ? modules[module_num - 1].offset + size : 0;
    modules[module_num].offset = offset;
    modules[module_num].after_load = after_load;
    module_num++;
    memcpy(default_data.data + offset, def, size); // backup the default
    return new_data.data + offset;
}

void save_request(bool immediately)
{
    if (!requesting_save) {
        printf("Save requested.\n");
        requesting_save = true;
        new_data.magic = my_magic;
        requesting_time = time_us_64();
    }
    if (immediately) {
        requesting_time = 0;
        save_loop();
    }
}
