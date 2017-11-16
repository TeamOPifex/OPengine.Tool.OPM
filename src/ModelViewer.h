#pragma once

#include "./OPengine.h"
#include "OPMconvert.h"

#define DEFAULT_TEXTURE "Default_Albedo.png"

class ModelViewer {
private:
	OPscene* scene;
	OPexporter* exporter;

public:
	OPmodel* model = NULL;
	OPrendererEntity* entity = NULL;
	OPboundingBox3D bounds;
	OPfloat Scale = 1.0f;
	OPskeleton* activeSkeleton = NULL;
	OPstring* OutputFilename = NULL;
	OPstring* OutputAbsolutePath = NULL;
	OPskeletonAnimationResult animations = { NULL, 0 };
	OPskeletonAnimation* activeAnimation = NULL;
	ui32 splitterIndex = 0;
	bool splitFileNameForAnim = false;
	AnimationSplit splitters[100];
	OPtexture* result = NULL;

	ModelViewer() {}
	ModelViewer(OPscene* s, OPexporter* e);

	bool LoadOPMFromFile(const OPchar* fullFilePath);
	bool LoadModelFromFile(const OPchar* fullFilePath, bool animsFromFile, bool* useAnimation);
	bool ApplyTexture(const OPchar* filename, ui32 meshInd);
	bool ProcessAnimationsFile(const OPchar* filename);
};