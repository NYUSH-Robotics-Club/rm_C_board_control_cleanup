/* Retain the upstream VTM dependency without colliding with the vision CRC API. */
#define crc_16 Nyush_crc_16
#define crc_modbus Nyush_crc_modbus
#define update_crc_16 Nyush_update_crc_16
#define init_crc16_tab Nyush_init_crc16_tab
#include "../../third_party/nyush_remote/modules/algorithm/crc16.c"
