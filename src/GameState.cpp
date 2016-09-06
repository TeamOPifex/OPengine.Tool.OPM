#include "./GameState.h"
#include "Main.h"
#include "OPMconvert.h"

#include "OPimgui.h"


OPscene scene;
OPrendererForward* renderer;
OPmaterialPBR materialPBR;
OPtextureCube environment;
OPmaterialPBRInstance* materialInstance;
OPmaterialPBRInstance* materialInstance2;
OPmaterialInstance* materialInstances;
OPmaterialInstance** materialInstancesArr = NULL;
OPmaterialPBRInstance** materialPBRInstancesArr = NULL;
OPmodel model;
OPsceneEntity* entity;
OPcamFreeFlight camera;
OPfloat Rotation = 0;
OPfloat Scale = 1.0f;
OPboundingBox3D* bounds = NULL;

const OPchar* meshFile = NULL;
const OPchar* texFile = NULL;

OPchar* outputRoot = NULL;
OPchar outName[255];

bool autoExport = false;
bool featureNormals = true;
bool featureUVs = true;
bool featureTangents = true;
bool featureBiTangents = true;
bool featureColors = false;
bool featureBones = false;
bool exportSkeleton = false;
bool exportAnimations = false;

OPchar* AnimationTracks[10];
double AnimationDurations[10];
ui32 totalAnimationTracks = 0;

ui32 SplittersStart[100];
ui32 SplittersEnd[100];
OPchar* SplittersName[100];
ui32 SplittersIndex = 0;

OPmodelMeta metaData[1] = {
	"albedo", 0, NULL
};

OPmeshMeta** meshMeta;

bool LoadMeshFromFile(const OPchar* filename) {
	const OPchar* ext = NULL;
	// Load up an fbx with assimp
	OPmodel* modelTemp = (OPmodel*)OPCMAN.LoadFromFile(filename);
	if (modelTemp == NULL) return false;
	model = *modelTemp;

	materialPBRInstancesArr = materialPBR.CreateInstances(&model, false);
	materialInstancesArr = OPALLOC(OPmaterialInstance*, model.meshCount);
	for (ui32 i = 0; i < model.meshCount; i++) {

		if (model.meshes[i].materialDesc != NULL && model.meshes[i].materialDesc->albedo != NULL) {

			ext = strrchr(filename, '\\');
			outputRoot = OPstringCopy(filename);
			ui32 pos = strlen(outputRoot) - strlen(ext) + 1;
			outputRoot[pos] = NULL;

			OPchar* fullPath = OPstringCreateMerged(outputRoot, model.meshes[i].materialDesc->albedo);
			OPlogInfo("TEXTURE: %s", model.meshes[i].materialDesc->albedo);
			OPtexture* result = (OPtexture*)OPCMAN.LoadFromFile(fullPath);
			if (result == NULL) {
				materialPBRInstancesArr[i]->SetAlbedoMap("Default_Albedo.png");
			}
			else {
				materialPBRInstancesArr[i]->SetAlbedoMap(result);
			}

		} else {
			materialPBRInstancesArr[i]->SetAlbedoMap("Default_Albedo.png");
		}
		materialPBRInstancesArr[i]->SetSpecularMap("Default_Specular.png");
		materialPBRInstancesArr[i]->SetGlossMap("Default_Gloss.png");
		materialPBRInstancesArr[i]->SetNormalMap("Default_Normals.png");
		materialPBRInstancesArr[i]->SetEnvironmentMap(&environment);

		materialInstancesArr[i] = &materialPBRInstancesArr[i]->rootMaterialInstance;
	}

	scene.entities[0].material = materialInstancesArr;

	meshFile = OPstringCopy(filename);
	bounds = &model.meshes[0].boundingBox;

	ext = strrchr(filename, '\\');
	outputRoot = OPstringCopy(filename);
	ui32 pos = strlen(outputRoot) - strlen(ext) + 1;
	outputRoot[pos] = NULL;
	if (ext != NULL) {
		ui32 len = strlen(ext);
		for (ui32 i = 1; i < len - 3 && i < 255; i++) {
			outName[i - 1] = ext[i];
		}

		outName[len - 4] = 'o';
		outName[len - 3] = 'p';
		outName[len - 2] = 'm';
		outName[len - 1] = NULL;
	}
	else {
		ext = strrchr(filename, '/');
		if (ext != NULL) {
			ui32 len = strlen(ext);
			for (ui32 i = 1; i < len - 3 && i < 255; i++) {
				outName[i - 1] = ext[i];
			}

			outName[len - 4] = 'o';
			outName[len - 3] = 'p';
			outName[len - 2] = 'm';
			outName[len - 1] = NULL;

		}
	}



	return true;
}

int texInd = 0;

#include <stdio.h>
#include <iostream>
#include <fstream>
using namespace std;

void DropCallback(OPuint count, const OPchar** filenames) {
	const OPchar* ext = NULL;

	// Process any textures first
	for (OPuint i = 0; i < count; i++) {
		ext = strrchr(filenames[i], '.');
		if (ext == NULL) continue;

		if (OPstringEquals(ext, ".png") || OPstringEquals(ext, ".jpg")) {
			OPtexture* tex = (OPtexture*)OPCMAN.LoadFromFile(filenames[i]);
			if (tex == NULL) continue;
			texFile = OPstringCopy(filenames[i]);
			materialInstance->SetAlbedoMap(tex);

			if (materialInstancesArr != NULL) {

				//for (ui32 i = 0; i < model.meshCount; i++) {
					materialPBRInstancesArr[texInd]->SetAlbedoMap(tex);
				//}
			}

			// Set Meta

			OPchar* justTexName = OPstringCopy(filenames[i]);
			ext = strrchr(justTexName, '\\');
			justTexName = OPstringCopy(&ext[1]);

			metaData[0].dataSize = strlen(justTexName) * sizeof(OPchar);
			metaData[0].data = justTexName;
		}

	}

	for (OPuint i = 0; i < count; i++) {
		ext = strrchr(filenames[i], '.');
		if (ext == NULL) continue;

		if (OPstringEquals(ext, ".txt")) {
			SplittersIndex = 0;
			// Assume it's an animation splitter file
			ifstream myFile(filenames[i]);
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

					SplittersStart[SplittersIndex] = atoi(start);
					SplittersEnd[SplittersIndex] = atoi(end);
					SplittersName[SplittersIndex] = OPstringCopy(name);
					SplittersIndex++;

				}
			}
		}
	}

	// Process models next
	for (OPuint i = 0; i < count; i++) {
		ext = strrchr(filenames[i], '.');
		if (ext == NULL) continue;

		if (OPstringEquals(ext, ".fbx")) {
			if (LoadMeshFromFile(filenames[i])) {
				if (OPvec3Len(bounds->max - bounds->min) > 80) {
					Scale = 0.01f;
				}
				totalAnimationTracks = GetAvailableTracks(filenames[i], AnimationTracks, AnimationDurations, 10);
				if (autoExport) {
					OPchar* output = OPstringCreateMerged(outputRoot, outName);
					ExportOPM(meshFile, output, Scale, &model,
						featureNormals,
						featureUVs,
						featureTangents,
						featureBiTangents,
						featureColors,
						featureBones,
						exportSkeleton,
						exportAnimations,
						SplittersIndex,
						SplittersStart,
						SplittersEnd,
						SplittersName);
					OPfree(output);
				}
			}
		}
		else if (OPstringEquals(ext, ".obj")) {
			if (LoadMeshFromFile(filenames[i])) {
				if (OPvec3Len(bounds->max - bounds->min) > 80) {
					Scale = 0.01f;
				}
				if (autoExport) {
					OPchar* output = OPstringCreateMerged(outputRoot, outName);
					ExportOPM(meshFile, output, Scale, &model,
						featureNormals,
						featureUVs,
						featureTangents,
						featureBiTangents,
						featureColors,
						featureBones,
						exportSkeleton,
						exportAnimations,
						SplittersIndex,
						SplittersStart,
						SplittersEnd,
						SplittersName);
					OPfree(output);
				}
				totalAnimationTracks = 0;
			}
		}
		else if(OPstringEquals(ext, ".opm")) {
			LoadMeshFromFile(filenames[i]);
			Scale = 1.0f;
			bounds = NULL;
			totalAnimationTracks = 0;
		}
	}
}

#include "turbojpeg.h"

OPint OPloaderJPEG(OPstream* stream, OPtexture** image) {

	int jpegSubsamp, width, height;

	tjhandle _jpegDecompressor = tjInitDecompress();

	tjDecompressHeader2(_jpegDecompressor, stream->Data, stream->Length, &width, &height, &jpegSubsamp);

	ui8* data = OPALLOC(ui8, width * height * 3);

	tjDecompress2(_jpegDecompressor, stream->Data, stream->Length, data, width, 0/*pitch*/, height, TJPF_RGB, TJFLAG_FASTDCT);

	tjDestroy(_jpegDecompressor);

	OPtexture* tex = OPNEW(OPtexture());

	ui16 w = width, h = height;
	OPtextureDesc desc;
	desc.width = w;
	desc.height = h;

	desc.format = OPtextureFormat::RGB;

	desc.wrap = OPtextureWrap::REPEAT;
	desc.filter = OPtextureFilter::LINEAR;

	OPRENDERER_ACTIVE->Texture.Init(tex, desc);
	tex->SetData(data);

	OPfree(data);

	*image = tex;

	return 1;
}

OPassetLoader OPLOADER_JPEG = {
	".jpg",
	"Textures/",
	sizeof(OPtexture),
	(OPint(*)(OPstream*, void**))OPloaderJPEG,
	NULL,
	NULL
};

void ExampleStateInit(OPgameState* last) {
	OPimguiInit(OPRENDERER_ACTIVE->OPWINDOW_ACTIVE, true);

	mainWindow.SetDropCallback(DropCallback);

	OPCMAN.AddLoader(&OPLOADER_JPEG);


	const OPchar* envImages[6] = {
		"Textures/Default_Albedo.png",
		"Textures/Default_Albedo.png",
		"Textures/Default_Albedo.png",
		"Textures/Default_Albedo.png",
		"Textures/Default_Albedo.png",
		"Textures/Default_Albedo.png"
	};
	environment.Init(envImages);

	renderer = OPrendererForward::Create();
	scene.Init(&renderer->rendererRoot, 1000, 50);

	camera.Init(1.0f, 1.0f, OPvec3(0, 5, 5));
	scene.camera = &camera.Camera;

	materialPBR.Init(OPNEW(OPeffect("Common/PBR.vert", "Common/PBR.frag")));
	materialPBR.rootMaterial.AddParam("uCamPos", &camera.Camera.pos);
	renderer->SetMaterial(&materialPBR.rootMaterial, 0);

	materialInstance = materialPBR.CreateInstance();
	materialInstance->SetAlbedoMap("Default_Albedo.png");
	materialInstance->SetSpecularMap("Default_Specular.png");
	materialInstance->SetGlossMap("Default_Gloss.png");
	materialInstance->SetNormalMap("Default_Normals.png");
	materialInstance->SetEnvironmentMap(&environment);

	model = *(OPmodel*)OPCMAN.LoadGet("ground_block_2x2x2.opm");

	materialInstances = &materialInstance->rootMaterialInstance;
	entity = scene.Add(&model, &materialInstances);
}


OPint ExampleStateUpdate(OPtimer* time) {
	camera.Update(time);
	entity->world.SetRotY(Rotation / 200.0f)->Scl(Scale);
	return false;
}

void ExampleStateRender(OPfloat delta) {
	OPrenderClear(0.2f, 0.2f, 0.2f, 1);
	OPrenderCullMode(0);
	OPrenderCull(false);
	//OPrenderBlend(true);
	//OPrenderDepth(false);
	//OPrenderDepthWrite(false);

	scene.Render(delta);

	OPimguiNewFrame();
	
	bool openDebugInfo = true;// texFile != NULL || meshFile != NULL;
	ImGui::SetNextWindowPos(ImVec2(10, 10));
	ImGui::Begin("Overlay", &openDebugInfo, ImVec2(OPRENDERER_ACTIVE->OPWINDOW_ACTIVE->Width - 20 - 200, 80), 0.3f, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);

	if (meshFile != NULL) ImGui::Text("Model: %s", meshFile); else  ImGui::Text("Model: ");
	if (texFile != NULL) ImGui::Text("Texture: %s", texFile); else  ImGui::Text("Texture: ");

	ImGui::SliderFloat("Scale", &Scale, 0.001f, 4.0f);

	ImGui::End();

	bool alwaysShow = true;
	ImGui::SetNextWindowPos(ImVec2(OPRENDERER_ACTIVE->OPWINDOW_ACTIVE->Width - 20 - 180, 10));
	ImGui::Begin("Overlay2", &alwaysShow, ImVec2(190, 80), 0.3f, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
	ImGui::Text("Scale");
	if (ImGui::Button("Default")) {
		Scale = 1.0f;
	}
	if (ImGui::Button("CM -> Meters")) {
		Scale = 0.01f;
	}
	ImGui::End();

	ImGui::SetNextWindowPos(ImVec2(OPRENDERER_ACTIVE->OPWINDOW_ACTIVE->Width - 20 - 240, OPRENDERER_ACTIVE->OPWINDOW_ACTIVE->Height - 110));
	ImGui::Begin("OverlayExport", &alwaysShow, ImVec2(250, 100), 0.3f, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
	ImGui::InputText("Output", outName, 255);
	if (ImGui::Button("Export")) {
		if (meshFile != NULL) {
			OPchar* output = OPstringCreateMerged(outputRoot, outName);
			ExportOPM(meshFile, output, Scale, &model,
				featureNormals,
				featureUVs,
				featureTangents,
				featureBiTangents,
				featureColors,
				featureBones,
				exportSkeleton,
				exportAnimations,
				SplittersIndex,
				SplittersStart,
				SplittersEnd,
				SplittersName);
			OPfree(output);
		}
	}

	ImGui::Checkbox("Auto-Export on drop", &autoExport);
	ImGui::End();

	ImGui::SetNextWindowPos(ImVec2(10, 100));
	ImGui::Begin("OverlayFeatures", &alwaysShow, ImVec2(200, 200), 0.3f, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);

	ImGui::Checkbox("Normals", &featureNormals);
	ImGui::Checkbox("UVs", &featureUVs);
	ImGui::Checkbox("Tangents", &featureTangents);
	ImGui::Checkbox("BiTangents", &featureBiTangents);
	ImGui::Checkbox("Colors", &featureColors);
	ImGui::Checkbox("Bones", &featureBones);
	ImGui::Checkbox("Skeleton", &exportSkeleton);
	ImGui::Checkbox("Animations", &exportAnimations);

	if (materialInstancesArr != NULL) {
		ImGui::SliderInt("Mesh", &texInd, 0, model.meshCount);
	}
	ImGui::End();
	//ImGui::ShowTestWindow();

	if (totalAnimationTracks > 0) {

		ImGui::SetNextWindowPos(ImVec2(10, 310));
		ImGui::Begin("OverlayAnimations", &alwaysShow, ImVec2(200, 20 * totalAnimationTracks), 0.3f, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);

		for (ui32 i = 0; i < totalAnimationTracks; i++) {
			ImGui::Text("%s : %d frames", AnimationTracks[i], (int)AnimationDurations[i]);
		}
		ImGui::End();

		if (SplittersIndex > 0) {
			ImGui::SetNextWindowPos(ImVec2(10, 330 + 20 * totalAnimationTracks));
			ImGui::Begin("OverlaySplitters", &alwaysShow, ImVec2(200, 20 * SplittersIndex), 0.3f, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);

			for (ui32 i = 0; i < SplittersIndex; i++) {
				ImGui::Text("%s : %d - %d", SplittersName[i], SplittersStart[i], SplittersEnd[i]);
			}
			ImGui::End();
		}
	}

	ImGui::Render();

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
