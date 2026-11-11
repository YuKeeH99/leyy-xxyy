#include "vn_jpg_decode.h"

#include <stdlib.h>
#include <string.h>

#include "vn_log.h"
#include "../third_party/lvgl/src/extra/libs/sjpg/tjpgd.h"

#define VN_JPG_WORKSPACE_SIZE      4096U
#define VN_JPG_OUTPUT_BUFFER_SIZE  (100U * 1024U)

typedef struct
{
    uint8_t *jpg_data;
    uint32_t jpg_size;
    uint32_t current_pos;
    uint8_t *rgb_buffer;
    uint32_t width;
    uint32_t height;
    uint32_t decoding_complete;
} JpegImage_Data_TypeDef;

lv_img_dsc_t Vn_Jpg_dsc;

static JDEC jdec;
static uint8_t TJPGD_Workspace[VN_JPG_WORKSPACE_SIZE];
static uint8_t jpg_rgb_buffer[VN_JPG_OUTPUT_BUFFER_SIZE];

static int JpegOutput_Callback(JDEC *jd, void *bitmap, JRECT *rect)
{
    JpegImage_Data_TypeDef *img_data = (JpegImage_Data_TypeDef *)jd->device;
    const uint8_t *src = (const uint8_t *)bitmap;
    uint32_t rect_width = (uint32_t)rect->right - rect->left + 1U;
    uint32_t rect_height = (uint32_t)rect->bottom - rect->top + 1U;
    uint32_t y;

    for(y = 0; y < rect_height; ++y)
    {
        uint32_t x;
        uint8_t *dst = img_data->rgb_buffer +
                       (((uint32_t)rect->top + y) * img_data->width + rect->left) * 3U;

        for(x = 0; x < rect_width; ++x)
        {
            dst[0] = (uint8_t)((src[2] & 0xF8U) | (src[1] >> 5));
            dst[1] = (uint8_t)(((src[1] << 3) & 0xE0U) | (src[0] >> 3));
            dst[2] = LV_OPA_COVER;
            src += 3;
            dst += 3;
        }
    }

    return 1;
}

static size_t JpegInput_Callback(JDEC *jd, uint8_t *buff, size_t ndata)
{
    JpegImage_Data_TypeDef *img_data = (JpegImage_Data_TypeDef *)jd->device;
    uint32_t remaining = 0;
    size_t read_size = ndata;

    if(img_data->current_pos < img_data->jpg_size)
    {
        remaining = img_data->jpg_size - img_data->current_pos;
    }
    if(read_size > remaining)
    {
        read_size = remaining;
    }

    if(buff != NULL && read_size > 0U)
    {
        memcpy(buff, img_data->jpg_data + img_data->current_pos, read_size);
    }
    img_data->current_pos += (uint32_t)read_size;

    return read_size;
}

void JpegDecode_TrueColor(uint8_t *jpg_data, uint32_t jpg_size, lv_img_dsc_t *lvgl_img)
{
    JpegImage_Data_TypeDef img_data;
    JRESULT result;
    uint8_t *rgb_buffer;
    size_t rgb_buffer_size;

    if(lvgl_img == NULL)
    {
        return;
    }
    memset(lvgl_img, 0, sizeof(*lvgl_img));

    if(jpg_data == NULL || jpg_size == 0U)
    {
        VN_LOG_E("invalid JPEG buffer");
        return;
    }

    memset(&img_data, 0, sizeof(img_data));
    img_data.jpg_data = jpg_data;
    img_data.jpg_size = jpg_size;

    result = jd_prepare(&jdec, JpegInput_Callback, TJPGD_Workspace,
                        VN_JPG_WORKSPACE_SIZE, &img_data);
    if(result != JDR_OK)
    {
        VN_LOG_E("jd_prepare failed: %d", result);
        return;
    }

    rgb_buffer_size = (size_t)jdec.width * (size_t)jdec.height * 3U;
    if(jdec.width == 0U || jdec.height == 0U ||
       rgb_buffer_size > VN_JPG_OUTPUT_BUFFER_SIZE || rgb_buffer_size > UINT32_MAX)
    {
        VN_LOG_E("unsupported JPEG size: %ux%u",
                 (unsigned int)jdec.width, (unsigned int)jdec.height);
        return;
    }

    rgb_buffer = (uint8_t *)malloc(rgb_buffer_size);
    if(rgb_buffer == NULL)
    {
        VN_LOG_E("JPEG RGB buffer allocation failed");
        return;
    }

    img_data.rgb_buffer = rgb_buffer;
    img_data.width = jdec.width;
    img_data.height = jdec.height;

    result = jd_decomp(&jdec, JpegOutput_Callback, 0);
    if(result != JDR_OK)
    {
        VN_LOG_E("jd_decomp failed: %d", result);
        free(rgb_buffer);
        return;
    }

    memcpy(jpg_rgb_buffer, rgb_buffer, rgb_buffer_size);
    free(rgb_buffer);

    img_data.decoding_complete = 1U;
    lvgl_img->header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA;
    lvgl_img->header.w = jdec.width;
    lvgl_img->header.h = jdec.height;
    lvgl_img->data_size = (uint32_t)rgb_buffer_size;
    lvgl_img->data = jpg_rgb_buffer;
}
