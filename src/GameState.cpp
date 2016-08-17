#include "./GameState.h"

void ExampleStateInit(OPgameState* last) {
}


OPint ExampleStateUpdate(OPtimer* time) {
	return false;
}

void ExampleStateRender(OPfloat delta) {
	OPrenderClear(0, 0, 0, 1);
	OPrenderPresent();
}

OPint ExampleStateExit(OPgameState* next) {
	return 0;
}


OPgameState GS_EXAMPLE = {
	ExampleStateInit,
	ExampleStateUpdate,
	ExampleStateRender,
	ExampleStateExit
};
