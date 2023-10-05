#include <windows.h>
#include <stdlib.h>
#include <stdio.h>

#include <limits.h>
#include <stdint.h>

#include <hidsdi.h>
#include <setupapi.h>

#include "mai2io/mai2io.h"
#include "mai2io/config.h"

static GUID hidclass_guid = {0x745a17a0, 0x74d3, 0x11d0, {0xb6, 0xfe, 0x00, 0xa0, 0xc9, 0x0f, 0x57, 0xda}};

static BOOLEAN get_device_path(char *lPath, uint16_t vid, uint16_t pid, int8_t mi)
{
    const GUID *guid = &hidclass_guid;
    HidD_GetHidGuid(&hidclass_guid);
// Get device interface info set handle
// for all devices attached to system
    HDEVINFO hDevInfo = SetupDiGetClassDevs(guid, NULL, NULL,  DIGCF_PRESENT | DIGCF_DEVICEINTERFACE); // Function class devices.
    if(hDevInfo == INVALID_HANDLE_VALUE)
        return FALSE;

// Retrieve a context structure for a device interface of a device information set.
    BYTE                             buf[1024];
    PSP_DEVICE_INTERFACE_DETAIL_DATA pspdidd = (PSP_DEVICE_INTERFACE_DETAIL_DATA)buf;
    SP_DEVICE_INTERFACE_DATA         spdid;
    SP_DEVINFO_DATA                  spdd;
    DWORD                            dwSize;
	char vidstr[64];
	char mistr[64];
	
	(void) pid; /* not used for now */
	//sprintf(vidpidstr, "vid_%04x&pid_%04x&mi_%02x", vid, pid, mi);
sprintf(vidstr, "vid_%04x&", vid);
if (mi != -1) sprintf(mistr, "&mi_%02x", mi);

	#if DEBUG == 1
printf("looking for substring %s in device path\r\n", vidstr);	
	#endif
    spdid.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

// Iterate through all the interfaces and try to match one based on
// the device number.    
    for(DWORD i = 0; SetupDiEnumDeviceInterfaces(hDevInfo, NULL,guid, i, &spdid); i++)
    {
    // Get the device path.
        dwSize = 0;
        SetupDiGetDeviceInterfaceDetail(hDevInfo, &spdid, NULL, 0, &dwSize, NULL);
        if(dwSize == 0 || dwSize > sizeof(buf))
            continue;
        
        pspdidd->cbSize = sizeof(*pspdidd);
        ZeroMemory((PVOID)&spdd, sizeof(spdd));
        spdd.cbSize = sizeof(spdd);
        if(!SetupDiGetDeviceInterfaceDetail(hDevInfo, &spdid, pspdidd,
                                            dwSize, &dwSize, &spdd))
            continue;
        
	#if DEBUG == 1
printf("checking path %s... ", pspdidd->DevicePath);	
	#endif
	
        /* check if the device contains our wanted vid/pid */
 //       if ( strstr( pspdidd->DevicePath, vidpidstr ) == NULL )
        if ( strstr( pspdidd->DevicePath, vidstr ) == NULL || ((mi!= -1) && strstr( pspdidd->DevicePath, mistr ) == NULL) )
        {
			#if DEBUG == 1
			printf("that's not it.\r\n");
			#endif
            continue;
        }
#if DEBUG == 1
        printf("\r\nDevice found at %s\r\n", pspdidd->DevicePath);
#endif
        //copy devpath into lPath
        strcpy(lPath, pspdidd->DevicePath);
        SetupDiDestroyDeviceInfoList(hDevInfo);
        return TRUE;
     }
    SetupDiDestroyDeviceInfoList(hDevInfo);
    return FALSE;
}


int hid_open_device(HANDLE *device_handle, uint16_t vid, uint16_t pid, uint8_t mi){
    static uint8_t err_count = 0;
    char path[256];
	
    if (!get_device_path(path, vid, pid, mi))
    {
#if DEBUG == 1
        printf("\r\nDevice not detected (vid %04x pid %04x mi %02x).\r\n",vid,pid,mi);
#endif
        err_count++;
        if (err_count > 2){
            printf("Could not init device after multiple attempts. Exiting.\r\n");
            exit(1);
        }
        return -1;
    }
#if DEBUG == 1
        printf("\r\nDevice found (vid %04x pid %04x mi %02x).\r\n",vid,pid,mi);
#endif
    *device_handle = CreateFile(path, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    
    if ( *device_handle == INVALID_HANDLE_VALUE )
    {
        printf("Could not open detected device (err = %lx).\r\n", GetLastError());
        return -1;
    }
    return 0;
}

int hid_get_report(HANDLE device_handle, uint8_t *buf, uint8_t report_id, uint8_t nb_bytes)
{
    DWORD          bytesRead = 0;
	static uint8_t tmp_buf[128];
	
	if (buf == NULL) return -1;
	
    tmp_buf[0] = report_id;

    ReadFile(device_handle, tmp_buf, nb_bytes*2, &bytesRead, NULL);
    // bytesRead should either be nb_bytes*2 (if it successfully read 2 reports) or nb_bytes (only one)
    if ( bytesRead != nb_bytes*2 && bytesRead != nb_bytes )
    {
#ifdef DEBUG
        printf("HID read error (expected %u (or twice that), but got %lu bytes)\n",nb_bytes, bytesRead);
#endif
        return -1;
    }

    /* HID read ok, copy latest report bytes */
    memcpy(buf, tmp_buf + bytesRead - nb_bytes, nb_bytes);
	
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

static HANDLE joy_handle;

HRESULT mai2_io_init(void)
{
    mai2_io_config_load(&mai2_io_cfg, L".\\segatools.ini");

    if (hid_open_device(&joy_handle, 0x0f0d, 0x0092, 0) != 0) {
        return S_OK;
    }

    HidD_SetNumInputBuffers(joy_handle, 2);

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

joy_report_t joy_data;

HRESULT mai2_io_poll_(void)
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

    //Player 1
    if (GetAsyncKeyState(mai2_io_cfg.vk_1p_btn[0])) {
        mai2_player1_btn |= MAI2_IO_GAMEBTN_1;
    }

    if (GetAsyncKeyState(mai2_io_cfg.vk_1p_btn[1])) {
        mai2_player1_btn |= MAI2_IO_GAMEBTN_2;
    }

    if (GetAsyncKeyState(mai2_io_cfg.vk_1p_btn[2])) {
        mai2_player1_btn |= MAI2_IO_GAMEBTN_3;
    }

    if (GetAsyncKeyState(mai2_io_cfg.vk_1p_btn[3])) {
        mai2_player1_btn |= MAI2_IO_GAMEBTN_4;
    }

    if (GetAsyncKeyState(mai2_io_cfg.vk_1p_btn[4])) {
        mai2_player1_btn |= MAI2_IO_GAMEBTN_5;
    }

    if (GetAsyncKeyState(mai2_io_cfg.vk_1p_btn[5])) {
        mai2_player1_btn |= MAI2_IO_GAMEBTN_6;
    }

    if (GetAsyncKeyState(mai2_io_cfg.vk_1p_btn[6])) {
        mai2_player1_btn |= MAI2_IO_GAMEBTN_7;
    }

    if (GetAsyncKeyState(mai2_io_cfg.vk_1p_btn[7])) {
        mai2_player1_btn |= MAI2_IO_GAMEBTN_8;
    }

    if (GetAsyncKeyState(mai2_io_cfg.vk_1p_btn[8])) {
        mai2_player1_btn |= MAI2_IO_GAMEBTN_SELECT;
    }

    //Player 2
    if (GetAsyncKeyState(mai2_io_cfg.vk_2p_btn[0])) {
        mai2_player2_btn |= MAI2_IO_GAMEBTN_1;
    }

    if (GetAsyncKeyState(mai2_io_cfg.vk_2p_btn[1])) {
        mai2_player2_btn |= MAI2_IO_GAMEBTN_2;
    }

    if (GetAsyncKeyState(mai2_io_cfg.vk_2p_btn[2])) {
        mai2_player2_btn |= MAI2_IO_GAMEBTN_3;
    }

    if (GetAsyncKeyState(mai2_io_cfg.vk_2p_btn[3])) {
        mai2_player2_btn |= MAI2_IO_GAMEBTN_4;
    }

    if (GetAsyncKeyState(mai2_io_cfg.vk_2p_btn[4])) {
        mai2_player2_btn |= MAI2_IO_GAMEBTN_5;
    }

    if (GetAsyncKeyState(mai2_io_cfg.vk_2p_btn[5])) {
        mai2_player2_btn |= MAI2_IO_GAMEBTN_6;
    }

    if (GetAsyncKeyState(mai2_io_cfg.vk_2p_btn[6])) {
        mai2_player2_btn |= MAI2_IO_GAMEBTN_7;
    }

    if (GetAsyncKeyState(mai2_io_cfg.vk_2p_btn[7])) {
        mai2_player2_btn |= MAI2_IO_GAMEBTN_8;
    }

    if (GetAsyncKeyState(mai2_io_cfg.vk_2p_btn[8])) {
        mai2_player2_btn |= MAI2_IO_GAMEBTN_SELECT;
    }

    return S_OK;
}

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

	hid_get_report(joy_handle, (uint8_t *)&joy_data, 0x01, sizeof(joy_data));
    mai2_player1_btn = joy_data.buttons;

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