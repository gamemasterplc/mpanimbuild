#pragma once
#include "AnimFormat.h"

#include "tinyxml2.h"
#include <string>
#include <vector>

#define ATB_TEX_FORMAT_RGBA8 0
#define ATB_TEX_FORMAT_RGB5A3 1
#define ATB_TEX_FORMAT_RGB5A3_DUPE 2
#define ATB_TEX_FORMAT_CI8 3
#define ATB_TEX_FORMAT_CI4 4
#define ATB_TEX_FORMAT_IA8 5
#define ATB_TEX_FORMAT_IA4 6
#define ATB_TEX_FORMAT_I8 7
#define ATB_TEX_FORMAT_I4 8
#define ATB_TEX_FORMAT_A8 9
#define ATB_TEX_FORMAT_CMPR 10
#define ATB_TEX_FORMAT_COUNT 11

struct AtbFrame {
	std::string pattern_name;
	int delay;
};

struct AtbBank {
	std::string name;
	std::vector<AtbFrame> frames;
};

struct AtbLayer {
	unsigned int alpha;
	bool flip_x;
	bool flip_y;
	std::string tex_name;
	int src_x;
	int src_y;
	int w;
	int h;
	int shift_x;
	int shift_y;
};

struct AtbPattern {
	std::string name;
	int center_x;
	int center_y;
	int w;
	int h;
	std::vector<AtbLayer> layers;
};

struct AtbTexture {
	std::string name;
	uint8_t format;
	int w;
	int h;
	uint8_t *image_data;
};

class AtbFormat : public AnimFormat
{
public:
	AtbFormat(tinyxml2::XMLDocument *document, std::string base_path);
	~AtbFormat();

public:
	virtual void WriteData(FILE *dst_file);

private:
	std::vector<AtbBank> m_bank_list;
	std::vector<AtbPattern> m_pattern_list;
	std::vector<AtbTexture> m_texture_list;
	uint32_t m_pattern_ofs;
	uint32_t m_bank_ofs;
	uint32_t m_texture_ofs;

private:
	uint32_t GetPatternSize();
	uint32_t GetBankSize();
	int32_t SearchTexture(std::string name);
	int32_t SearchPattern(std::string name);
	void WritePatterns(FILE *file);
	void WriteBanks(FILE *file);
	void WriteTextures(FILE *file);
	void ParseBanks(tinyxml2::XMLNode *node);
	void ParsePatterns(tinyxml2::XMLNode *node);
	void ParseTextures(std::string base_path, tinyxml2::XMLNode *node);
};

