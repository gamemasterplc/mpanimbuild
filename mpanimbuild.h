#pragma once

#include "tinyxml2.h"

#define TEX_FORMAT_RGBA8 0
#define TEX_FORMAT_RGB5A3 1
#define TEX_FORMAT_CI8 2
#define TEX_FORMAT_CI4 3
#define TEX_FORMAT_IA8 4
#define TEX_FORMAT_IA4 5
#define TEX_FORMAT_I8 6
#define TEX_FORMAT_I4 7
#define TEX_FORMAT_A8 8
#define TEX_FORMAT_CMPR 9
#define TEX_FORMAT_COUNT 10

void PrintError(const char *fmt, ...);
void PrintXmlError(tinyxml2::XMLError error_code);
void WriteU8(FILE *file, uint8_t value);
void WriteS8(FILE *file, int8_t value);
void WriteU16(FILE *file, uint16_t value);
void WriteS16(FILE *file, int16_t value);
void WriteU32(FILE *file, uint32_t value);
void WriteS32(FILE *file, int32_t value);
void WriteFloat(FILE *file, float value);
uint32_t GetTexDataSize(uint8_t format, int32_t w, int32_t h);
void AlignFile32(FILE *file);
//Writes Palette Immediately Before Texture if Used
void TextureWrite(FILE *file, uint8_t format, int32_t w, int32_t h, uint8_t *src);