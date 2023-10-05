#pragma once

#include <windows.h>

#include <stdint.h>

enum {
    MAI2_IO_OPBTN_TEST = 0x01,
    MAI2_IO_OPBTN_SERVICE = 0x02,
    MAI2_IO_OPBTN_COIN = 0x04,
};

enum {
    MAI2_IO_GAMEBTN_1 = 0x01,
    MAI2_IO_GAMEBTN_2 = 0x02,
    MAI2_IO_GAMEBTN_3 = 0x04,
    MAI2_IO_GAMEBTN_4 = 0x08,
    MAI2_IO_GAMEBTN_5 = 0x10,
    MAI2_IO_GAMEBTN_6 = 0x20,
    MAI2_IO_GAMEBTN_7 = 0x40,
    MAI2_IO_GAMEBTN_8 = 0x80,
    MAI2_IO_GAMEBTN_SELECT = 0x100,
};

/* Get the version of the Maimai IO API that this DLL supports. This
   function should return a positive 16-bit integer, where the high byte is
   the major version and the low byte is the minor version (as defined by the
   Semantic Versioning standard).

   The latest API version as of this writing is 0x0100. */

uint16_t mai2_io_get_api_version(void);

/* Initialize the IO DLL. This is the second function that will be called on
   your DLL, after mai2_io_get_api_version.

   All subsequent calls to this API may originate from arbitrary threads.

   Minimum API version: 0x0100 */

HRESULT mai2_io_init(void);

/* Send any queued outputs (of which there are currently none, though this may
   change in subsequent API versions) and retrieve any new inputs.

   Minimum API version: 0x0100 */

HRESULT mai2_io_poll(void);

/* Get the state of the cabinet's operator buttons as of the last poll. See
   MAI2_IO_OPBTN enum above: this contains bit mask definitions for button
   states returned in *opbtn. All buttons are active-high.

   Minimum API version: 0x0100 */

void mai2_io_get_opbtns(uint8_t *opbtn);

/* Get the state of the cabinet's gameplay buttons as of the last poll. See
   MAI2_IO_GAMEBTN enum above for bit mask definitions. Inputs are split into
   a left hand side set of inputs and a right hand side set of inputs: the bit
   mappings are the same in both cases.

   All buttons are active-high, even though some buttons' electrical signals
   on a real cabinet are active-low.

   Minimum API version: 0x0100 */

void mai2_io_get_gamebtns(uint16_t *player1, uint16_t *player2);
