#ifndef RESOURCE_VN_JPG_DECODE_H_
#define RESOURCE_VN_JPG_DECODE_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "vn_plat.h"
#include "lvgl.h"

extern void JpegDecode_TrueColor(uint8_t * jpg_data, uint32_t jpg_size, lv_img_dsc_t * lvgl_img);
extern lv_img_dsc_t Vn_Jpg_dsc;

#endif /* RESOURCE_VN_JPG_DECODE_H_ */
