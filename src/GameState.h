#pragma once

#include "./OPengine.h"
#include "OPMconvert.h"

struct ExporterState {
	OPexporter exporter;

	OPcamFreeFlight camera;

	void Init();
	OPint Update(OPtimer* timer);
	void Render(OPfloat delta);
	OPint Exit();

	bool _loadMeshFromFile(const OPchar* filename);
	void _drop(OPuint count, const OPchar** filenames);
};

extern OPgameState GS_EXAMPLE;
