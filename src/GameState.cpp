#include "./GameState.h"
#include "Main.h"
#include "Utils.h"
#include "OPMconvert.h"
#include "OPimgui.h"

void DropCallback(OPuint count, const OPchar** filenames);
void windowDump(OPstring* out);

bool ExporterState::_loadMeshFromFile(const OPchar* filename) {
	const OPchar* ext = NULL;


	OPmodel* model = (OPmodel*)OPCMAN.LoadFromFile(filename);
	if (model == NULL) return false;

	// Load up an fbx with assimp
	exporter.Init(filename, model);

	bounds = OPboundingBox3D();

	scene.Remove(entity);
	entity = scene.Add(model, OPrendererEntityDesc(false, true, true, true));

	// Setup the materials per mesh in the model
	for (ui32 i = 0; i < model->meshCount; i++) {
		if (model->meshes[i].materialDesc == NULL) continue;

		OPtexture* result;


		if (model->meshes[i].boundingBox.min.x < bounds.min.x) bounds.min.x = model->meshes[i].boundingBox.min.x;
		if (model->meshes[i].boundingBox.min.y < bounds.min.y) bounds.min.y = model->meshes[i].boundingBox.min.y;
		if (model->meshes[i].boundingBox.min.z < bounds.min.z) bounds.min.z = model->meshes[i].boundingBox.min.z;
		if (model->meshes[i].boundingBox.max.x > bounds.max.x) bounds.max.x = model->meshes[i].boundingBox.max.x;
		if (model->meshes[i].boundingBox.max.y > bounds.max.y) bounds.max.y = model->meshes[i].boundingBox.max.y;
		if (model->meshes[i].boundingBox.max.z > bounds.max.z) bounds.max.z = model->meshes[i].boundingBox.max.z;

		// Diffuse
		result = LoadTexture(filename, model->meshes[i].materialDesc->diffuse);
		if (result == NULL) {
			entity->SetAlbedoMap("Default_Albedo.png", i);
		}
		else {
			entity->SetAlbedoMap(result, i);
		}

		//// Normals
		//result = LoadTexture(filename, model.meshes[i].materialDesc->normals);
		//if (result == NULL) {
		//	materialPBRInstancesArr[i]->SetNormalMap("Default_Normals.png");
		//}
		//else {
		//	materialPBRInstancesArr[i]->SetNormalMap(result);
		//}

		//// Specular
		//result = LoadTexture(filename, model.meshes[i].materialDesc->specular);
		//if (result == NULL) {
		//	materialPBRInstancesArr[i]->SetSpecularMap("Default_Specular.png");
		//}
		//else {
		//	materialPBRInstancesArr[i]->SetSpecularMap(result);
		//}
	}

	if (outputFilename != NULL) {
		delete outputFilename;
	}
	outputFilename = GetFilenameOPM(filename);

	if (outputAbsolutePath != NULL) {
		delete outputAbsolutePath;
	}
	outputAbsolutePath = GetAbsolutePathOPM(filename);

	return true;
}

void ExporterState::Init() {
	OPimguiInit(OPRENDERER_ACTIVE->OPWINDOW_ACTIVE, true);
	ImGui::GetStyle().WindowRounding = 0.0f;

	mainWindow.SetDropCallback(DropCallback);

	fullForwardRenderer = OPrendererFullForward::Create();
	scene.Init(&fullForwardRenderer->rendererRoot, 1000, 50);

	camera.Init(1.0f, 1.0f, OPvec3(5, 5, 5));
	camera.Rotation.x = -0.644917190;
	camera.Rotation.y = -0.785485148;
	camera.Rotation.z = 0.955316544;
	camera.Camera.pos = OPvec3(-5, 5, 5);
	camera.Update();
	scene.camera = &camera.Camera;

	OPfloat shadowCameraSize = 16.0f;
	fullForwardRenderer->shadowCamera.SetOrtho(OPvec3(-6, 6, 1), OPVEC3_ZERO, OPVEC3_UP, 0.1f, 15.0f, -shadowCameraSize, shadowCameraSize, -shadowCameraSize, shadowCameraSize);
	//fullForwardRenderer->shadowCamera.SetOrtho(OPvec3(2), OPVEC3_ZERO, 10.0f);

	OPmodel* model = (OPmodel*)OPCMAN.LoadGet("box.opm");
	entity = scene.Add(model, OPrendererEntityDesc(false, true, true, false));
	bounds = OPboundingBox3D(OPvec3(-0.5), OPvec3(0.5));
	entity->SetAlbedoMap("Default_Albedo.png");
	entity->world.SetScl(Scale)->Translate(0, (bounds.max.y - bounds.min.y) / 2.0f, 0);

	OPmodel* ground = OPquadCreateZPlane(10.0f, 10.0f);
	OPrendererEntity* groundEnt = scene.Add(model, OPrendererEntityDesc(false, true, true, false));
	groundEnt->SetAlbedoMap("Default_Normals.png");
	groundEnt->world.SetScl(10, 0.1, 10)->Translate(0, -0.05, 0);
}

OPint ExporterState::Update(OPtimer* timer) {
	camera.Update(timer);
	scene.Update(timer);
	entity->world.SetScl(Scale)->Translate(0, -1.0 * Scale * bounds.min.y, 0);
	return false;
}

void ExporterState::Render(OPfloat delta) {

	OPrenderClear(0.2f, 0.2f, 0.2f, 1);
	OPrenderCullMode(OPcullFace::BACK);
	OPrenderCull(false);

	scene.Render(delta);

	// Happens here so that we can get a rendered image of the model too
	_processDroppedFiles();

    if(outputAbsolutePath != NULL && getThumbnail) {
        windowDump(outputAbsolutePath);
        getThumbnail = false;
    }


	// Render the GUI
	OPimguiNewFrame();

    bool always = true;
	{ // Render Settings Window
		if (outputFilename != NULL) {

			ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiSetCond_::ImGuiSetCond_FirstUseEver);
			ImGui::Begin("Settings", &always, ImVec2(250, 400), -1.0F, ImGuiWindowFlags_NoResize);

			ImGui::Checkbox("Normals", &exporter.Feature_Normals);
			ImGui::Checkbox("UVs", &exporter.Feature_UVs);
			ImGui::Checkbox("Tangents", &exporter.Feature_Tangents);
			ImGui::Checkbox("BiTangents", &exporter.Feature_BiTangents);
			ImGui::Checkbox("Colors", &exporter.Feature_Colors);
			ImGui::Checkbox("Bones", &exporter.Feature_Bones);
			ImGui::Checkbox("Skeleton", &exporter.Export_Skeleton);
			ImGui::Checkbox("Animations", &exporter.Export_Animations);

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			ImGui::Text("Scale");
			if (ImGui::Button("Default")) {
				Scale = 1.0f;
			}
			if (ImGui::Button("CM -> Meters")) {
				Scale = 0.01f;
			}
			ImGui::SliderFloat("Scale", &Scale, 0.001f, 4.0f);

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			ImGui::Checkbox("Auto-Export on drop", &autoExport);

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			if (ImGui::Button("Snap Thumbnail")) {
				getThumbnail = true;
			}
			ImGui::End();
		}
	}

	{ // Render Export Window
		if (outputFilename != NULL) {
			ImGui::SetNextWindowPos(ImVec2(0, (OPRENDERER_ACTIVE->OPWINDOW_ACTIVE->Height / OPRENDERER_ACTIVE->OPWINDOW_ACTIVE->HeightScaled) - 41));
			ImGui::Begin("OverlayExport", &always, ImVec2((OPRENDERER_ACTIVE->OPWINDOW_ACTIVE->Width / OPRENDERER_ACTIVE->OPWINDOW_ACTIVE->WidthScaled), 41), 0.3f, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);

			char buffer[255];
			sprintf(buffer, "Export %s", outputFilename->C_Str());

			if (ImGui::Button(buffer, ImVec2((OPRENDERER_ACTIVE->OPWINDOW_ACTIVE->Width / OPRENDERER_ACTIVE->OPWINDOW_ACTIVE->WidthScaled), 25))) {
				exporter.Export(outputAbsolutePath->C_Str());
			}

			ImGui::End();
		}
	}


	{ // Render Animations Window
		//ImGui::ShowTestWindow();

		//if (totalAnimationTracks > 0) {

		//	ImGui::SetNextWindowPos(ImVec2(10, 310));
		//	ImGui::Begin("OverlayAnimations", &alwaysShow, ImVec2(200, 20 * totalAnimationTracks), 0.3f, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);

		//	for (ui32 i = 0; i < totalAnimationTracks; i++) {
		//		ImGui::Text("%s : %d frames", AnimationTracks[i], (int)AnimationDurations[i]);
		//	}
		//	ImGui::End();

		//	if (SplittersIndex > 0) {
		//		ImGui::SetNextWindowPos(ImVec2(10, 330 + 20 * totalAnimationTracks));
		//		ImGui::Begin("OverlaySplitters", &alwaysShow, ImVec2(200, 20 * SplittersIndex), 0.3f, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);

		//		for (ui32 i = 0; i < SplittersIndex; i++) {
		//			ImGui::Text("%s : %d - %d", SplittersName[i], SplittersStart[i], SplittersEnd[i]);
		//		}
		//		ImGui::End();
		//	}
		//}
	}

	ImGui::Render();

	OPrenderPresent();
}

OPint ExporterState::Exit() {
	return 0;
}

void ExporterState::_processTexture(const OPchar* filename) {

	OPtexture* tex = (OPtexture*)OPCMAN.LoadFromFile(filename);
	if (tex == NULL) return;


    #ifdef OPIFEX_WINDOWS
       const OPchar* ext = strrchr(filename, '\\');
    #else
       const OPchar* ext = strrchr(filename, '/');
    #endif
    OPchar* justTexName = OPstringCopy(&ext[1]);


	for (ui32 i = 0; i < entity->model->meshCount; i++) {
        entity->SetAlbedoMap(tex, i);
        entity->model->meshes[i].materialDesc->diffuse = justTexName;
    }
	//materialInstance->SetAlbedoMap(tex);

	//if (materialInstancesArr != NULL) {
	//	// Set the current materials albedo texture
	//	materialPBRInstancesArr[texInd]->SetAlbedoMap(tex);
	//}

	// Set Meta

	//metaData[0].dataSize = strlen(justTexName) * sizeof(OPchar);
	//metaData[0].data = justTexName;
}

void ExporterState::_processAnimations(const OPchar* filename) {

	//SplittersIndex = 0;
	//// Assume it's an animation splitter file
	//ifstream myFile(filename);
	//OPchar str[255];
	//if (myFile.is_open()) {
	//	while (!myFile.eof()) {
	//		myFile.getline(str, 255);
	//		// Parse out each line
	//		if (OPstringCount(str, ':') != 2) continue;
	//		// Must have exactly 2 :'s

	//		ui32 colonPos = OPstringFirst(str, ':');
	//		OPchar* start = OPstringSub(str, 0, colonPos);
	//		OPchar* secondPart = &str[colonPos + 1];
	//		colonPos = OPstringFirst(&str[1], ':');
	//		OPchar* end = OPstringSub(secondPart, 0, colonPos + 1);
	//		OPchar* name = &secondPart[colonPos + 2];

	//		SplittersStart[SplittersIndex] = atoi(start);
	//		SplittersEnd[SplittersIndex] = atoi(end);
	//		SplittersName[SplittersIndex] = OPstringCopy(name);
	//		SplittersIndex++;

	//	}
	//}
}

void ExporterState::_processModel(const OPchar* filename) {
	if (!_loadMeshFromFile(filename)) return;

	if (OPvec3Len(bounds.max - bounds.min) > 80) {
		Scale = 0.01f;
	}

	if (autoExport) {
		exporter.Export(outputAbsolutePath->C_Str());
	}
}

#ifdef OPIFEX_OPENGL_ES_2
#ifdef OPIFEX_IOS
#include <OpenGLES/ES2/gl.h>
#else
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#endif
#else
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#endif


void windowDump(OPstring* out) {
	int i, j;
	FILE *fptr;
	static int counter = 0; /* This supports animation sequences */
	char fname[32];
	ui8 *image, *image2;

	ui32 width = OPRENDERER_ACTIVE->OPWINDOW_ACTIVE->Width;
	ui32 height = OPRENDERER_ACTIVE->OPWINDOW_ACTIVE->Height;

	/* Allocate our buffer for the image */
	if ((image = (ui8*)OPalloc(3 * width*height * sizeof(char))) == NULL) {
		OPlogErr("Failed to allocate memory for image");
		return;
	}

	if ((image2 = (ui8*)OPalloc(3 * width*height * sizeof(char))) == NULL) {
		OPlogErr("Failed to allocate memory for image");
		return;
	}

	glPixelStorei(GL_PACK_ALIGNMENT, 1);


	glReadBuffer(GL_BACK_LEFT);
	glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, image);

	// Flip the image back around

	for (j = height - 1; j >= 0; j--) {
		for (i = 0; i<width; i++) {
			image2[3 * (height - j)*width + 3 * i + 0] = image[3 * j*width + 3 * i + 0];
			image2[3 * (height - j)*width + 3 * i + 1] = image[3 * j*width + 3 * i + 1];
			image2[3 * (height - j)*width + 3 * i + 2] = image[3 * j*width + 3 * i + 2];
		}
	}

	OPstring* output = out->Copy();
	output->Add(".png");
	OPimagePNGCreate24(image2, width, height, output->C_Str());
	delete output;

}

void ExporterState::_processDroppedFiles() {

	if (currentFile > dropCount) {
		return;
	}

	if (currentFile <= dropCount && currentFile != 0) {
		OPuint prevFile = currentFile - 1;
		// Grab a snapshot
		windowDump(outputAbsolutePath);
	}

	if (dropCount == 0 || currentFile == dropCount) {
		currentFile++;
		return;
	}

	camera.Rotation.x = -0.644917190;
	camera.Rotation.y = -0.785485148;
	camera.Rotation.z = 0.955316544;
	camera.Camera.pos = OPvec3(-5, 5, 5);
	camera.Update();

	//camera.Init(1.0f, 1.0f, OPvec3(5, 5, 5));

	// We haven't maxed out yet, so if this is the 2nd model,

	const OPchar* ext = NULL;
	ext = strrchr(dropFilenames[currentFile].C_Str(), '.');

	if (IsImageFile(ext)) { // Process file as a texture
		_processTexture(dropFilenames[currentFile].C_Str());
	}
	else if (IsAnimationFile(ext)) { // Process as animation file
		_processAnimations(dropFilenames[currentFile].C_Str());
	}
	else if(IsModelFile(ext)) { // Process as model file
		if (OPstringEquals(ext, ".opm")) { // Doesn't go through Assimp for OPM files
			_loadMeshFromFile(dropFilenames[currentFile].C_Str());
			Scale = 1.0f;
		} else {
			_processModel(dropFilenames[currentFile].C_Str());
		}
	}

	currentFile++;
}

void ExporterState::_drop(OPuint count, const OPchar** filenames) {
	currentFile = 0;
	dropCount = count;
	if (dropFilenames != NULL) {
		OPfree(dropFilenames);
	}
	dropFilenames = OPALLOC(OPstring, count);
	for (ui32 i = 0; i < count; i++) {
		dropFilenames[i].Init(filenames[i]);
	}
}



// Hookups for the game state
ExporterState exporterState;
void ExampleStateInit(OPgameState* last) { exporterState.Init(); }
OPint ExampleStateUpdate(OPtimer* time) { return exporterState.Update(time); }
void ExampleStateRender(OPfloat delta) { exporterState.Render(delta); }
OPint ExampleStateExit(OPgameState* next) { return exporterState.Exit(); }

void DropCallback(OPuint count, const OPchar** filenames) {
	exporterState._drop(count, filenames);
}

OPgameState GS_EXAMPLE = {
	ExampleStateInit,
	ExampleStateUpdate,
	ExampleStateRender,
	ExampleStateExit
};
