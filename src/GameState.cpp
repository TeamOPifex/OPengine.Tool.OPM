#include "./GameState.h"
#include "Main.h"
#include "Utils.h"
#include "OPMconvert.h"
#include "OPimgui.h"

#include "./Human/include/Platform/opengl/OPtextureAPIGL.h"

void DropCallback(OPuint count, const OPchar** filenames);

bool ItemGetter(void* source, int pos, const char** result) {
	OPchar** animationNames = (OPchar**)source;
	*result = animationNames[pos]; //GS_EXAMPLE.modelViewer.animations.AnimationNames
	return true;
}

bool MeshNameGetter(void* source, int pos, const char** result) {
	OPmodel* model = (OPmodel*)source;
	if (pos == 0) {
		*result = "All";
	}
	else {
		if (model->meshes[pos - 1].name == NULL) {
			*result = "Mesh";
		}
		else {
			*result = model->meshes[pos - 1].name;
		}
	}
	return true;
}

// Start up the exporter
void ExporterState::Init(OPgameState* state) {
	OPimguiInit(OPRENDERER_ACTIVE->OPWINDOW_ACTIVE, true);
	ImGui::GetStyle().WindowRounding = 0.0f;

	mainWindow.SetDropCallback(DropCallback);

	camera.Init(1.0f, 1.0f, OPvec3(5, 5, 5));
	camera.Rotation.x = -0.644917190;
	camera.Rotation.y = -0.785485148;
	camera.Rotation.z = 0.955316544;
	camera.Camera.pos = OPvec3(-5, 5, 5);

	OPfloat shadowCameraSize = 16.0f;
	shadowCam.SetOrtho(OPvec3(-6, 6, 1), OPVEC3_ZERO, OPVEC3_UP, 0.1f, 15.0f, -shadowCameraSize, shadowCameraSize, -shadowCameraSize, shadowCameraSize);
	
	scene.Init(&fullForwardRenderer, 1000, 50);
	scene.SetCamera(&camera.Camera);
	scene.SetShadowCamera(&shadowCam);

	modelViewer = ModelViewer(&scene, &exporter);
	windowSnapshot = new WindowSnapshot();

	OPmodel* ground = OPquadCreateZPlane(1.0f, 1.0f, OPvec2(0,0), OPvec2(16, 16), (ui32)OPattributes::POSITION | (ui32)OPattributes::NORMAL | (ui32)OPattributes::TANGENT | (ui32)OPattributes::BITANGENT | (ui32)OPattributes::UV);
	OPrendererEntity* groundEnt = scene.Add(ground, OPrendererEntityDesc(false, true, true, false));
	groundEnt->SetAlbedoMap("Tile2.png");
	groundEnt->world.SetScl(10, 0.1, 10)->Translate(0, -0.05, 0);

}

OPint ExporterState::Update(OPtimer* timer) {
	// This stopped working right...
	//if (OPRENDERER_ACTIVE->OPWINDOW_ACTIVE->HasFocus())
		camera.Update(timer);

	scene.Update(timer);
	modelViewer.entity->world.SetScl(modelViewer.Scale)->Translate(0, -1.0 * modelViewer.Scale * modelViewer.bounds.min.y, 0);

	if (useAnimation && modelViewer.activeAnimation != NULL && modelViewer.activeSkeleton != NULL) {
		modelViewer.activeAnimation->Update(timer);
		modelViewer.activeAnimation->Apply(modelViewer.activeSkeleton);
	modelViewer.activeSkeleton->Update();
	} else if (modelViewer.activeSkeleton != NULL) {
		modelViewer.activeSkeleton->Reset();
		modelViewer.activeSkeleton->Update();
	}

	return false;
}

void ExporterState::renderGUI() {
	// Render the GUI
	OPimguiNewFrame();

	renderGUIMenu();
	renderGUISettings();
	renderGUISkeleton();
	renderGUIAddAnimation();
	renderGUIExport();
	renderGUIMeshSelect();
	renderGUITextureSelect();

	ImGui::Render();
}

void ExporterState::renderGUIMenu() {
	ImGui::BeginMainMenuBar();
	if (ImGui::BeginMenu("File"))
	{
		if (ImGui::MenuItem("New")) {

		}
		if (ImGui::MenuItem("Load...")) {

		}
		ImGui::Separator();
		if (ImGui::MenuItem("Save...")) {

		}
		ImGui::Separator();
		if (ImGui::MenuItem("Exit", "Escape")) {
			exit(0);
		}
		ImGui::EndMenu();
	}
	ImGui::EndMainMenuBar();
}

void ExporterState::renderGUISettings() {
	bool always = true;

	ImGui::SetNextWindowPos(ImVec2(10, 30), ImGuiSetCond_::ImGuiSetCond_FirstUseEver);
	ImGui::Begin("Settings", &always, ImVec2(250, 475), -1.0F, ImGuiWindowFlags_NoResize);

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
	ImGui::Checkbox("Auto-Scale on drop", &autoScale);
	if (ImGui::Button("Default")) {
		modelViewer.Scale = 1.0f;
	}
	ImGui::SameLine(0);
	if (ImGui::Button("CM -> Meters")) {
		modelViewer.Scale = 0.01f;
	}
	ImGui::InputFloat("Scale", &modelViewer.Scale, 0.1, 1.0, 6);
	//ImGui::SliderFloat("Scale", &Scale, 0.001f, 100.0f);

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	if (ImGui::Checkbox("Show Animations", &useAnimation)) {
		if (modelViewer.activeSkeleton != NULL) {
			modelViewer.activeSkeleton->Reset();
		}
	}

	if (ImGui::Checkbox("Split anim name", &modelViewer.splitFileNameForAnim)) {

	}

	if (ImGui::Button("Show Add Anims")) {
		showAddAnimation = !showAddAnimation;
	}

	if (ImGui::Button("Show Skeleton")) {
		showSkeleton = !showSkeleton;
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

void ExporterState::renderGUISkeleton() {
	if (!showSkeleton || modelViewer.activeSkeleton == NULL) return;

	ImGui::Begin("Skeleton");
	OPimguiDebug(modelViewer.activeSkeleton);
	ImGui::End();
}

void ExporterState::renderGUIAddAnimation() {
	if (!showAddAnimation) return;

	bool always = true;
	ImGui::SetNextWindowPos(ImVec2(10, 30), ImGuiSetCond_::ImGuiSetCond_FirstUseEver);
	ImGui::Begin("Add Animation", &always, ImVec2(500, 100), -1.0F, ImGuiWindowFlags_NoResize);

	if (ImGui::Button("Clear Anims")) {
		useAnimation = false;
		if (modelViewer.activeSkeleton != NULL) {
			modelViewer.activeSkeleton->Reset();
		}
	}

	ImGui::InputInt("Start", &AnimStart);
	ImGui::InputInt("End", &AnimEnd);
	ImGui::InputText("Name", AnimName, 100);

	if (ImGui::Button("Add Anim")) {
		modelViewer.splitterIndex = 1;
		modelViewer.splitters[0].Start = AnimStart;
		modelViewer.splitters[0].End = AnimEnd;
		modelViewer.splitters[0].Name = AnimName;
		modelViewer.animations = exporter.LoadAnimations(modelViewer.splitters, modelViewer.splitterIndex);
		if (modelViewer.animations.AnimationsCount > 0) {
			modelViewer.activeAnimation = modelViewer.animations.Animations[0];
		}
		else {
			modelViewer.activeAnimation = NULL;
		}
	}

	ImGui::End();
}

void ExporterState::renderGUIExport() {
	if (modelViewer.OutputFilename == NULL) return;

	bool always = true;

	ImGui::SetNextWindowPos(ImVec2(0, OPRENDERER_ACTIVE->OPWINDOW_ACTIVE->Height - 41));
	ImGui::Begin("OverlayExport", &always, ImVec2(OPRENDERER_ACTIVE->OPWINDOW_ACTIVE->Width, 41), 0.3f, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);

	char buffer[255];
	sprintf(buffer, "Export %s", modelViewer.OutputFilename->C_Str());

	if (ImGui::Button(buffer, ImVec2(OPRENDERER_ACTIVE->OPWINDOW_ACTIVE->Width, 25))) {
		if (modelViewer.splitterIndex == 0) {
			exporter.Export(modelViewer.OutputAbsolutePath->C_Str());
		}
		else {
			exporter.Export(modelViewer.splitters, modelViewer.splitterIndex, modelViewer.OutputAbsolutePath->C_Str());
		}
	}

	ImGui::End();
}

void ExporterState::renderGUIMeshSelect() {
	ui32 windowWidth = OPRENDERER_ACTIVE->OPWINDOW_ACTIVE->Width;
	bool always = true;

	ImGui::SetNextWindowPos(ImVec2(0, 20));
	ImGui::Begin("SelectorMeshes", &always, ImVec2(windowWidth / 2, 36), 0.3f, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
	if (modelViewer.OutputFilename != NULL) {
		ImGui::PushItemWidth(-1.0f);
		if (ImGui::Combo("Meshes", &meshInd, MeshNameGetter, modelViewer.entity->model, modelViewer.entity->model->meshCount + 1)) {
			for (ui32 i = 0; i < modelViewer.entity->model->meshCount; i++) {
				modelViewer.entity->material[i].visible = meshInd == 0 || i == (meshInd - 1);
			}
		}
	}
	ImGui::End();

	ImGui::SetNextWindowPos(ImVec2(windowWidth / 2, 20));
	ImGui::Begin("SelectorAnimations", &always, ImVec2(windowWidth / 2, 36), 0.3f, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
	if (modelViewer.OutputFilename != NULL) {
		ImGui::PushItemWidth(-1.0f);
		if (ImGui::Combo("Animations", &ind, ItemGetter, modelViewer.animations.AnimationNames, modelViewer.animations.AnimationsCount)) {
			modelViewer.activeAnimation = modelViewer.animations.Animations[ind];
		}
	}
	ImGui::End();
}

void ExporterState::renderGUITextureSelect() {
	if (modelViewer.result == NULL) return;
	bool always = true;

	ui32 windowWidth = OPRENDERER_ACTIVE->OPWINDOW_ACTIVE->Width;

	ImGui::SetNextWindowPos(ImVec2(windowWidth - 128 - 16 - 4, 60));
	ui32 childWindowWidth = 128 + 16;
	ImGui::Begin("CurrentTextures", &always, ImVec2(childWindowWidth, 128 + 16 + 16), 0.0f, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);

	OPtextureGL* glTex = (OPtextureGL*)modelViewer.result->internalPtr;
	ImTextureID tex = (void*)glTex->Handle;
	ImGui::Image(tex, ImVec2(128, 128), ImVec2(0, 0), ImVec2(1, 1), ImColor(255, 255, 255, 255), ImColor(255, 255, 255, 128));

	ImVec2 textSize = ImGui::CalcTextSize("Diffuse");
	ImGui::Text("");
	ui32 offset = (childWindowWidth / 2) - (textSize.x / 2);
	ImGui::SameLine(offset);
	ImGui::Text("Diffuse");

	ImGui::End();
}

void ExporterState::Render(OPfloat delta) {

	OPrenderClear(0.2f, 0.2f, 0.2f, 1);
	OPrenderCullMode(OPcullFace::BACK);
	OPrenderCull(false);

	scene.Render(delta);

	// Happens here instead of update so that we can get a rendered image of the model too
	_processDroppedFiles();

    if(modelViewer.OutputAbsolutePath != NULL && getThumbnail) {
        windowSnapshot->Snapshot(modelViewer.OutputAbsolutePath);
        getThumbnail = false;
    }

	renderGUI();

	OPrenderPresent();
}

OPint ExporterState::Exit(OPgameState* state) {
	return 0;
}

void ExporterState::_processModel(const OPchar* filename) {
	if (!modelViewer.LoadModelFromFile(filename, animsFromFile, &useAnimation)) return;

	if (autoScale && OPvec3Len(modelViewer.bounds.max - modelViewer.bounds.min) > 80) {
		modelViewer.Scale = 0.01f;
	}

	if (autoExport) {
		if (modelViewer.splitterIndex == 0 || !modelViewer.splitFileNameForAnim) {
			exporter.Export(modelViewer.OutputAbsolutePath->C_Str());
		}
		else {
			exporter.Export(modelViewer.splitters, modelViewer.splitterIndex, modelViewer.OutputAbsolutePath->C_Str());
		}
	}
}

bool ExporterState::LoadOPM() {
	modelViewer.LoadOPMFromFile(dropFilenames[currentFile].C_Str());
	modelViewer.Scale = 1.0f;

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

	modelViewer.animations.AnimationNames = OPALLOC(OPchar*, totalAnimCount);
	modelViewer.animations.Animations = OPALLOC(OPskeletonAnimation*, totalAnimCount);
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

			modelViewer.animations.Animations[ind] = (OPskeletonAnimation*)OPCMAN.LoadFromFile(absPath.C_Str());
			modelViewer.animations.AnimationNames[ind] = OPstringCopy(&f._data[filename._len + 1]);
			ind++;
		}
	}

	modelViewer.animations.AnimationsCount = totalAnimCount;

	if (totalAnimCount > 0) {
		modelViewer.activeAnimation = modelViewer.animations.Animations[0];
	}

	return true;
}

void ExporterState::_processDroppedFiles() {
	if (currentFile > dropCount) {
		return;
	}

	if (modelViewer.OutputAbsolutePath != NULL && currentFile <= dropCount && currentFile != 0) {
		windowSnapshot->Snapshot(modelViewer.OutputAbsolutePath);
	}

	if (dropCount == 0 || currentFile == dropCount) {
		currentFile++;
		return;
	}
	
	// We haven't maxed out yet, so if this is the 2nd model
	const OPchar* ext = strrchr(dropFilenames[currentFile].C_Str(), '.');

	if (IsImageFile(ext)) { // Process file as a texture
		modelViewer.ApplyTexture(dropFilenames[currentFile].C_Str(), meshInd);
	}
	else if (IsAnimationFile(ext)) { // Process as animation file
		animsFromFile = true;
		modelViewer.ProcessAnimationsFile(dropFilenames[currentFile].C_Str());
	}
	else if (IsModelFile(ext)) { // Process as model file
		// Reset the camera (this is primarily for the snapshot feature)
		camera.Rotation.x = -0.644917190;
		camera.Rotation.y = -0.785485148;
		camera.Rotation.z = 0.955316544;
		camera.Camera.pos = OPvec3(-5, 5, 5);
		camera.Update();

		if (OPstringEquals(ext, ".opm")) { // Doesn't go through Assimp for OPM files
			LoadOPM();
		}
		else {
			_processModel(dropFilenames[currentFile].C_Str());
		}
	}
	else if (IsSkeltonFile(ext)) { // Process as model file
		OPskeleton* skel = (OPskeleton*)OPCMAN.LoadFromFile(dropFilenames[currentFile].C_Str());
		if (skel != NULL) {
			modelViewer.activeSkeleton = skel;
			modelViewer.activeAnimation = NULL;
		}
	}
	else if (IsSkeletonAnimationFile(ext) && modelViewer.activeSkeleton != NULL) { // Process as anim file
		modelViewer.activeAnimation = (OPskeletonAnimation*)OPCMAN.LoadFromFile(dropFilenames[currentFile].C_Str());
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
ExporterState GS_EXAMPLE;

void DropCallback(OPuint count, const OPchar** filenames) {
	GS_EXAMPLE._drop(count, filenames);
}
