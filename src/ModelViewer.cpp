#include "ModelViewer.h"

#include "Utils.h"


OPboundingBox3D GetBounds(OPmesh* mesh);
OPskeleton* LoadSkeletonFromFile(OPstring& filePath);

ModelViewer::ModelViewer(OPscene* s, OPexporter* e) {
	scene = s;
	exporter = e;

	OPmodel* model = (OPmodel*)OPCMAN.LoadGet("box.opm");
	entity = scene->Add(model, OPrendererEntityDesc(false, false, false, false));
	bounds = OPboundingBox3D(OPvec3(-0.5), OPvec3(0.5));
	entity->SetAlbedoMap("Default_Albedo.png");
	entity->material->SetMap("uNormalMap", OPtexture::Load("Default_Albedo.png"));
	entity->material->SetMap("uNormalMap", OPtexture::Load("EmptyNormals.png"));
	entity->material->SetMap("uMetallicRoughnessAOMap", OPtexture::Load("EmptyAO.png"));
	entity->world.
		SetScl(Scale)->
		RotY(Rotate.y)->
		RotX(Rotate.x)->
		RotZ(Rotate.z)->
		Translate(0, (bounds.max.y - bounds.min.y) / 2.0f, 0);

	for (ui32 i = 0; i < 6; i++) {
		textures[i] = NULL;
	}
}


// Loads up an already existing OPM file
// This should ideally be loaded as an Assimp plugin eventually
bool ModelViewer::LoadOPMFromFile(const OPchar* fullFilePath) {
	OPstring filePath(fullFilePath);

	OPmodel* model = (OPmodel*)OPCMAN.LoadFromFile(filePath.C_Str());
	// If we failed to load the model, just return
	if (model == NULL) return false;

	// Remove the previous entity
	scene->Remove(entity);
	bounds = OPboundingBox3D();

	// Try to load a skeleton (checks if it exists)
	activeSkeleton = LoadSkeletonFromFile(filePath);

	if (activeSkeleton != NULL) {
		entity = scene->Add(model, activeSkeleton, OPrendererEntityDesc(true, false, false, true));
	}
	else {
		entity = scene->Add(model, OPrendererEntityDesc(false, false, false, true));
	}

	// Setup the materials per mesh in the model
	for (ui32 i = 0; i < model->meshCount; i++) {
		if (model->meshes[i].materialDesc == NULL) continue;
		bounds = GetBounds(&model->meshes[i]);

		// Diffuse
		textures[0] = LoadTexture(fullFilePath, model->meshes[i].materialDesc->diffuse);

		if (textures[0] == NULL) entity->SetAlbedoMap(DEFAULT_TEXTURE, i);
		else entity->SetAlbedoMap(textures[0], i);
	}

	if (OutputFilename != NULL) {
		delete OutputFilename;
	}
	OutputFilename = GetFilenameOPM(fullFilePath, splitFileNameForAnim);

	if (OutputAbsolutePath != NULL) {
		delete OutputAbsolutePath;
	}
	OutputAbsolutePath = GetAbsolutePathOPM(fullFilePath, splitFileNameForAnim);

	return true;
}



bool ModelViewer::LoadModelFromFile(const OPchar* filename, bool animsFromFile, bool* useAnimation) {
	const OPchar* ext = NULL;

	if (!animsFromFile) {
		splitterIndex = 0;
	}

	// Load up an fbx with assimp
	exporter->Init(filename, NULL);

	if (exporter->scene == NULL) {
		return false;
	}

	bounds = OPboundingBox3D();

	if (entity != NULL) {
		scene->Remove(entity);
	}

	activeSkeleton = NULL;

	if (exporter->HasAnimations) {
		//OPstream* stream = OPfile::ReadFromFile(filename);
		// Get Skeleton
		OPskeleton* skeleton = exporter->LoadSkeleton();
		activeSkeleton = skeleton;

		*useAnimation = true;

		// Get Animations
		if (splitterIndex == 0) {
			OPstring str(filename);
			if (str.Contains("_")) {
				ui32 ind = str.IndexOfLast('_');
				ui32 indEnd = str.IndexOfLast('.');
				OPstring* result = str.Substr(ind + 1, indEnd);
				animations = exporter->LoadAnimations(result->C_Str());

				splitterIndex = 1;
				splitters[0].Start = 0;
				splitters[0].End = animations.Animations[0]->FrameCount;
				splitters[0].Name = animations.AnimationNames[0];

				OPfree(result);
			}
			else {
				animations = exporter->LoadAnimations();
			}
		}
		else {
			animations = exporter->LoadAnimations(splitters, splitterIndex);
		}

		if (animations.AnimationsCount > 0) {
			activeAnimation = animations.Animations[0];
		}
		else {
			activeAnimation = NULL;
		}
	}

	OPmodel* model = exporter->existingModel;

	if (model == NULL) {
		return false;
	}

	if (activeSkeleton != NULL) {
		entity = scene->Add(model, activeSkeleton, OPrendererEntityDesc(true, false, false, true));
	}
	else {
		entity = scene->Add(model, OPrendererEntityDesc(false, false, false, true));
	}

	// Setup the materials per mesh in the model
	for (ui32 i = 0; i < model->meshCount; i++) {
		if (model->meshes[i].materialDesc == NULL) continue;
		bounds = GetBounds(&model->meshes[i]);

		// Diffuse
		textures[0] = LoadTexture(filename, model->meshes[i].materialDesc->diffuse);
		if (textures[0] == NULL) {
			entity->SetAlbedoMap("Default_Albedo.png", i);
		}
		else {
			entity->SetAlbedoMap(textures[0], i);
		}
	}

	if (OutputFilename != NULL) {
		delete OutputFilename;
	}
	OutputFilename = GetFilenameOPM(filename, splitFileNameForAnim);

	if (OutputAbsolutePath != NULL) {
		delete OutputAbsolutePath;
	}
	OutputAbsolutePath = GetAbsolutePathOPM(filename, splitFileNameForAnim);

	return true;
}

bool ModelViewer::ApplyTexture(const OPchar* filename, ui32 meshInd) {
	OPtexture* tex = (OPtexture*)OPCMAN.LoadFromFile(filename);
	if (tex == NULL) return false;

	textures[textureInd] = tex;


	OPstring texName(filename);
	OPint pos = texName.IndexOfLast('\\');
	if (pos != -1) {
		texName.Init(&texName._data[pos + 1]);
	}

	pos = texName.IndexOfLast('/');
	if (pos != -1) {
		texName.Init(&texName._data[pos + 1]);
	}

	switch (textureInd) {
		case 0: {
			if (meshInd == 0) {
				for (ui32 i = 0; i < entity->model->meshCount; i++) {
					entity->SetAlbedoMap(tex, i);
					entity->model->meshes[i].materialDesc->diffuse = OPstringCopy(texName.C_Str());
				}
			}
			else {
				entity->SetAlbedoMap(tex, meshInd - 1);
				entity->model->meshes[meshInd - 1].materialDesc->diffuse = OPstringCopy(texName.C_Str());
			}
			break;
		}
		case 1: {
			if (meshInd == 0) {
				for (ui32 i = 0; i < entity->model->meshCount; i++) {
					entity->material[i].AddParam("uNormalMap", tex, 0);
					entity->model->meshes[i].materialDesc->normals = OPstringCopy(texName.C_Str());
				}
			}
			else {
				entity->material[meshInd - 1].AddParam("uNormalMap", tex, 0);
				entity->model->meshes[meshInd - 1].materialDesc->normals = OPstringCopy(texName.C_Str());
			}
			break;
		}
		case 2: {
			if (meshInd == 0) {
				for (ui32 i = 0; i < entity->model->meshCount; i++) {
					entity->material[i].AddParam("uMetallicMap", tex, 0);
					entity->model->meshes[i].materialDesc->other1 = OPstringCopy(texName.C_Str());
				}
			}
			else {
				entity->material[meshInd - 1].AddParam("uMetallicMap", tex, 0);
				entity->model->meshes[meshInd - 1].materialDesc->other1 = OPstringCopy(texName.C_Str());
			}
			break;
		}
		case 3: {
			if (meshInd == 0) {
				for (ui32 i = 0; i < entity->model->meshCount; i++) {
					entity->material[i].AddParam("uRoughnessMap", tex, 0);
					entity->model->meshes[i].materialDesc->other2 = OPstringCopy(texName.C_Str());
				}
			}
			else {
				entity->material[meshInd - 1].AddParam("uRoughnessMap", tex, 0);
				entity->model->meshes[meshInd - 1].materialDesc->other2 = OPstringCopy(texName.C_Str());
			}
			break;
		}
		case 4: {
			if (meshInd == 0) {
				for (ui32 i = 0; i < entity->model->meshCount; i++) {
					entity->material[i].AddParam("uAOMap", tex, 0);
					entity->model->meshes[i].materialDesc->ambient = OPstringCopy(texName.C_Str());
				}
			}
			else {
				entity->material[meshInd - 1].AddParam("uAOMap", tex, 0);
				entity->model->meshes[meshInd - 1].materialDesc->ambient = OPstringCopy(texName.C_Str());
			}
			break;
		}
		case 5: {
			if (meshInd == 0) {
				for (ui32 i = 0; i < entity->model->meshCount; i++) {
					entity->material[i].AddParam("uMetallicRoughnessAOMap", tex, 0);
					entity->model->meshes[i].materialDesc->ambient = OPstringCopy(texName.C_Str());
				}
			}
			else {
				entity->material[meshInd - 1].AddParam("uMetallicRoughnessAOMap", tex, 0);
				entity->model->meshes[meshInd - 1].materialDesc->ambient = OPstringCopy(texName.C_Str());
			}
			break;
		}
	}
}

bool ModelViewer::ProcessAnimationsFile(const OPchar* filename) {

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
			animations = exporter->LoadAnimations(splitters, splitterIndex);
			if (animations.AnimationsCount > 0) {
				activeAnimation = animations.Animations[0];
			}
			else {
				activeAnimation = NULL;
			}
		}
	}
	return true;
}

OPboundingBox3D GetBounds(OPmesh* mesh) {
	OPboundingBox3D bounds = OPboundingBox3D();
	if (mesh->boundingBox.min.x < bounds.min.x) bounds.min.x = mesh->boundingBox.min.x;
	if (mesh->boundingBox.min.y < bounds.min.y) bounds.min.y = mesh->boundingBox.min.y;
	if (mesh->boundingBox.min.z < bounds.min.z) bounds.min.z = mesh->boundingBox.min.z;
	if (mesh->boundingBox.max.x > bounds.max.x) bounds.max.x = mesh->boundingBox.max.x;
	if (mesh->boundingBox.max.y > bounds.max.y) bounds.max.y = mesh->boundingBox.max.y;
	if (mesh->boundingBox.max.z > bounds.max.z) bounds.max.z = mesh->boundingBox.max.z;
	return bounds;
}


OPskeleton* LoadSkeletonFromFile(OPstring& filePath) {
	OPskeleton* result = NULL;
	filePath.Add(".skel");
	if (OPfile::Exists(filePath.C_Str())) {
		OPstream* stream = OPfile::ReadFromFile(filePath.C_Str());
		OPskeleton* skeleton;
		if (OPloaderOPskeletonLoad(stream, &skeleton)) {
			result = skeleton;
		}
		stream->Destroy();
	}
	return result;
}