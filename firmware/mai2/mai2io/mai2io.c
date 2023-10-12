#include <windows.h>
#include <stdlib.h>
#include <stdio.h>

#include <limits.h>
#include <stdint.h>

#include <hidsdi.h>
#include <setupapi.h>

#include "mai2io/mai2io.h"
#include "mai2io/config.h"

static FILE *logfile;
static GUID hidclass_guid = {0x745a17a0, 0x74d3, 0x11d0, {0xb6, 0xfe, 0x00, 0xa0, 0xc9, 0x0f, 0x57, 0xda}};

static BOOLEAN get_device_path(char *lPath, uint16_t vid, uint16_t pid, int8_t mi, const char *skip)
{
    const GUID *guid = &hidclass_guid;
    HidD_GetHidGuid(&hidclass_guid);
// Get device interface info set handle
// for all devices attached to system
    HDEVINFO hDevInfo = SetupDiGetClassDevs(guid, NULL, NULL,  DIGCF_PRESENT | DIGCF_DEVICEINTERFACE); // Function class devices.
    if (hDevInfo == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

// Retrieve a context structure for a device interface of a device information set.
    BYTE                             buf[1024];
    PSP_DEVICE_INTERFACE_DETAIL_DATA pspdidd = (PSP_DEVICE_INTERFACE_DETAIL_DATA)buf;
    SP_DEVICE_INTERFACE_DATA         spdid;
    SP_DEVINFO_DATA                  spdd;
    DWORD                            dwSize;
	char pidstr[16];
	char vidstr[16];
	char mistr[16];
	
    sprintf(pidstr, "pid_%04x&", pid);
    sprintf(vidstr, "vid_%04x&", vid);
    sprintf(mistr, "&mi_%02x", mi);

    printf("Looking up `%s` skip `%s`.\n", vidstr,
            skip ? skip : "none");
    spdid.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    for (DWORD i = 0; SetupDiEnumDeviceInterfaces(hDevInfo, NULL, guid, i, &spdid); i++) {
        dwSize = 0;
        SetupDiGetDeviceInterfaceDetail(hDevInfo, &spdid, NULL, 0, &dwSize, NULL);
        if (dwSize == 0 || dwSize > sizeof(buf)) {
            continue;
        }

        pspdidd->cbSize = sizeof(*pspdidd);
        ZeroMemory((PVOID)&spdd, sizeof(spdd));
        spdd.cbSize = sizeof(spdd);
        if (!SetupDiGetDeviceInterfaceDetail(hDevInfo, &spdid, pspdidd,
                                             dwSize, &dwSize, &spdd)) {
            continue;
        }
        if (strstr(pspdidd->DevicePath, vidstr) == NULL ||
            ((mi != -1) && strstr(pspdidd->DevicePath, mistr) == NULL)) {
            continue;
        }
        if (skip != NULL && strcmp(pspdidd->DevicePath, skip) == 0) {
            fprintf(logfile, "DevicePath already used: %s\n", skip);
            continue;
        }

        fprintf(logfile, "Found Device: %s\n", pspdidd->DevicePath);
        strcpy(lPath, pspdidd->DevicePath);
        SetupDiDestroyDeviceInfoList(hDevInfo);
        return TRUE;
     }
    SetupDiDestroyDeviceInfoList(hDevInfo);
    return FALSE;
}

HANDLE hid_open_device(uint16_t vid, uint16_t pid, uint8_t mi, bool first)
{
    static char path1[256];
    static char path2[256];

    char *path = first ? path1 : path2;
    const char *skip = first ? NULL : path1;

    if (!get_device_path(path, vid, pid, mi, skip)) {
        return INVALID_HANDLE_VALUE;
    }

    fprintf(logfile, "Opening Device: %s\n", path);
    fflush(logfile);
    return CreateFile(path, GENERIC_READ|GENERIC_WRITE,
                            FILE_SHARE_READ|FILE_SHARE_WRITE,
                            NULL, OPEN_EXISTING, 0, NULL);
}

int hid_get_report(HANDLE handle, uint8_t *buf, uint8_t report_id, uint8_t nb_bytes)
{
    DWORD bytesRead = 0;
	uint8_t tmp_buf[128];
	
	if (buf == NULL) {
        return -1;
    }
	
    tmp_buf[0] = report_id;

    ReadFile(handle, tmp_buf, nb_bytes, &bytesRead, NULL);
    // bytesRead should either be nb_bytes*2 (if it successfully read 2 reports) or nb_bytes (only one)
    if (bytesRead != nb_bytes) {
        return -1;
    }

    /* HID read ok, copy latest report bytes */
    memcpy(buf, tmp_buf, nb_bytes);
	
    return 0;	
}

static uint8_t mai2_opbtn;
static uint16_t mai2_player1_btn;
static uint16_t mai2_player2_btn;
static struct mai2_io_config mai2_io_cfg;
static bool mai2_io_coin;

uint16_t mai2_io_get_api_version(void)
{
    return 0x0100;
}

static HANDLE joy1_handle;
static HANDLE joy2_handle;

HRESULT mai2_io_init(void)
{
    mai2_io_config_load(&mai2_io_cfg, L".\\segatools.ini");
    logfile = fopen(".\\mai2_log.txt", "w+");

    joy1_handle = hid_open_device(0x0f0d, 0x0092, 0, true);
    fprintf(logfile, "Handle1: %Id\n", (intptr_t)joy1_handle);
    fflush(logfile);
    joy2_handle = hid_open_device(0x0f0d, 0x0092, 0, false);
    fprintf(logfile, "Handle2: %Id\n", (intptr_t)joy2_handle);
    fflush(logfile);

    if (joy1_handle != INVALID_HANDLE_VALUE) {
        fprintf(logfile, "Joy1 OK\n");
        HidD_SetNumInputBuffers(joy1_handle, 2);
    }

    if (joy2_handle != INVALID_HANDLE_VALUE) {
        fprintf(logfile, "Joy2 OK\n");
        HidD_SetNumInputBuffers(joy2_handle, 2);
    }

    fprintf(logfile, "MAI2 JOY Init Done\n");

    fflush(logfile);
    fclose(logfile);
    return S_OK;
}

#pragma pack(1)
typedef struct joy_report_s {
	uint8_t  report_id;
	uint16_t buttons; // 16 buttons; see JoystickButtons_t for bit mapping
	uint8_t  HAT;    // HAT switch; one nibble w/ unused nibble
	uint32_t axis;
	uint8_t  VendorSpec;
} joy_report_t;

HRESULT mai2_io_poll(void)
{
    mai2_opbtn = 0;
    mai2_player1_btn = 0;
    mai2_player2_btn = 0;

    if (GetAsyncKeyState(mai2_io_cfg.vk_test) & 0x8000) {
        mai2_opbtn |= MAI2_IO_OPBTN_TEST;
    }

    if (GetAsyncKeyState(mai2_io_cfg.vk_service) & 0x8000) {
        mai2_opbtn |= MAI2_IO_OPBTN_SERVICE;
    }

    if (GetAsyncKeyState(mai2_io_cfg.vk_coin) & 0x8000) {
        if (!mai2_io_coin) {
            mai2_io_coin = true;
            mai2_opbtn |= MAI2_IO_OPBTN_COIN;
        }
    } else {
        mai2_io_coin = false;
    }

    if (joy1_handle != INVALID_HANDLE_VALUE) {
        joy_report_t joy_data;
    	hid_get_report(joy1_handle, (uint8_t *)&joy_data, 0x01, sizeof(joy_data));
        mai2_player1_btn = joy_data.buttons;
    }

    if (joy2_handle != INVALID_HANDLE_VALUE) {
        joy_report_t joy_data;
        hid_get_report(joy2_handle, (uint8_t *)&joy_data, 0x01, sizeof(joy_data));
        mai2_player2_btn = joy_data.buttons;
    }

    if (mai2_io_cfg.swap_btn) {
        uint16_t tmp = mai2_player1_btn;
        mai2_player1_btn = mai2_player2_btn;
        mai2_player2_btn = tmp;
    }
    return S_OK;
}

void mai2_io_get_opbtns(uint8_t *opbtn)
{
    if (opbtn != NULL) {
        *opbtn = mai2_opbtn;
    }
}

void mai2_io_get_gamebtns(uint16_t *player1, uint16_t *player2)
{
    if (player1 != NULL) {
        *player1 = mai2_player1_btn;
    }

    if (player2 != NULL ){
        *player2 = mai2_player2_btn;
    }
}