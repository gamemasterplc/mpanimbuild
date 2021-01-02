#include <algorithm>
#include <cctype>
#include "mpanimbuild.h"
#include "stb_image.h"
#include "AnimExFormat.h"

tinyxml2::XMLElement *AnimExFormat::GetFirstChildNode(tinyxml2::XMLElement *node)
{
	if (node->FirstChildElement("root")) {
		return node->FirstChildElement("root");
	} else if (node->FirstChildElement("type1")) {
		return node->FirstChildElement("type1");
	} else if (node->FirstChildElement("transform")) {
		return node->FirstChildElement("transform");
	} else if (node->FirstChildElement("image")) {
		return node->FirstChildElement("image");
	}
	return nullptr;
}

tinyxml2::XMLElement *AnimExFormat::GetSiblingNode(tinyxml2::XMLElement *node)
{
	if (node->NextSiblingElement("root")) {
		return node->NextSiblingElement("root");
	} else if (node->NextSiblingElement("type1")) {
		return node->NextSiblingElement("type1");
	} else if (node->NextSiblingElement("transform")) {
		return node->NextSiblingElement("transform");
	} else if (node->NextSiblingElement("image")) {
		return node->NextSiblingElement("image");
	}
	return nullptr;
}

void AnimExFormat::ReadTransform(tinyxml2::XMLElement *element, AnimExTransform *node)
{
	const char *str_temp;
	PrintXmlError(element->QueryAttribute("name", &str_temp));
	node->name = str_temp;
	node->scale_x = node->scale_y = node->scale_z = 1.0f;
	node->rot_x = node->rot_y = node->rot_z = 0.0f;
	node->pos_x = node->pos_y = node->pos_z = 0.0f;
	element->QueryFloatAttribute("scale_x", &node->scale_x);
	element->QueryFloatAttribute("scale_y", &node->scale_y);
	element->QueryFloatAttribute("rot_z", &node->rot_z);
	element->QueryFloatAttribute("pos_x", &node->pos_x);
	element->QueryFloatAttribute("pos_y", &node->pos_y);
}

void AnimExFormat::ReadImage(tinyxml2::XMLElement *element, AnimExImage *node)
{
	const char *str_temp;
	PrintXmlError(element->QueryAttribute("name", &str_temp));
	node->name = str_temp;
	StringReference str_reference;
	str_reference.data = node->name;
	if (data.strings.size() == 0) {
		str_reference.ofs = 0;
	} else {
		str_reference.ofs = data.strings[data.strings.size() - 1].ofs + data.strings[data.strings.size() - 1].data.length() + 1;
	}
	data.strings.push_back(str_reference);
	PrintXmlError(element->QueryAttribute("texture_name", &str_temp));
	node->tex_name = str_temp;
	PrintXmlError(element->QueryFloatAttribute("x", &node->x));
	PrintXmlError(element->QueryFloatAttribute("y", &node->y));
	PrintXmlError(element->QueryFloatAttribute("w", &node->w));
	PrintXmlError(element->QueryFloatAttribute("h", &node->h));
	node->uv_x = node->uv_y = 0.0f;
	node->uv_w = node->uv_h = 0.0f;
	element->QueryFloatAttribute("uv_x", &node->uv_x);
	element->QueryFloatAttribute("uv_y", &node->uv_y);
	element->QueryFloatAttribute("uv_w", &node->uv_w);
	element->QueryFloatAttribute("uv_h", &node->uv_h);
	element->QueryFloatAttribute("color_r", &node->color[0]);
	element->QueryFloatAttribute("color_g", &node->color[1]);
	element->QueryFloatAttribute("color_b", &node->color[2]);
	element->QueryFloatAttribute("color_a", &node->color[3]);
}

void AnimExFormat::ReadNode(tinyxml2::XMLElement *node, AnimExNode *parent)
{
	tinyxml2::XMLElement *child_node = GetFirstChildNode(node);
	AnimExNode *anim_node = nullptr;
	AnimExTransform *transform = nullptr;
	AnimExImage *image = nullptr;
	std::string value = node->Name();
	if (value == "root") {
		if (header.root_cnt != 0) {
			PrintError("Found Multiple Root Nodes.\n");
		}
		anim_node = new AnimExNode;
		anim_node->type = ANIMEX_NODE_TYPE_ROOT;
		data.root = anim_node;
		header.root_cnt++;
	} else if (value == "type1") {
		if (header.type1_cnt != 0) {
			PrintError("Found Multiple Type 1 Nodes.\n");
		}
		anim_node = new AnimExNode;
		anim_node->type = 1;
		data.type1 = anim_node;
		header.type1_cnt++;
	} else if (value == "transform") {
		transform = new AnimExTransform;
		anim_node = &transform->node;
		anim_node->type = ANIMEX_NODE_TYPE_TRANSFORM;
		anim_node->node_idx = data.transforms.size();
		ReadTransform(node, transform);
	} else if (value == "image") {
		image = new AnimExImage;
		anim_node = &image->node;
		anim_node->node_idx = data.images.size();
		anim_node->type = ANIMEX_NODE_TYPE_IMAGE;
		ReadImage(node, image);
	}
	if (value == "transform") {
		data.transforms.push_back(transform);
	} else if (value == "image") {
		data.images.push_back(image);
	}
	if (parent) {
		parent->children.push_back(anim_node);
	}
	while (child_node) {
		ReadNode(child_node, anim_node);
		child_node = GetSiblingNode(child_node);
	}
	
}

int32_t AnimExFormat::GetTransformIdx(std::string name)
{
	for (uint32_t i = 0; i < data.transforms.size(); i++) {
		if (name == data.transforms[i]->name) {
			return i;
		}
	}
	return -1;
}

int32_t AnimExFormat::GetImageIdx(std::string name)
{
	for (uint32_t i = 0; i < data.images.size(); i++) {
		if (name == data.images[i]->name) {
			return i;
		}
	}
	return -1;
}

uint32_t AnimExFormat::GetInterpType(std::string value)
{
	std::string type_str[3] = { "none", "linear", "bezier" };
	uint32_t modes[3] = { ANIMEX_INTERP_MODE_NONE, ANIMEX_INTERP_MODE_LINEAR, ANIMEX_INTERP_MODE_SPLINE };
	for (uint32_t i = 0; i < 3; i++) {
		if (type_str[i] == value) {
			return modes[i];
		}
	}
	return ANIMEX_INTERP_MODE_NONE;
}

void AnimExFormat::ReadTracks(tinyxml2::XMLElement *root)
{
	tinyxml2::XMLElement *track_node = root->FirstChildElement("track");
	while (track_node) {
		AnimExTrack track;
		const char *str_temp;
		PrintXmlError(track_node->QueryAttribute("target_name", &str_temp));
		int32_t node_idx = GetTransformIdx(str_temp);
		int16_t node_type = -1;
		if (node_idx == -1) {
			node_idx = GetImageIdx(str_temp);
			if (node_idx != -1) {
				node_type = ANIMEX_NODE_TYPE_IMAGE;
			}
		} else {
			node_type = ANIMEX_NODE_TYPE_TRANSFORM;
		}
		track.node_type = node_type;
		track.node_id = node_idx;
		PrintXmlError(track_node->QueryAttribute("var", &str_temp));
		std::string var_name = str_temp;
		if (node_type == ANIMEX_NODE_TYPE_TRANSFORM) {
			if (var_name == "pos_x") {
				track.track_type = ANIMEX_TRACK_POS;
				track.var_id = ANIMEX_TRACK_VAR_X;
			} else if (var_name == "pos_y") {
				track.track_type = ANIMEX_TRACK_POS;
				track.var_id = ANIMEX_TRACK_VAR_Y;
			} else if (var_name == "rot_z") {
				track.track_type = ANIMEX_TRACK_ROTATE;
				track.var_id = ANIMEX_TRACK_VAR_Z;
			} else if (var_name == "scale_x") {
				track.track_type = ANIMEX_TRACK_SCALE;
				track.var_id = ANIMEX_TRACK_VAR_X;
			} else if (var_name == "scale_y") {
				track.track_type = ANIMEX_TRACK_SCALE;
				track.var_id = ANIMEX_TRACK_VAR_Y;
			} else {
				track.track_type = ANIMEX_TRACK_POS;
				track.var_id = ANIMEX_TRACK_VAR_Z;
			}
		} else if (node_type == ANIMEX_NODE_TYPE_IMAGE) {
			if (var_name == "color_r") {
				track.track_type = ANIMEX_TRACK_COLOR;
				track.var_id = ANIMEX_TRACK_VAR_R;
			} else if (var_name == "color_g") {
				track.track_type = ANIMEX_TRACK_COLOR;
				track.var_id = ANIMEX_TRACK_VAR_G;
			}
			else if (var_name == "color_b") {
				track.track_type = ANIMEX_TRACK_COLOR;
				track.var_id = ANIMEX_TRACK_VAR_B;
			}
			else if (var_name == "color_a") {
				track.track_type = ANIMEX_TRACK_COLOR;
				track.var_id = ANIMEX_TRACK_VAR_A;
			} else {
				track.track_type = ANIMEX_TRACK_COLOR;
				track.var_id = 0;
			}
		} else {
			track.track_type = ANIMEX_TRACK_POS;
			track.var_id = ANIMEX_TRACK_VAR_Z;
		}
		track.keyframe_start = data.keyframes.size();
		tinyxml2::XMLElement *keyframe_node = track_node->FirstChildElement("keyframe");
		while (keyframe_node) {
			AnimExKeyframe keyframe;
			str_temp = "";
			keyframe_node->QueryAttribute("interp_mode", &str_temp);
			keyframe.interp_type = GetInterpType(str_temp);
			keyframe.frame_num = 0;
			keyframe_node->QueryUnsignedAttribute("frame_num", &keyframe.frame_num);
			keyframe.points[0] = keyframe.points[1] = keyframe.points[2] = keyframe.points[3] = 0;
			if (keyframe.interp_type == ANIMEX_INTERP_MODE_SPLINE) {
				bool use_point3 = false;
				PrintXmlError(keyframe_node->QueryFloatAttribute("point1", &keyframe.points[0]));
				keyframe_node->QueryBoolAttribute("use_point3", &use_point3);
				if (use_point3) {
					keyframe.points[1] = 3.0f;
				}
				PrintXmlError(keyframe_node->QueryFloatAttribute("point2", &keyframe.points[2]));
				keyframe_node->QueryFloatAttribute("point3", &keyframe.points[3]);
			} else {
				PrintXmlError(keyframe_node->QueryFloatAttribute("point", &keyframe.points[0]));
			}
			track.keyframes.push_back(keyframe);
			data.keyframes.push_back(keyframe);
			keyframe_node = keyframe_node->NextSiblingElement("keyframe");
		}
		data.tracks.push_back(track);
		track_node = track_node->NextSiblingElement("track");
	}
}

uint8_t AnimExFormat::GetTextureFormat(std::string id)
{
	std::string format_list[10] = { "RGBA8", "RGB5A3", "CI8", "CI4", "IA8", "IA4", "I8", "I4", "A8", "CMPR" };
	uint8_t format_ids[10] = { ANIMEX_TEX_FORMAT_RGBA8, ANIMEX_TEX_FORMAT_RGB5A3, ANIMEX_TEX_FORMAT_CI8, ANIMEX_TEX_FORMAT_CI4,
		ANIMEX_TEX_FORMAT_IA8, ANIMEX_TEX_FORMAT_IA4, ANIMEX_TEX_FORMAT_I8, ANIMEX_TEX_FORMAT_I4, ANIMEX_TEX_FORMAT_A8, ANIMEX_TEX_FORMAT_CMPR };
	std::transform(id.begin(), id.end(), id.begin(), ::toupper);
	for (uint32_t i = 0; i < 10; i++) {
		if (format_list[i] == id) {
			return format_ids[i];
		}
	}
	return ANIMEX_TEX_FORMAT_RGBA8;
}

void AnimExFormat::ReadTextures(std::string base_path, tinyxml2::XMLElement *root)
{
	tinyxml2::XMLElement *texture_node = root->FirstChildElement("texture");
	while (texture_node) {
		AnimExTexture texture;
		const char *str_temp;
		PrintXmlError(texture_node->QueryAttribute("name", &str_temp));
		texture.name = str_temp;
		texture_node->QueryAttribute("format", &str_temp);
		texture.format = GetTextureFormat(str_temp);
		PrintXmlError(texture_node->QueryAttribute("file", &str_temp));
		std::string file_rel_path = str_temp;
		std::string file_path = base_path + file_rel_path;
		int channels;
		texture.data = stbi_load(file_path.c_str(), &texture.w, &texture.h, &channels, 4);
		data.textures.push_back(texture);
		texture_node = texture_node->NextSiblingElement("texture");
	}
}

void AnimExFormat::ReadBanks(tinyxml2::XMLElement *root)
{
	tinyxml2::XMLElement *bank_node = root->FirstChildElement("bank");
	while (bank_node) {
		unsigned int frame_start = 0;
		bank_node->QueryUnsignedAttribute("frame_start", &frame_start);
		data.bank_frame_starts.push_back(frame_start);
		bank_node = bank_node->NextSiblingElement("bank");
	}
}

void AnimExFormat::AddNodeReference(AnimExNode *node)
{
	node->child_ref_idx = data.node_references.size();
	for (size_t i = 0; i < node->children.size(); i++) {
		data.node_references.push_back(node->children[i]);
	}
}

void AnimExFormat::AddTransformReferences()
{
	for (size_t i = 0; i < data.transforms.size(); i++) {
		AddNodeReference(&data.transforms[i]->node);
	}
}

void AnimExFormat::AddImageReferences()
{
	for (size_t i = 0; i < data.images.size(); i++) {
		AddNodeReference(&data.images[i]->node);
	}
}

AnimExFormat::AnimExFormat(tinyxml2::XMLDocument *document, std::string base_path)
{
	tinyxml2::XMLElement *root = document->FirstChild()->ToElement();
	data.length = 1;
	root->QueryUnsignedAttribute("length", &data.length);
	tinyxml2::XMLElement *root_elem = root->FirstChildElement("root");
	if (!root_elem) {
		PrintError("Failed to find root element.\n");
	}
	header.root_cnt = 0;
	header.type1_cnt = 0;
	header.transform_cnt = 0;
	header.image_cnt = 0;
	ReadNode(root_elem, nullptr);
	ReadTracks(root);
	tinyxml2::XMLElement *textures_elem = root->FirstChildElement("textures");
	if (!textures_elem) {
		PrintError("Failed to find textures element.\n");
	}
	ReadTextures(base_path, textures_elem);
	tinyxml2::XMLElement *banks_elem = root->FirstChildElement("banks");
	if (!banks_elem) {
		PrintError("Failed to find banks element.\n");
	}
	ReadBanks(banks_elem);
	AddNodeReference(data.root);
	AddNodeReference(data.type1);
	AddTransformReferences();
	AddImageReferences();
}

AnimExFormat::~AnimExFormat()
{
	delete data.root;
	delete data.type1;
	for (size_t i = 0; i < data.transforms.size(); i++) {
		delete data.transforms[i];
	}
	for (size_t i = 0; i < data.images.size(); i++) {
		delete data.images[i];
	}
	for (size_t i = 0; i < data.textures.size(); i++) {
		stbi_image_free(data.textures[i].data);
	}
}

uint32_t AnimExFormat::GetStringTableSize()
{
	uint32_t size = 0;
	for (uint32_t i = 0; i < data.strings.size(); i++) {
		size += data.strings[i].data.length() + 1;
	}
	return size;
}

void AnimExFormat::WriteNode(FILE *dst_file, AnimExNode *node)
{
	WriteS16(dst_file, node->type);
	WriteU16(dst_file, node->children.size());
	uint32_t child_ofs = header.node_ref_ofs + (node->child_ref_idx * 4);
	WriteU32(dst_file, child_ofs);
}

void AnimExFormat::WriteTransforms(FILE *dst_file)
{
	for (uint32_t i = 0; i < data.transforms.size(); i++) {
		WriteNode(dst_file, &data.transforms[i]->node);
		WriteFloat(dst_file, data.transforms[i]->scale_x);
		WriteFloat(dst_file, data.transforms[i]->scale_y);
		WriteFloat(dst_file, data.transforms[i]->scale_z);
		WriteFloat(dst_file, data.transforms[i]->rot_x);
		WriteFloat(dst_file, data.transforms[i]->rot_y);
		WriteFloat(dst_file, data.transforms[i]->rot_z);
		WriteFloat(dst_file, data.transforms[i]->pos_x);
		WriteFloat(dst_file, data.transforms[i]->pos_x);
		WriteFloat(dst_file, data.transforms[i]->pos_x);
		for (uint32_t j = 0; j < 6; j++) {
			WriteFloat(dst_file, 0.0f);
		}
	}
}

int32_t AnimExFormat::GetTextureIdx(std::string name)
{
	for (uint32_t i = 0; i < data.textures.size(); i++) {
		if (data.textures[i].name == name) {
			return i;
		}
	}
	return -1;
}

uint32_t AnimExFormat::GetStringOfs(std::string string)
{
	uint32_t ofs = 0;
	for (uint32_t i = 0; i < data.strings.size(); i++) {
		if (data.strings[i].data == string) {
			return data.strings[i].ofs;
		}
	}
	return 0;
}

void AnimExFormat::WriteImages(FILE *dst_file)
{
	for (uint32_t i = 0; i < data.images.size(); i++) {
		WriteNode(dst_file, &data.images[i]->node);
		WriteU32(dst_file, GetStringOfs(data.images[i]->name) + header.str_table_ofs);
		float vertices[12];
		float uv[8];
		vertices[2] = vertices[5] = vertices[8] = vertices[11] = 0;
		vertices[0] = vertices[9] = data.images[i]->x;
		vertices[1] = vertices[4] = data.images[i]->y;
		vertices[3] = vertices[6] = (vertices[0] + data.images[i]->w);
		vertices[7] = vertices[10] = (vertices[1] + data.images[i]->h);
		for (uint32_t i = 0; i < 12; i++) {
			WriteFloat(dst_file, vertices[i]);
		}
		uv[0] = uv[6] = data.images[i]->uv_x;
		uv[1] = uv[3] = data.images[i]->uv_y;
		uv[2] = uv[4] = uv[0] + data.images[i]->uv_w;
		uv[5] = uv[7] = uv[1] + data.images[i]->uv_h;
		for (uint32_t i = 0; i < 8; i++) {
			WriteFloat(dst_file, uv[i]);
		}
		WriteFloat(dst_file, data.images[i]->color[0]);
		WriteFloat(dst_file, data.images[i]->color[1]);
		WriteFloat(dst_file, data.images[i]->color[2]);
		WriteFloat(dst_file, data.images[i]->color[3]);
		int32_t tex_id = GetTextureIdx(data.images[i]->tex_name);
		if (tex_id == -1) {
			PrintError("Failed to find texture %s.\n", data.images[i]->tex_name.c_str());
		}
		WriteU32(dst_file, header.texture_ofs + (tex_id * 20));
	}
}

void AnimExFormat::WriteTracks(FILE *dst_file)
{
	for (uint32_t i = 0; i < data.tracks.size(); i++) {
		WriteS16(dst_file, data.tracks[i].node_type);
		WriteU16(dst_file, data.tracks[i].node_id);
		WriteU16(dst_file, data.tracks[i].track_type);
		WriteU16(dst_file, data.tracks[i].var_id);
		WriteU32(dst_file, data.tracks[i].keyframes.size());
		uint32_t keyframe_ofs = header.keyframe_ofs + (data.tracks[i].keyframe_start * 24);
		WriteU32(dst_file, keyframe_ofs);
	}
}

void AnimExFormat::WriteKeyframes(FILE *dst_file)
{
	for (uint32_t i = 0; i < data.keyframes.size(); i++) {
		WriteU32(dst_file, data.keyframes[i].interp_type);
		WriteU32(dst_file, data.keyframes[i].frame_num);
		WriteFloat(dst_file, data.keyframes[i].points[0]);
		WriteFloat(dst_file, data.keyframes[i].points[1]);
		WriteFloat(dst_file, data.keyframes[i].points[2]);
		WriteFloat(dst_file, data.keyframes[i].points[3]);
	}
}

void AnimExFormat::WriteNodeReferences(FILE *dst_file)
{
	for (uint32_t i = 0; i < data.node_references.size(); i++) {
		uint32_t node_ofs = 0;
		switch (data.node_references[i]->type) {
			case ANIMEX_NODE_TYPE_ROOT:
				node_ofs = header.root_ofs;
				break;

			case 1:
				node_ofs = header.type1_ofs;
				break;

			case ANIMEX_NODE_TYPE_TRANSFORM:
				node_ofs = header.transform_ofs + (data.node_references[i]->node_idx * 68);
				break;

			case ANIMEX_NODE_TYPE_IMAGE:
				node_ofs = header.image_ofs + (data.node_references[i]->node_idx * 112);
				break;

			default:
				break;
		}
		WriteU32(dst_file, node_ofs);
	}
}

void AnimExFormat::WriteBanks(FILE *dst_file)
{
	for (uint32_t i = 0; i < data.bank_frame_starts.size(); i++) {
		WriteU32(dst_file, data.bank_frame_starts[i]);
	}
}

void AnimExFormat::WriteStringTable(FILE *dst_file)
{
	for (uint32_t i = 0; i < data.strings.size(); i++) {
		const char *string = data.strings[i].data.c_str();
		fwrite(string, 1, data.strings[i].data.length()+1, dst_file);
	}
	if (ftell(dst_file) % 4) {
		uint32_t value = 0;
		fwrite(&value, 1, 4-(ftell(dst_file) % 4), dst_file);
	}
}

void AnimExFormat::WriteTextures(FILE *dst_file)
{
	uint8_t lookup_fmt[ANIMEX_TEX_FORMAT_COUNT] = { TEX_FORMAT_RGBA8, TEX_FORMAT_RGB5A3, TEX_FORMAT_RGB5A3, TEX_FORMAT_CI8, TEX_FORMAT_CI4,
			TEX_FORMAT_IA8, TEX_FORMAT_IA4, TEX_FORMAT_I8, TEX_FORMAT_I4, TEX_FORMAT_A8, TEX_FORMAT_CMPR };
	uint32_t pal_ofs = header.texture_ofs + (20 * data.textures.size());
	pal_ofs = (pal_ofs + 31) & 0xFFFFFFE0;
	for (uint32_t i = 0; i < data.textures.size(); i++) {
		uint8_t bpp_table[ANIMEX_TEX_FORMAT_COUNT] = { 32, 16, 16, 8, 4, 16, 8, 8, 4, 8, 4 };
		uint32_t tex_ofs = pal_ofs;
		WriteU8(dst_file, bpp_table[data.textures[i].format]);
		WriteU8(dst_file, data.textures[i].format);
		if (data.textures[i].format == ANIMEX_TEX_FORMAT_CI8 || data.textures[i].format == ANIMEX_TEX_FORMAT_CI4) {
			tex_ofs += 2 << bpp_table[data.textures[i].format];
		}
		if (data.textures[i].format == ANIMEX_TEX_FORMAT_CI8 || data.textures[i].format == ANIMEX_TEX_FORMAT_CI8) {
			WriteS16(dst_file, 1 << bpp_table[data.textures[i].format]);
		} else {
			WriteS16(dst_file, 0);
		}
		WriteS16(dst_file, data.textures[i].w);
		WriteS16(dst_file, data.textures[i].h);

		uint8_t cur_lookup_fmt = lookup_fmt[data.textures[i].format];
		uint32_t data_size = GetTexDataSize(cur_lookup_fmt, data.textures[i].w, data.textures[i].h);
		WriteU32(dst_file, data_size);
		WriteU32(dst_file, pal_ofs);
		WriteU32(dst_file, tex_ofs);
		pal_ofs = tex_ofs + data_size;
	}
	AlignFile32(dst_file);
	for (uint32_t i = 0; i < data.textures.size(); i++) {
		TextureWrite(dst_file, lookup_fmt[data.textures[i].format], data.textures[i].w, data.textures[i].h, data.textures[i].data);
	}
}
void AnimExFormat::WriteData(FILE *dst_file)
{
	WriteU32(dst_file, 0x414E494D);
	WriteS16(dst_file, 1);
	WriteS16(dst_file, 0);
	WriteU32(dst_file, data.length);
	WriteU32(dst_file, 0);
	WriteU32(dst_file, 1);
	WriteU32(dst_file, 1);
	WriteU32(dst_file, data.transforms.size());
	WriteU32(dst_file, data.images.size());
	WriteU32(dst_file, data.tracks.size());
	WriteU32(dst_file, data.keyframes.size());
	WriteU32(dst_file, data.textures.size());
	WriteU32(dst_file, data.node_references.size());
	WriteU32(dst_file, data.bank_frame_starts.size());
	WriteU32(dst_file, GetStringTableSize());
	header.root_ofs = 96;
	WriteU32(dst_file, header.root_ofs);
	header.type1_ofs = header.root_ofs + 8;
	WriteU32(dst_file, header.type1_ofs);
	header.transform_ofs = header.type1_ofs + 8;
	WriteU32(dst_file, header.transform_ofs);
	header.image_ofs = header.transform_ofs + (data.transforms.size() * 68);
	WriteU32(dst_file, header.image_ofs);
	header.track_ofs = header.image_ofs + (data.images.size() * 112);
	WriteU32(dst_file, header.track_ofs);
	header.keyframe_ofs = header.track_ofs + (data.tracks.size() * 16);
	WriteU32(dst_file, header.keyframe_ofs);
	header.node_ref_ofs = header.keyframe_ofs + (data.keyframes.size() * 24);
	header.frame_start_ofs = header.node_ref_ofs + (data.node_references.size() * 4);
	header.str_table_ofs = header.frame_start_ofs + (data.bank_frame_starts.size() * 4);
	header.texture_ofs = (header.str_table_ofs + GetStringTableSize() + 3) & 0xFFFFFFFC;
	WriteU32(dst_file, header.texture_ofs);
	WriteU32(dst_file, header.node_ref_ofs);
	WriteU32(dst_file, header.frame_start_ofs);
	WriteU32(dst_file, header.str_table_ofs);
	WriteNode(dst_file, data.root);
	WriteNode(dst_file, data.type1);
	WriteTransforms(dst_file);
	WriteImages(dst_file);
	WriteTracks(dst_file);
	WriteKeyframes(dst_file);
	WriteNodeReferences(dst_file);
	WriteBanks(dst_file);
	WriteStringTable(dst_file);
	WriteTextures(dst_file);
}