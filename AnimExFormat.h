#pragma once
#include "AnimFormat.h"

#include "tinyxml2.h"
#include <string>
#include <vector>

#define ANIMEX_TEX_FORMAT_RGBA8 0
#define ANIMEX_TEX_FORMAT_RGB5A3 1
#define ANIMEX_TEX_FORMAT_RGB5A3_DUPE 2
#define ANIMEX_TEX_FORMAT_CI8 3
#define ANIMEX_TEX_FORMAT_CI4 4
#define ANIMEX_TEX_FORMAT_IA8 5
#define ANIMEX_TEX_FORMAT_IA4 6
#define ANIMEX_TEX_FORMAT_I8 7
#define ANIMEX_TEX_FORMAT_I4 8
#define ANIMEX_TEX_FORMAT_A8 9
#define ANIMEX_TEX_FORMAT_CMPR 10
#define ANIMEX_TEX_FORMAT_COUNT 11

#define ANIMEX_NODE_TYPE_ROOT 0
#define ANIMEX_NODE_TYPE_TRANSFORM 2
#define ANIMEX_NODE_TYPE_IMAGE 3

#define ANIMEX_INTERP_MODE_NONE 11
#define ANIMEX_INTERP_MODE_LINEAR 12
#define ANIMEX_INTERP_MODE_SPLINE 13

#define ANIMEX_TRACK_POS 0
#define ANIMEX_TRACK_ROTATE 1
#define ANIMEX_TRACK_SCALE 2
#define ANIMEX_TRACK_COLOR 3

#define ANIMEX_TRACK_VAR_X 4
#define ANIMEX_TRACK_VAR_Y 5
#define ANIMEX_TRACK_VAR_Z 6
#define ANIMEX_TRACK_VAR_R 7
#define ANIMEX_TRACK_VAR_G 8
#define ANIMEX_TRACK_VAR_B 9
#define ANIMEX_TRACK_VAR_A 10

struct AnimExTexture {
	uint8_t format;
	std::string name;
	int w;
	int h;
	uint8_t *data;
};

struct AnimExNode {
	int16_t type;
	unsigned int child_ref_idx;
	uint32_t node_idx;
	std::vector<AnimExNode *> children;
};

struct AnimExTransform {
	AnimExNode node;
	std::string name;
	float scale_x;
	float scale_y;
	float scale_z;
	float rot_x;
	float rot_y;
	float rot_z;
	float pos_x;
	float pos_y;
	float pos_z;
};

struct AnimExImage {
	AnimExNode node;
	std::string name;
	float x;
	float y;
	float w;
	float h;
	float uv_x;
	float uv_y;
	float uv_w;
	float uv_h;
	float color[4];
	std::string tex_name;
};

struct AnimExKeyframe {
	uint32_t interp_type;
	unsigned int frame_num;
	float points[4];
};

struct AnimExTrack {
	int16_t node_type;
	uint16_t node_id;
	uint16_t track_type;
	uint16_t var_id;
	uint32_t keyframe_start;
	std::vector<AnimExKeyframe> keyframes;
};

struct StringReference {
	std::string data;
	uint32_t ofs;
};

struct AnimExData {
	unsigned int length;
	AnimExNode *root;
	AnimExNode *type1;
	std::vector<AnimExTransform *> transforms;
	std::vector<AnimExImage *> images;
	std::vector<AnimExTrack> tracks;
	std::vector<AnimExKeyframe> keyframes;
	std::vector<AnimExTexture> textures;
	std::vector<AnimExNode *> node_references;
	std::vector<unsigned int> bank_frame_starts;
	std::vector<StringReference> strings;
};

typedef struct animex_header {
	uint32_t magic;
	uint32_t frame_cnt;
	uint32_t ref_cnt;
	uint32_t root_cnt;
	uint32_t type1_cnt;
	uint32_t transform_cnt;
	uint32_t image_cnt;
	uint32_t track_cnt;
	uint32_t keyframe_cnt;
	uint32_t texture_cnt;
	uint32_t node_ref_cnt;
	uint32_t frame_start_cnt;
	uint32_t str_table_len;
	uint32_t root_ofs;
	uint32_t type1_ofs;
	uint32_t transform_ofs;
	uint32_t image_ofs;
	uint32_t track_ofs;
	uint32_t keyframe_ofs;
	uint32_t texture_ofs;
	uint32_t node_ref_ofs;
	uint32_t frame_start_ofs;
	uint32_t str_table_ofs;
} AnimExHeader;


class AnimExFormat : public AnimFormat
{
public:
	AnimExFormat(tinyxml2::XMLDocument *document, std::string base_path);
	~AnimExFormat();

public:
	virtual void WriteData(FILE *dst_file);

private:
	void ReadNode(tinyxml2::XMLElement *node, AnimExNode *parent);
	void ReadTransform(tinyxml2::XMLElement *element, AnimExTransform *node);
	void ReadImage(tinyxml2::XMLElement *element, AnimExImage *node);
	void ReadTracks(tinyxml2::XMLElement *root);
	void ReadTextures(std::string base_path, tinyxml2::XMLElement *root);
	void ReadBanks(tinyxml2::XMLElement *root);
	void AddNodeReference(AnimExNode *node);
	void AddTransformReferences();
	void AddImageReferences();
	void WriteNode(FILE *dst_file, AnimExNode *node);
	void WriteTransforms(FILE *dst_file);
	void WriteImages(FILE *dst_file);
	void WriteTracks(FILE *dst_file);
	void WriteKeyframes(FILE *dst_file);
	void WriteNodeReferences(FILE *dst_file);
	void WriteBanks(FILE *dst_file);
	void WriteStringTable(FILE *dst_file);
	void WriteTextures(FILE *dst_file);
	uint32_t GetStringTableSize();
	int32_t GetTransformIdx(std::string name);
	int32_t GetImageIdx(std::string name);
	int32_t GetTextureIdx(std::string name);
	uint32_t GetInterpType(std::string value);
	uint32_t GetStringOfs(std::string string);
	uint8_t GetTextureFormat(std::string id);
	tinyxml2::XMLElement *GetFirstChildNode(tinyxml2::XMLElement *node);
	tinyxml2::XMLElement *GetSiblingNode(tinyxml2::XMLElement *node);

private:
	AnimExData data;
	AnimExHeader header;
	uint32_t m_node_idx;
};

