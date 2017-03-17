#pragma once

#include "./OPengine.h"

class WindowSnapshot {
private:
	ui8 *image, *image2;
public:
	WindowSnapshot();
	void Snapshot(OPstring* out);
};