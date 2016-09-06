#pragma once

#include "./OPengine.h"

void ExportOPM(
	const OPchar* path, 
	OPchar* output, 
	f32 scale, 
	OPmodel* model,
	bool featureNormals,
	bool featureUVs,
	bool featureTangents,
	bool featureBiTangents,
	bool featureColors,
	bool featureBones,
	bool exportSkeleton,
	bool exportAnimations,
	ui32 splitCount,
	ui32* splitStart,
	ui32* splitEnd,
	OPchar** splitName
	);

ui32 GetAvailableTracks(const OPchar* path, OPchar** buff, double* durations, ui32 max);