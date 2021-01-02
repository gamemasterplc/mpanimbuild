#include <algorithm>
#include <cctype>
#include "AtbFormat.h"
#include "mpanimbuild.h"
#include "stb_image.h"

void AtbFormat::ParseBanks(tinyxml2::XMLNode *node)
{
	tinyxml2::XMLElement *bank_node = node->FirstChildElement("bank");
	while (bank_node) {
		AtbBank bank;
		const char *str_temp;
		PrintXmlError(bank_node->QueryAttribute("name", &str_temp));
		bank.name = str_temp;
		tinyxml2::XMLElement *frame_node = bank_node->FirstChildElement("frame");
		while (frame_node) {
			AtbFrame frame;
			PrintXmlError(frame_node->QueryAttribute("pattern", &str_temp));
			frame.pattern_name = str_temp;
			frame.delay = 6;
			frame_node->QueryIntAttribute("delay", &frame.delay);
			bank.frames.push_back(frame);
			frame_node = frame_node->NextSiblingElement("frame");
		}
		m_bank_list.push_back(bank);
		bank_node = bank_node->NextSiblingElement("bank");
	}
}

void AtbFormat::ParsePatterns(tinyxml2::XMLNode *node)
{
	tinyxml2::XMLElement *pattern_node = node->FirstChildElement("pattern");
	while (pattern_node) {
		AtbPattern pattern;
		const char *str_temp;
		PrintXmlError(pattern_node->QueryAttribute("name", &str_temp));
		pattern.name = str_temp;
		PrintXmlError(pattern_node->QueryIntAttribute("center_x", &pattern.center_x));
		PrintXmlError(pattern_node->QueryIntAttribute("center_y", &pattern.center_y));
		PrintXmlError(pattern_node->QueryIntAttribute("w", &pattern.w));
		PrintXmlError(pattern_node->QueryIntAttribute("h", &pattern.h));
		tinyxml2::XMLElement *layer_node = pattern_node->FirstChildElement("layer");
		while (layer_node) {
			AtbLayer layer;
			layer.alpha = 255;
			layer.flip_x = layer.flip_y = false;
			layer_node->QueryUnsignedAttribute("alpha", &layer.alpha);
			layer_node->QueryBoolAttribute("flip_x", &layer.flip_x);
			layer_node->QueryBoolAttribute("flip_y", &layer.flip_y);
			PrintXmlError(layer_node->QueryAttribute("tex_name", &str_temp));
			layer.tex_name = str_temp;
			layer.src_x = layer.src_y = 0;
			layer_node->QueryIntAttribute("src_x", &layer.src_x);
			layer_node->QueryIntAttribute("src_y", &layer.src_y);
			PrintXmlError(layer_node->QueryIntAttribute("w", &layer.w));
			PrintXmlError(layer_node->QueryIntAttribute("h", &layer.h));
			layer_node->QueryIntAttribute("shift_x", &layer.shift_x);
			layer_node->QueryIntAttribute("shift_y", &layer.shift_y);
			pattern.layers.push_back(layer);
			layer_node = layer_node->NextSiblingElement("layer");
		}
		m_pattern_list.push_back(pattern);
		pattern_node = pattern_node->NextSiblingElement("pattern");
	}
}

uint8_t GetTextureFormat(std::string format)
{
	std::string format_list[10] = { "RGBA8", "RGB5A3", "CI8", "CI4", "IA8", "IA4", "I8", "I4", "A8", "CMPR" };
	uint8_t format_ids[10] = { ATB_TEX_FORMAT_RGBA8, ATB_TEX_FORMAT_RGB5A3, ATB_TEX_FORMAT_CI8, ATB_TEX_FORMAT_CI4, 
		ATB_TEX_FORMAT_IA8, ATB_TEX_FORMAT_IA4, ATB_TEX_FORMAT_I8, ATB_TEX_FORMAT_I4, ATB_TEX_FORMAT_A8, ATB_TEX_FORMAT_CMPR };
	std::transform(format.begin(), format.end(), format.begin(), ::toupper);
	for (uint32_t i = 0; i < 10; i++) {
		if (format_list[i] == format) {
			return format_ids[i];
		}
	}
	return ATB_TEX_FORMAT_RGBA8;
}

void AtbFormat::ParseTextures(std::string base_path, tinyxml2::XMLNode *node)
{
	tinyxml2::XMLElement *texture_node = node->FirstChildElement("texture");
	while (texture_node) {
		AtbTexture texture;
		const char *str_temp;
		PrintXmlError(texture_node->QueryAttribute("name", &str_temp));
		texture.name = str_temp;
		str_temp = "RGBA8";
		texture_node->QueryAttribute("format", &str_temp);
		texture.format = GetTextureFormat(str_temp);
		PrintXmlError(texture_node->QueryAttribute("file", &str_temp));
		std::string file_rel_path = str_temp;
		std::string file_path = base_path + file_rel_path;
		int channels;
		texture.image_data = stbi_load(file_path.c_str(), &texture.w, &texture.h, &channels, 4);
		m_texture_list.push_back(texture);
		texture_node = texture_node->NextSiblingElement("texture");
	}
}

AtbFormat::AtbFormat(tinyxml2::XMLDocument *document, std::string base_path)
{
	tinyxml2::XMLNode *root = document->FirstChild();
	tinyxml2::XMLNode *bank = root->FirstChildElement("banks");
	if (!bank) {
		PrintError("Failed to find a banks node.\n");
	}
	ParseBanks(bank);
	tinyxml2::XMLNode *pattern = root->FirstChildElement("patterns");
	if (!pattern) {
		PrintError("Failed to find a patterns node.\n");
	}
	ParsePatterns(pattern);
	tinyxml2::XMLNode *texture = root->FirstChildElement("textures");
	if (!texture) {
		PrintError("Failed to find a textures node.\n");
	}
	ParseTextures(base_path, texture);
}

AtbFormat::~AtbFormat()
{
	for (size_t i = 0; i < m_texture_list.size(); i++) {
		stbi_image_free(m_texture_list[i].image_data);
	}
}

uint32_t AtbFormat::GetPatternSize()
{
	uint32_t pattern_size = m_pattern_list.size() * 16;
	for (uint32_t i = 0; i < m_pattern_list.size(); i++) {
		pattern_size += 32 * m_pattern_list[i].layers.size();
	}
	return pattern_size;
}

uint32_t AtbFormat::GetBankSize()
{
	uint32_t bank_size = m_bank_list.size() * 8;
	for (uint32_t i = 0; i < m_bank_list.size(); i++) {
		bank_size += 12 * m_bank_list[i].frames.size();
	}
	return bank_size;
}

int32_t AtbFormat::SearchTexture(std::string name)
{
	for (int32_t i = 0; i < m_texture_list.size(); i++) {
		if (m_texture_list[i].name == name) {
			return i;
		}
	}
	return -1;
}

int32_t AtbFormat::SearchPattern(std::string name)
{
	for (int32_t i = 0; i < m_pattern_list.size(); i++) {
		if (m_pattern_list[i].name == name) {
			return i;
		}
	}
	return -1;
}

void AtbFormat::WritePatterns(FILE *file)
{
	uint32_t layer_ofs = m_pattern_ofs + (m_pattern_list.size() * 16);
	for (uint32_t i = 0; i < m_pattern_list.size(); i++) {
		WriteS16(file, m_pattern_list[i].layers.size());
		WriteS16(file, m_pattern_list[i].center_x);
		WriteS16(file, m_pattern_list[i].center_y);
		WriteS16(file, m_pattern_list[i].w);
		WriteS16(file, m_pattern_list[i].h);
		WriteS16(file, 0);
		WriteU32(file, layer_ofs);
		layer_ofs += 32 * m_pattern_list[i].layers.size();
	}
	for (uint32_t i = 0; i < m_pattern_list.size(); i++) {
		for (uint32_t j = 0; j < m_pattern_list[i].layers.size(); j++) {
			WriteU8(file, m_pattern_list[i].layers[j].alpha);
			uint8_t flip_flags = 0;
			if (m_pattern_list[i].layers[j].flip_x) {
				flip_flags |= 1;
			}
			if (m_pattern_list[i].layers[j].flip_y) {
				flip_flags |= 2;
			}
			WriteU8(file, flip_flags);
			int32_t tex_idx = SearchTexture(m_pattern_list[i].layers[j].tex_name);
			if (tex_idx == -1) {
				PrintError("Texture %s doesn't exist.\n", m_pattern_list[i].layers[j].tex_name.c_str());
			}
			WriteS16(file, tex_idx);
			WriteS16(file, m_pattern_list[i].layers[j].src_x);
			WriteS16(file, m_pattern_list[i].layers[j].src_y);
			WriteS16(file, m_pattern_list[i].layers[j].w);
			WriteS16(file, m_pattern_list[i].layers[j].h);
			WriteS16(file, m_pattern_list[i].layers[j].shift_x);
			WriteS16(file, m_pattern_list[i].layers[j].shift_y);
			int16_t vertices[8];
			vertices[6] = vertices[0] = m_pattern_list[i].layers[j].shift_x;
			vertices[3] = vertices[1] = m_pattern_list[i].layers[j].shift_y;
			vertices[2] = vertices[4] = vertices[0] + m_pattern_list[i].layers[j].w;
			vertices[5] = vertices[7] = vertices[1] + m_pattern_list[i].layers[j].h;
			for (int16_t i = 0; i < 8; i++) {
				WriteS16(file, vertices[i]);
			}
		}
	}
}

void AtbFormat::WriteBanks(FILE *file)
{
	uint32_t frame_ofs = m_bank_ofs + (m_bank_list.size() * 8);
	for (uint32_t i = 0; i < m_bank_list.size(); i++) {
		WriteS16(file, m_bank_list[i].frames.size());
		WriteS16(file, 0);
		WriteU32(file, frame_ofs);
		frame_ofs += m_bank_list[i].frames.size() * 12;
	}
	for (uint32_t i = 0; i < m_bank_list.size(); i++) {
		for (uint32_t j = 0; j < m_bank_list[i].frames.size(); j++) {
			int32_t pattern = SearchPattern(m_bank_list[i].frames[j].pattern_name);
			if (pattern == -1) {
				PrintError("Pattern %s doesn't exist.\n", m_bank_list[i].frames[j].pattern_name.c_str());
			}
			WriteS16(file, pattern);
			WriteS16(file, m_bank_list[i].frames[j].delay);
			WriteS16(file, 0);
			WriteS16(file, 0);
			WriteS16(file, 0);
			WriteS16(file, 0);
		}
	}
}

void AtbFormat::WriteTextures(FILE *file)
{
	uint8_t lookup_fmt[ATB_TEX_FORMAT_COUNT] = { TEX_FORMAT_RGBA8, TEX_FORMAT_RGB5A3, TEX_FORMAT_RGB5A3, TEX_FORMAT_CI8, TEX_FORMAT_CI4,
			TEX_FORMAT_IA8, TEX_FORMAT_IA4, TEX_FORMAT_I8, TEX_FORMAT_I4, TEX_FORMAT_A8, TEX_FORMAT_CMPR };
	uint32_t pal_data_ofs = m_texture_ofs + (20 * m_texture_list.size());
	pal_data_ofs = (pal_data_ofs + 31) & 0xFFFFFFE0;
	for (uint32_t i = 0; i < m_texture_list.size(); i++) {
		uint8_t bpp_table[ATB_TEX_FORMAT_COUNT] = { 32, 16, 16, 8, 4, 16, 8, 8, 4, 8, 4 };
		uint32_t tex_data_ofs = pal_data_ofs;
		if (m_texture_list[i].format == ATB_TEX_FORMAT_CI8 || m_texture_list[i].format == ATB_TEX_FORMAT_CI4) {
			tex_data_ofs += 2 << bpp_table[m_texture_list[i].format];
		}
		WriteU8(file, bpp_table[m_texture_list[i].format]);
		WriteU8(file, m_texture_list[i].format);
		if (m_texture_list[i].format == ATB_TEX_FORMAT_CI8 || m_texture_list[i].format == ATB_TEX_FORMAT_CI4) {
			WriteS16(file, 1 << bpp_table[m_texture_list[i].format]);
		} else {
			WriteS16(file, 0);
		}
		WriteS16(file, m_texture_list[i].w);
		WriteS16(file, m_texture_list[i].h);
		
		uint8_t cur_lookup_fmt = lookup_fmt[m_texture_list[i].format];
		uint32_t data_size = GetTexDataSize(cur_lookup_fmt, m_texture_list[i].w, m_texture_list[i].h);
		WriteU32(file, data_size);
		WriteU32(file, pal_data_ofs);
		WriteU32(file, tex_data_ofs);
		pal_data_ofs = tex_data_ofs + data_size;
	}
	AlignFile32(file);
	for (uint32_t i = 0; i < m_texture_list.size(); i++) {
		TextureWrite(file, lookup_fmt[m_texture_list[i].format], m_texture_list[i].w, m_texture_list[i].h, m_texture_list[i].image_data);
	}
}

void AtbFormat::WriteData(FILE *file)
{
	WriteS16(file, m_bank_list.size());
	WriteS16(file, m_pattern_list.size());
	WriteS16(file, m_texture_list.size());
	WriteS16(file, 0x4100);
	m_pattern_ofs = 0x14;
	m_bank_ofs = m_pattern_ofs + GetPatternSize();
	m_texture_ofs = m_bank_ofs + GetBankSize();
	WriteU32(file, m_bank_ofs);
	WriteU32(file, m_pattern_ofs);
	WriteU32(file, m_texture_ofs);
	WritePatterns(file);
	WriteBanks(file);
	WriteTextures(file);
}