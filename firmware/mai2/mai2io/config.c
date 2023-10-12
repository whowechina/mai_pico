#include <windows.h>

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "mai2io/config.h"

/*
Maimai DX Default key binding
1P: self-explanatory
2P: (Numpad) 8, 9, 6, 3, 2, 1, 4, 7, *
*/
static const int mai2_io_1p_default[] = {'W', 'E', 'D', 'C', 'X', 'Z', 'A', 'Q', '3'};
static const int mai2_io_2p_default[] = {0x68, 0x69, 0x66, 0x63, 0x62, 0x61, 0x64, 0x67, 0x54};

void mai2_io_config_load(
        struct mai2_io_config *cfg,
        const wchar_t *filename)
{
    wchar_t key[16];
    int i;

    assert(cfg != NULL);
    assert(filename != NULL);

    cfg->vk_test = GetPrivateProfileIntW(L"io4", L"test", '1', filename);
    cfg->vk_service = GetPrivateProfileIntW(L"io4", L"service", '2', filename);
    cfg->vk_coin = GetPrivateProfileIntW(L"io4", L"coin", '3', filename);
    cfg->swap_btn = GetPrivateProfileIntW(L"io4", L"swap", '4', filename);

    for (i = 0 ; i < 9 ; i++) {
        swprintf_s(key, _countof(key), L"1p_btn%i", i + 1);
        cfg->vk_1p_btn[i] = GetPrivateProfileIntW(
                L"button",
                key,
                mai2_io_1p_default[i],
                filename);
    }

    for (i = 0 ; i < 9 ; i++) {
        swprintf_s(key, _countof(key), L"2p_btn%i", i + 1);
        cfg->vk_2p_btn[i] = GetPrivateProfileIntW(
                L"button",
                key,
                mai2_io_2p_default[i],
                filename);
    }
}
