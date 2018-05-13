#pragma once

#include "./OPengine.h"
#include "./Pipeline/include/Renderers/OPrendererForward.h"
#include "./Pipeline/include/Renderers/OPrendererPBR.h"
#include "OPMconvert.h"
#include "ModelViewer.h"
#include "WindowSnapshot.h"

class ExporterState : public OPgameState {
public:
	OPexporter exporter;
	ModelViewer modelViewer;
	WindowSnapshot* windowSnapshot;

	OPscene scene;
	OPrendererForward fullForwardRenderer;
	OPrendererPBR pbrRenderer;
	OPcamFreeFlight camera;
	OPcam shadowCam;


	OPuint currentFile;
	OPuint dropCount;
	OPstring* dropFilenames;

	bool autoScale = false;
	bool autoExport = false;
    bool getThumbnail = false;
	bool useAnimation = false;
	bool animsFromFile = false;


	

	int ind = 0;
	int meshInd = 0;
	int AnimStart = 0, AnimEnd = 0;
	char AnimName[100];

	char* items = "test\0two\0three";

	bool showAddAnimation = false;
	bool showSkeleton = false;


	void Init(OPgameState* state);
	OPint Update(OPtimer* timer);
	void Render(OPfloat delta);
	OPint Exit(OPgameState* state);

	void renderGUI();
	void renderGUIMenu();
	void renderGUISettings();
	void renderGUISkeleton();
	void renderGUIAddAnimation();
	void renderGUIExport();
	void renderGUIMeshSelect();
	void renderGUITextureSelect();
	void _drop(OPuint count, const OPchar** filenames);
	void _processDroppedFiles();
	void _processModel(const OPchar* filename);
	bool LoadOPM();
};

extern ExporterState GS_EXAMPLE;
