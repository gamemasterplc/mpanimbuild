#define _CRT_SECURE_NO_WARNINGS
#include <string>
#include <stdio.h>
#include <stdarg.h>
#include "tinyxml2.h"
#include "AnimFormat.h"
#include "AnimExFormat.h"
#include "AtbFormat.h"
#include "mpanimbuild.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

void PrintError(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    exit(1);
    va_end(args);
}

void PrintXmlError(tinyxml2::XMLError error_code)
{
    if (error_code != tinyxml2::XML_SUCCESS) {
        PrintError("tinyxml2 Error %d.\n", error_code);
    }
}

void WriteU8(FILE *file, uint8_t value)
{
    fwrite(&value, 1, 1, file);
}

void WriteS8(FILE *file, int8_t value)
{
    fwrite(&value, 1, 1, file);
}

void WriteU16(FILE *file, uint16_t value)
{
    uint8_t temp[2];
    temp[0] = value >> 8;
    temp[1] = value & 0xFF;
    fwrite(temp, 2, 1, file);
}

void WriteS16(FILE *file, int16_t value)
{
    int8_t temp[2];
    temp[0] = value >> 8;
    temp[1] = value & 0xFF;
    fwrite(temp, 2, 1, file);
}

void WriteU32(FILE *file, uint32_t value)
{
    uint8_t temp[4];
    temp[0] = value >> 24;
    temp[1] = (value >> 16) & 0xFF;
    temp[2] = (value >> 8) & 0xFF;
    temp[3] = value & 0xFF;
    fwrite(temp, 4, 1, file);
}

void WriteS32(FILE *file, int32_t value)
{
    int8_t temp[4];
    temp[0] = value >> 24;
    temp[1] = (value >> 16) & 0xFF;
    temp[2] = (value >> 8) & 0xFF;
    temp[3] = value & 0xFF;
    fwrite(temp, 4, 1, file);
}

void WriteFloat(FILE *file, float value)
{
    WriteS32(file, *(int32_t *)&value);
}

void AlignFile32(FILE *file)
{
    static uint8_t data = 0x88;
    size_t ofs = ftell(file);
    while (ofs % 32) {
        fwrite(&data, 1, 1, file);
        ofs = ftell(file);
    }
}

uint32_t GetTexDataSize(uint8_t format, int32_t w, int32_t h)
{
    uint32_t block_w, block_h;
    uint32_t bpp;
    switch (format) {
        case TEX_FORMAT_RGBA8:
            bpp = 32;
            block_w = 4;
            block_h = 4;
            break;

        case TEX_FORMAT_RGB5A3:
            bpp = 16;
            block_w = 4;
            block_h = 4;
            break;

        case TEX_FORMAT_CI8:
            bpp = 8;
            block_w = 8;
            block_h = 4;
            break;

        case TEX_FORMAT_CI4:
            bpp = 4;
            block_w = 8;
            block_h = 8;
            break;

        case TEX_FORMAT_IA8:
            bpp = 16;
            block_w = 4;
            block_h = 4;
            break;

        case TEX_FORMAT_IA4:
            bpp = 8;
            block_w = 8;
            block_h = 4;
            break;

        case TEX_FORMAT_I8:
            bpp = 8;
            block_w = 8;
            block_h = 4;
            break;

        case TEX_FORMAT_I4:
            bpp = 4;
            block_w = 8;
            block_h = 8;
            break;

        case TEX_FORMAT_A8:
            bpp = 8;
            block_w = 8;
            block_h = 4;
            break;

        case TEX_FORMAT_CMPR:
            bpp = 4;
            block_w = 8;
            block_h = 8;
            break;

        default:
            PrintError("Invalid texture format %d.\n", format);
            break;
    }
    uint32_t block_pitch = (w + block_w - 1) / block_w;
    uint32_t num_y_blocks = (h + block_h - 1) / block_h;
    uint32_t block_cnt = num_y_blocks * block_pitch;
    return (block_cnt * block_w * block_h * bpp) / 8;
}

int main(int argc, char **argv)
{
    if (argc != 2 && argc != 3) {
        printf("Usage: %s: anim_xml [anim_file]", argv[0]);
        return 1;
    }
    std::string xml_path = argv[1];
    std::string xml_dir = xml_path;
    if (xml_dir.find_last_of("\\/") != std::string::npos) {
        xml_dir = xml_dir.substr(0, xml_dir.find_last_of("\\/")+1);
    } else {
        xml_dir = "";
    }
    std::string anim_file;
    if (argc == 3) {
        anim_file = argv[2];
    } else {
        anim_file = xml_path.substr(0, xml_path.find_last_of("."))+".anm";
    }
    tinyxml2::XMLDocument document;
    PrintXmlError(document.LoadFile(xml_path.c_str()));
    tinyxml2::XMLElement *root = document.FirstChild()->ToElement();
    if (root == NULL) {
        PrintXmlError(tinyxml2::XML_ERROR_FILE_READ_ERROR);
    }
    std::string type = root->Name();
    AnimFormat *format = nullptr;
    if (type == "anim") {
        format = new AtbFormat(&document, xml_dir);
    } else if (type == "animex") {
        format = new AnimExFormat(&document, xml_dir);
    } else {
        PrintError("File %s is not a valid animation XML.\n", xml_path.c_str());
    }
    FILE *file = fopen(anim_file.c_str(), "wb");
    if (!file) {
        PrintError("Failed to open %s for writing.\n", anim_file.c_str());
    }
    format->WriteData(file);
    fclose(file);
    return 0;
}