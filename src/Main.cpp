//////////////////////////////////////
// Application Entry Point
//////////////////////////////////////
#include "Main.h"
#include "GameState.h"
#include "./OPassimp.h"

//////////////////////////////////////
// Application Methods
//////////////////////////////////////

OPwindow mainWindow;
void ApplicationInit() {
	OPCMAN.Init(OPIFEX_ASSETS);
	OPloadersAddDefault();
	OPassimpAddLoaders();

	OPassetLoader loaderFBX = {
		".fbx",
		"Models/",
		sizeof(OPmesh),
		(OPint(*)(OPstream*, void**))OPassimpLoadMem,
		(OPint(*)(void*))OPassimpUnLoad,
		NULL
	};
	OPCMAN.AddLoader(&loaderFBX);

	OPrenderSetup();

	OPwindowSystemInit();
	mainWindow.Init(NULL, OPwindowParameters("Main Window", false, 1280, 720));

	OPrenderInit(&mainWindow);

	OPgameState::Change(&GS_EXAMPLE);
}

OPint ApplicationUpdate(OPtimer* timer) {
	if (mainWindow.Update()) {
		// Window received an exit request
		return 1;
	}

	OPinputSystemUpdate(timer);
	if (OPKEYBOARD.WasPressed(OPkeyboardKey::ESCAPE)) return 1;

	return ActiveState->Update(timer);
}

void ApplicationRender(OPfloat delta) {
	ActiveState->Render(delta);
}

void ApplicationDestroy() {

}

void ApplicationSetup() {
	OPinitialize = ApplicationInit;
	OPupdate = ApplicationUpdate;
	OPrender = ApplicationRender;
	OPdestroy = ApplicationDestroy;
}

#include "OPMconvert.h"

//////////////////////////////////////
// Application Entry Point
//////////////////////////////////////
OP_MAIN_START

	OPLOGLEVEL = (ui32)OPlogLevel::TIMING;
	OPlog("Starting up OPifex Engine");

	if (argc == 2) {
		OPlog(args[0]);
		OPlog(args[1]);
		OPchar* filename = OPstringCopy(args[1]);
		i32 ind = -1;
		for (ui32 i = 0; i < strlen(filename); i++) {
			if (filename[i] == '.') {
				ind = i;
			}
		}

		if (ind > -1) {
			filename[ind] = NULL;
		}

		OPchar* output = OPstringCreateMerged(filename, ".opm");

		ExportOPM(args[1], output, 1.0f, NULL, true, true, true, true, false, false, false, false, 0, NULL, NULL, NULL);
	} else {
		ApplicationSetup();
		OP_MAIN_RUN
		//OP_MAIN_RUN_STEPPED
	}
OP_MAIN_END
