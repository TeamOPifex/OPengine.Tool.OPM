#include "./GameState.h"
#include "Main.h"
#include "Utils.h"
#include "OPMconvert.h"
#include "OPimgui.h"

#include "./Human/include/Platform/opengl/OPtextureAPIGL.h"

void DropCallback(OPuint count, const OPchar** filenames);
void windowDump(OPstring* out);

OPtexture* result;
OPskeleton* activeSkeleton = NULL;
OPskeletonAnimationResult animations = { NULL, 0 };
OPskeletonAnimation* activeAnimation = NULL;

bool ItemGetter(void* source, int pos, const char** result) {
	*result = animations.AnimationNames[pos];
	return true;
}

bool ExporterState::_loadOPMFromFile(const OPchar* filename) {
	OPstring filePath(filename);

	OPmodel* model = (OPmodel*)OPCMAN.LoadFromFile(filePath.C_Str());
	if (model == NULL) return false;

	bounds = OPboundingBox3D();
	scene.Remove(entity);

	activeSkeleton = NULL;

	filePath.Add(".skel");
	if (OPfile::Exists(filePath.C_Str())) {
		OPstream* stream = OPfile::ReadFromFile(filePath.C_Str());
		OPskeleton* skeleton;
		if (OPloaderOPskeletonLoad(stream, &skeleton)) {
			activeSkeleton = skeleton;
			// Get Animations
			//animations = exporter.LoadAnimations(stream);
			//if (animations.AnimationsCount > 0) {
			//	activeAnimation = animations.Animations[0];
			//}
			//else {
			//	activeAnimation = NULL;
			//}
		}
	}

	if (activeSkeleton != NULL) {
		entity = scene.Add(model, activeSkeleton, OPrendererEntityDesc(true, true, true, true));
	}
	else {
		entity = scene.Add(model, OPrendererEntityDesc(false, true, true, true));
	}
	
	// Setup the materials per mesh in the model
	for (ui32 i = 0; i < model->meshCount; i++) {
		if (model->meshes[i].materialDesc == NULL) continue;

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

bool ExporterState::_loadMeshFromFile(const OPchar* filename) {
	const OPchar* ext = NULL;


	//OPmodel* model = (OPmodel*)OPCMAN.LoadFromFile(filename);
	//if (model == NULL) return false;

	// Load up an fbx with assimp
	exporter.Init(filename, NULL);

	if (exporter.scene == NULL) {
		return false;
	}
	
	bounds = OPboundingBox3D();

	scene.Remove(entity);

	activeSkeleton = NULL;

	if (exporter.HasAnimations) {
		//OPstream* stream = OPfile::ReadFromFile(filename);
		// Get Skeleton
		OPskeleton* skeleton = exporter.LoadSkeleton();
		activeSkeleton = skeleton;

		// Get Animations
		if (splitterIndex == 0) {
			animations = exporter.LoadAnimations();
		}
		else {
			animations = exporter.LoadAnimations(splitters, splitterIndex);
		}

		if (animations.AnimationsCount > 0) {
			activeAnimation = animations.Animations[0];
		}
		else {
			activeAnimation = NULL;
		}
	}

	OPmodel* model = exporter.existingModel;

	if (activeSkeleton != NULL) {
		entity = scene.Add(model, activeSkeleton, OPrendererEntityDesc(true, true, true, true));
	}
	else {
		entity = scene.Add(model, OPrendererEntityDesc(false, true, true, true));
	}

	// Setup the materials per mesh in the model
	for (ui32 i = 0; i < model->meshCount; i++) {
		if (model->meshes[i].materialDesc == NULL) continue;
		

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

	if (useAnimation && activeAnimation != NULL && activeSkeleton != NULL) {
		OPskeletonAnimationUpdate(activeAnimation, timer);
		OPskeletonAnimationApply(activeAnimation, activeSkeleton);
	}
	if (activeSkeleton != NULL) {
		OPskeletonUpdate(activeSkeleton);
	}
	return false;
}

int ind = 0;
char* items = "test\0two\0three";

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
	bool show_app_metrics;

	{ // Render Settings Window
		if (outputFilename != NULL) {

			ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiSetCond_::ImGuiSetCond_FirstUseEver);
			ImGui::Begin("Settings", &always, ImVec2(250, 430), -1.0F, ImGuiWindowFlags_NoResize);

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

			if (ImGui::Checkbox("Show Animations", &useAnimation)) {
				if (activeSkeleton != NULL) {
					activeSkeleton->Reset();
				}
			}

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
				if (splitterIndex == 0) {
					exporter.Export(outputAbsolutePath->C_Str());
				}
				else {
					exporter.Export(splitters, splitterIndex, outputAbsolutePath->C_Str());
				}
			}

			ImGui::End();
		}
	}

	ui32 windowWidth = (OPRENDERER_ACTIVE->OPWINDOW_ACTIVE->Width / OPRENDERER_ACTIVE->OPWINDOW_ACTIVE->WidthScaled);

	{ // Render Select Window
		ImGui::SetNextWindowPos(ImVec2(0, 0));
		ImGui::Begin("SelectorMeshes", &always, ImVec2(windowWidth / 2, 36), 0.3f, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
		if (outputFilename != NULL) {
			ImGui::PushItemWidth(-1.0f);
			ImGui::Combo("Meshes", &ind, items);
		}
		ImGui::End();

		ImGui::SetNextWindowPos(ImVec2(windowWidth / 2, 0));
		ImGui::Begin("SelectorAnimations", &always, ImVec2(windowWidth / 2, 36), 0.3f, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
		if (outputFilename != NULL) {
			ImGui::PushItemWidth(-1.0f);
			if (ImGui::Combo("Animations", &ind, ItemGetter, animations.AnimationNames, animations.AnimationsCount)) {
				activeAnimation = animations.Animations[ind];
			}
		}
		ImGui::End();
	}


	//ImGui::ShowTestWindow();

	if (result != NULL) {
		ImGui::SetNextWindowPos(ImVec2(windowWidth - 128 - 16 - 4, 40));
		ui32 childWindowWidth = 128 + 16;
		ImGui::Begin("CurrentTextures", &always, ImVec2(childWindowWidth, 128 + 16 + 16), 0.0f, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);

		OPtextureGL* glTex = (OPtextureGL*)result->internalPtr;
		ImTextureID tex = (void*)glTex->Handle;
		ImGui::Image(tex, ImVec2(128, 128), ImVec2(0, 0), ImVec2(1, 1), ImColor(255, 255, 255, 255), ImColor(255, 255, 255, 128));

		ImVec2 textSize = ImGui::CalcTextSize("Diffuse");
		ImGui::Text("");
		ui32 offset = (childWindowWidth / 2) - (textSize.x / 2);
		ImGui::SameLine(offset);
		ImGui::Text("Diffuse");

		ImGui::End();

		//ImGui::GetStyle().Colors[ImGuiCol_WindowBg].w = 0.8;
	}

	{
		//if (activeSkeleton != NULL) {
		//	ImGui::Begin("CurrentSkeleton", &always, ImVec2(200, 400), 0.0f);

		//	for (ui32 i = 0; i < activeSkeleton->hierarchyCount; i++) {
		//		if (activeSkeleton->hierarchy[i] >= 0) {
		//			ImGui::Text("%s ( %d ) : %s ( %d )", activeSkeleton->jointNames[i], i, activeSkeleton->jointNames[activeSkeleton->hierarchy[i]], activeSkeleton->hierarchy[i]);
		//		}
		//		else {
		//			ImGui::Text("%s ( %d ) : ROOT", activeSkeleton->jointNames[i], i);
		//		}
		//	}

		//	ImGui::End();
		//}
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

	result = tex;


	OPstring texName(filename);
	OPint pos = texName.IndexOfLast('\\');
	if (pos != -1) {
		texName.Init(&texName._data[pos + 1]);
	}

	pos = texName.IndexOfLast('/');
	if (pos != -1) {
		texName.Init(&texName._data[pos + 1]);
	}


	for (ui32 i = 0; i < entity->model->meshCount; i++) {
        entity->SetAlbedoMap(tex, i);
        entity->model->meshes[i].materialDesc->diffuse = OPstringCopy(texName.C_Str());
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
	if (splitterIndex > 0) {
		for (ui32 i = 0; i < splitterIndex; i++) {
			OPfree(splitters[i].Name);
		}
		splitterIndex = 0;
	}

	//SplittersIndex = 0;
	// Assume it's an animation splitter file
	ifstream myFile(filename);
	OPchar str[255];
	if (myFile.is_open()) {
		while (!myFile.eof()) {
			myFile.getline(str, 255);
			// Parse out each line
			if (OPstringCount(str, ':') != 2) continue;
			// Must have exactly 2 :'s

			ui32 colonPos = OPstringFirst(str, ':');
			OPchar* start = OPstringSub(str, 0, colonPos);
			OPchar* secondPart = &str[colonPos + 1];
			colonPos = OPstringFirst(&str[1], ':');
			OPchar* end = OPstringSub(secondPart, 0, colonPos + 1);
			OPchar* name = &secondPart[colonPos + 2];

			splitters[splitterIndex].Start = atoi(start);
			splitters[splitterIndex].End = atoi(end);
			splitters[splitterIndex].Name = OPstringCopy(name);
			splitterIndex++;
		}

		if (splitterIndex > 0 && activeAnimation != NULL) {
			// Reload animations
			animations = exporter.LoadAnimations(splitters, splitterIndex);
			if (animations.AnimationsCount > 0) {
				activeAnimation = animations.Animations[0];
			}
			else {
				activeAnimation = NULL;
			}
		}
	}
}

void ExporterState::_processModel(const OPchar* filename) {
	if (!_loadMeshFromFile(filename)) return;

	//if (OPvec3Len(bounds.max - bounds.min) > 80) {
	//	Scale = 0.01f;
	//}

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
	else if (IsModelFile(ext)) { // Process as model file

		camera.Rotation.x = -0.644917190;
		camera.Rotation.y = -0.785485148;
		camera.Rotation.z = 0.955316544;
		camera.Camera.pos = OPvec3(-5, 5, 5);
		camera.Update();

		if (OPstringEquals(ext, ".opm")) { // Doesn't go through Assimp for OPM files
			_loadOPMFromFile(dropFilenames[currentFile].C_Str());
			Scale = 1.0f;

			OPstring dir(dropFilenames[currentFile].C_Str());
			OPstring filename(dropFilenames[currentFile].C_Str());

			RemoveFilename(&dir);
			RemoveDirectory(&filename);

			ui32 fileCount = 0;
			OPchar** files = OPfile::GetDirectoryFiles(dir.C_Str(), &fileCount);

			ui32 totalAnimCount = 0;
			for (ui32 i = 0; i < fileCount; i++) {
				OPstring f(files[i]);
				bool startWith = f.StartsWith(filename.C_Str());
				bool endsWith = f.EndsWith(".anim");

				if (startWith && endsWith) {
					totalAnimCount++;
				}
			}

			animations.AnimationNames = OPALLOC(OPchar*, totalAnimCount);
			animations.Animations = OPALLOC(OPskeletonAnimation*, totalAnimCount);
			ui32 ind = 0;
			for (ui32 i = 0; i < fileCount; i++) {
				OPstring f(files[i]);
				bool startWith = f.StartsWith(filename.C_Str());
				bool endsWith = f.EndsWith(".anim");

				if (startWith && endsWith) {
					f.Resize(f._len - 5);

					OPlogErr("Anim File: %s", files[i]);

					OPstring absPath(dir.C_Str());
					absPath.Add(files[i]);

					animations.Animations[ind] = (OPskeletonAnimation*)OPCMAN.LoadFromFile(absPath.C_Str());
					animations.AnimationNames[ind] = OPstringCopy(&f._data[filename._len + 1]);
					ind++;
				}
			}

			animations.AnimationsCount = totalAnimCount;

			if (totalAnimCount > 0) {
				activeAnimation = animations.Animations[0];
			}

		}
		else {
			_processModel(dropFilenames[currentFile].C_Str());
		}
	}
	else if (IsSkeltonFile(ext)) { // Process as model file

		OPskeleton* skel = (OPskeleton*)OPCMAN.LoadFromFile(dropFilenames[currentFile].C_Str());
		if (skel != NULL) {
			activeSkeleton = skel;
			activeAnimation = NULL;
		}
	}
	else if (IsSkeletonAnimationFile(ext) && activeSkeleton != NULL) { // Process as model file
		activeAnimation = (OPskeletonAnimation*)OPCMAN.LoadFromFile(dropFilenames[currentFile].C_Str());
		
		//if (anim != NULL && activeSkeleton->hierarchyCount == anim->BoneCount) {

		//	OPlogErr("TEST");
		//	activeAnimation == anim;
		//	OPlogErr("%p", activeAnimation);

		//}
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
