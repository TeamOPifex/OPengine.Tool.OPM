#pragma once

#include "./OPengine.h"
#include "./OPassimp.h"
#include "Utils.h"


enum ModelFeatures {
	Model_Positions = 0,
	Model_Normals = 1,
	Model_UVs = 2,
	Model_Colors = 3,
	Model_Indices = 4,
	Model_Tangents = 5,
	Model_Bones = 6,
	Model_Skinning = 7,
	Model_Animations = 8,
	Model_Skeletons = 9,
	Model_Meta = 10,
	Model_Bitangents = 11,
	MAX_FEATURES
};

struct AnimationTrack {
	OPchar* Name;
	double Duration;
	ui32 Start;
	ui32 End;
};


struct OPskeletonAnimationResult {
	OPskeletonAnimation** Animations;
	OPchar** AnimationNames;
	OPuint AnimationsCount;
};

#include <map>
#include <vector>

struct BoneInfo
{
	i32 ParentIndex = -1;
	const OPchar* Name = NULL;
	OPmat4 BoneOffset = OPMAT4_IDENTITY;
	OPmat4 FinalTransformation = OPMAT4_IDENTITY;
};

struct OPexporter {
	bool Feature_Normals,
		Feature_UVs,
		Feature_Tangents,
		Feature_BiTangents,
		Feature_Colors,
		Feature_Bones,
		Export_Model,
		Export_Skeleton,
		Export_Animations;

	bool HasAnimations;

	ui32 splitCount;
	ui32* splitStart;
	ui32* splitEnd;
	OPchar** splitName;

	f32 scale = 1.0f;

	const OPchar* path;
	OPchar* output;

	Assimp::Importer importer;
	const aiScene* scene;

	AnimationTrack* animationTracks;
	ui32 animationCount;

	ui32 features[MAX_FEATURES];
	OPindexSize indexSize;

	OPfloat* boneWeights;
	i32* boneIndices;

	std::map<std::string, ui32> boneMapping;
	ui32 numBones;
	vector<BoneInfo> boneInfo;

	OPmodel* existingModel;

	OPexporter() { }

	OPexporter(const OPchar* filename, OPmodel* desc) {
		Init(filename, desc);
	}

	void Init(const OPchar* filename, OPmodel* desc);
	void Export();
	void Export(const OPchar* output);
	OPskeleton* LoadSkeleton(OPstream* stream);
	OPskeletonAnimationResult LoadAnimations(OPstream* stream);

	// Private
	void _write(const OPchar* outputFinal);
	void _writeSkeleton(const OPchar* outputFinal);
	void _writeAnimations(const OPchar* outputFinal);
	void _setFeatures();
	ui32 _getFeaturesFlag();
	ui32 _getTotalVertices();
	ui32 _getTotalIndices();
	ui32 _getTotalVertices(aiMesh* mesh);
	ui32 _getTotalIndices(aiMesh* mesh);
	void _writeMeshData(ofstream* myFile);
	void _setBoneData(aiMesh* mesh);
	ui32 _findBoneIndex(const OPchar* name);
	OPmat4 _findBoneOffset(ui32 ind);
	OPmat4 _findBoneOffset(const OPchar* name);
	void _loadBones();
	void _setHierarchy();
	void _setHierarchy(aiNode* node, i32 parent);
};

struct BoneWeight {
	OPfloat weights[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	i32 bones[4] = { 0, 0, 0, 0 };
};

ui32 GetAvailableTracks(const OPchar* path, OPchar** buff, double* durations, ui32 max);