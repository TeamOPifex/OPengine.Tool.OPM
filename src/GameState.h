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
	bool autoExport = false;
    bool getThumbnail = false;

	void Init();
	OPint Update(OPtimer* timer);
	void Render(OPfloat delta);
	OPint Exit();

	bool _loadMeshFromFile(const OPchar* filename);
	void _drop(OPuint count, const OPchar** filenames);
	void _processDroppedFiles();
	void _processTexture(const OPchar* filename);
	void _processAnimations(const OPchar* filename);
	void _processModel(const OPchar* filename);
};

extern OPgameState GS_EXAMPLE;
