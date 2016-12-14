#pragma once

#include "./OPengine.h"
#include "./Pipeline/include/OPrendererFullForward.h"
#include "OPMconvert.h"

struct ExporterState {
	OPexporter exporter;

	OPscene scene;
	OPrendererFullForward* fullForwardRenderer;
	OPcamFreeFlight camera;
	OPuint currentFile;
	OPuint dropCount;
	OPstring* dropFilenames;
	OPfloat Scale = 1.0f;
	OPboundingBox3D bounds;

	OPrendererEntity* entity;
	OPstring* outputFilename = NULL;
	OPstring* outputAbsolutePath = NULL;
	bool autoScale = false;
	bool autoExport = false;
    bool getThumbnail = false;
	bool useAnimation = false;

	ui32 splitterIndex = 0;
	AnimationSplit splitters[100];

	void Init();
	OPint Update(OPtimer* timer);
	void Render(OPfloat delta);
	OPint Exit();

	bool _loadOPMFromFile(const OPchar* filename);
	bool _loadMeshFromFile(const OPchar* filename);
	void _drop(OPuint count, const OPchar** filenames);
	void _processDroppedFiles();
	void _processTexture(const OPchar* filename);
	void _processAnimations(const OPchar* filename);
	void _processModel(const OPchar* filename);
};

extern OPgameState GS_EXAMPLE;
