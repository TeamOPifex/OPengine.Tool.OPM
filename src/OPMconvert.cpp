#include "OPMconvert.h"
#include "./Data/include/OPstring.h"
#include "./Human/include/Rendering/OPMvertex.h"
#ifdef ADDON_assimp
#include "./OPassimp.h"
#endif

#include <iostream>
#include <fstream>
using namespace std;
 
void write(ofstream* stream, void* data, i32 size) {
	stream->write((char*)data, size);
}
void writeU8(ofstream* stream, ui8 val) {
	write(stream, &val, sizeof(ui8));
}
void writeI8(ofstream* stream, i8 val) {
	write(stream, &val, sizeof(i8));
}
void writeF32(ofstream* stream, f32 val) {
	write(stream, &val, sizeof(f32));
}
void writeI16(ofstream* stream, i16 val) {
	write(stream, &val, sizeof(i16));
}
void writeU16(ofstream* stream, ui16 val) {
	write(stream, &val, sizeof(ui16));
}
void writeI32(ofstream* stream, i32 val) {
	write(stream, &val, sizeof(i32));
}
void writeU32(ofstream* stream, ui32 val) {
	write(stream, &val, sizeof(ui32));
}
void writeString(ofstream* stream, const OPchar* val) {
	ui32 len = strlen(val);
	writeU32(stream, len);
	if (len == 0) return;
	write(stream, (void*)val, len * sizeof(OPchar));
}

enum ModelFeatures {
	Model_Positions = 0,
	Model_Normals = 1,
	Model_UVs = 2,
	Model_Colors = 3,
	Model_Indices = 4,
	Model_Tangents = 5,
	Model_Bones = 6,
	Model_Skinning = 7,
	Model_Animations = 8,
	Model_Skeletons = 9,
	Model_Meta = 10,
	Model_Bitangents = 11,
	MAX_FEATURES
};

ui32 GetAvailableTracks(const OPchar* filename, OPchar** buff, double* durations, ui32 max) {
#ifdef ADDON_assimp
	Assimp::Importer importer;

	const aiScene* scene = importer.ReadFile(filename,
		aiProcess_CalcTangentSpace |
		aiProcess_Triangulate |
		aiProcess_JoinIdenticalVertices |
		aiProcess_SortByPType |
		aiProcess_OptimizeMeshes |
		aiProcess_OptimizeGraph
	);

	// If the import failed, report it
	if (!scene)
	{
		OPlogErr("Failed");
		return 0;
	}

	if (!scene->HasAnimations()) {
		return 0;
	}

	ui32 i = 0;
	for (; i < scene->mNumAnimations && i < max; i++) {
		buff[i] = OPstringCopy(scene->mAnimations[i]->mName.C_Str());
		durations[i] = scene->mAnimations[i]->mDuration;
	}

	return i;
    #endif
    return 0;
}

struct BoneWeight {
	OPfloat weights[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	i32 bones[4] = { 0, 0, 0, 0 };
};

void ExportOPM(const OPchar* filename, OPchar* output, f32 scale, OPmodel* model,
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
	OPchar** splitName) {

        #ifdef ADDON_assimp
	Assimp::Importer importer;
	// And have it read the given file with some example postprocessing
	// Usually - if speed is not the most important aspect for you - you'll
	// propably to request more postprocessing than we do in this example.
	const aiScene* scene = importer.ReadFile(filename,
		aiProcess_CalcTangentSpace |
		aiProcess_Triangulate |
		aiProcess_JoinIdenticalVertices |
		aiProcess_SortByPType |
		aiProcess_OptimizeMeshes |
		aiProcess_OptimizeGraph
	);
	//|
	//	aiProcess_OptimizeMeshes |
	//	aiProcess_OptimizeGraph

	// If the import failed, report it
	if (!scene)
	{
		OPlogErr("Failed");
		return;
	}

	if (scene->HasAnimations()) {
		OPlogInfo("Model has animaitons");
	}

	if (output == NULL) {
		output = OPstringCopy(filename);
	}

	OPstringToLower(output);
	OPint contains = OPstringContains(output, ".opm");
	if (contains > 0) {
		output[contains] = NULL;
	}
	// Now we can access the file's contents
	OPchar* outputFinal = OPstringCreateMerged(output, ".opm");
	ofstream myFile(outputFinal, ios::binary);

	// OPM File Format Version
	writeU16(&myFile, 3);

	writeString(&myFile, scene->mRootNode->mName.C_Str());
	OPlogInfo("MODEL: %s", scene->mRootNode->mName.C_Str());


	ui32 features[MAX_FEATURES];
	OPbzero(features, sizeof(ui32) * MAX_FEATURES);
	features[Model_Positions] = 1;
	features[Model_Normals] = featureNormals;
	features[Model_UVs] = featureUVs;
	features[Model_Tangents] = featureTangents;
	features[Model_Bitangents] = featureBiTangents;
	features[Model_Indices] = 1;
	features[Model_Colors] = featureColors;
	features[Model_Bones] = featureBones;
	features[Model_Skinning] = featureBones;
	features[Model_Skeletons] = exportSkeleton;
	features[Model_Animations] = exportAnimations;
	features[Model_Meta] = 0;

	// Number of meshes
	ui32 meshCount = 0;

	for (ui32 i = 0; i < scene->mNumMeshes; i++) {
		aiMesh* mesh = scene->mMeshes[i];
		if (features[Model_Bones] && !mesh->HasBones()) {
			continue;
		}
		meshCount++;
	}
	writeU32(&myFile, meshCount);


	// All of the meshes must have the same layout
	// We'll turn features off if we have to
	for (ui32 i = 0; i < scene->mNumMeshes; i++) {
		aiMesh* mesh = scene->mMeshes[i];

		ui32 stride = 0;
		if (features[Model_Normals] && !mesh->HasNormals()) {
			OPlogErr("Mesh %d didn't have 'Normals' to add: '%s'", i, mesh->mName);
			features[Model_Normals] = 0;
		}
		if (features[Model_UVs] && !mesh->HasTextureCoords(0)) {
			OPlogErr("Mesh %d didn't have 'UVs' to add: '%s'", i, mesh->mName);
			features[Model_UVs] = 0;
		}
		if (features[Model_Tangents] && !mesh->HasTangentsAndBitangents()) {
			OPlogErr("Mesh %d didn't have 'Tangents' to add: '%s'", i, mesh->mName);
			features[Model_Tangents] = 0;
		}
		if (features[Model_Bitangents] && !mesh->HasTangentsAndBitangents()) {
			OPlogErr("Mesh %d didn't have 'Bitangents' to add: '%s'", i, mesh->mName);
			features[Model_Bitangents] = 0;
		}
		if (features[Model_Bones] && !mesh->HasBones()) {
			OPlogErr("Mesh %d didn't have 'Bones' to add: '%s'", i, mesh->mName);
			//features[Model_Bones] = 0;
		}
	}

	ui32 featureFlags = 0;
	if (features[Model_Positions]) featureFlags += 0x01;
	if (features[Model_Normals]) featureFlags += 0x02;
	if (features[Model_UVs]) featureFlags += 0x04;
	if (features[Model_Tangents]) featureFlags += 0x08;
	if (features[Model_Bitangents]) featureFlags += 0x400;
	if (features[Model_Colors]) featureFlags += 0x100;
	if (features[Model_Indices]) featureFlags += 0x10;
	if (features[Model_Bones]) featureFlags += 0x20;
	if (features[Model_Skinning]) featureFlags += 0x40;
	if (features[Model_Animations]) featureFlags += 0x80;
	if (features[Model_Meta]) featureFlags += 0x200;

	// Features in the OPM
	writeU32(&myFile, featureFlags);


	// Vertex Mode
	// 1 == Vertex Stride ( Pos/Norm/Uv )[]
	// 2 == Vertex Arrays ( Pos )[] ( Norm )[] ( Uv )[]
	writeU16(&myFile, 1);


	ui32 totalVerticesEntireModel = 0;
	ui32 totalIndicesEntireModel = 0;

	for (ui32 i = 0; i < scene->mNumMeshes; i++) {
		aiMesh* mesh = scene->mMeshes[i];
		if (features[Model_Bones] && !mesh->HasBones()) {
			continue;
		}

		for (ui32 j = 0; j < mesh->mNumFaces; j++) {
			aiFace face = mesh->mFaces[j];
			totalVerticesEntireModel += face.mNumIndices;
			if (face.mNumIndices == 3) {
				totalIndicesEntireModel += 3;
			}
			else if (face.mNumIndices == 4) {
				totalIndicesEntireModel += 6;
			}
		}
	}

	writeU32(&myFile, totalVerticesEntireModel);
	writeU32(&myFile, totalIndicesEntireModel);

	OPindexSize indexSize = OPindexSize::INT;
	writeU8(&myFile, (ui8)indexSize);

	ui32 offset = 0;
	for (ui32 i = 0; i < scene->mNumMeshes; i++) {
		aiMesh* mesh = scene->mMeshes[i];

		if (features[Model_Bones] && !mesh->HasBones()) {
			continue;
		}

		writeString(&myFile, mesh->mName.C_Str());

		ui32 totalVertices = 0;
		ui32 totalIndices = 0;
		for (ui32 j = 0; j < mesh->mNumFaces; j++) {
			aiFace face = mesh->mFaces[j];
			totalVertices += face.mNumIndices;
			if (face.mNumIndices == 3) {
				totalIndices += 3;
			}
			else if (face.mNumIndices == 4) {
				totalIndices += 6;
			}
		}

		OPlogInfo("Mesh Verts %d | %d", i, totalVertices);
		writeU32(&myFile, totalVertices);
		writeU32(&myFile, totalIndices);

		OPboundingBox3D boundingBox;

		//ui16* indData = (ui16*)OPalloc(sizeof(ui16) * totalIndices);
		//ui16 offset = 0;



		for (ui32 j = 0; j < mesh->mNumFaces; j++) {
			aiFace face = mesh->mFaces[j];
			if (face.mNumIndices > 4) {
				OPlogErr("Only Supporting 3 and 4 point faces.");
				continue;
			}

			OPfloat* allWeights = NULL;
			i32* allBoneIndexes = NULL;
			if (mesh->HasBones() && featureBones) {

				i32* boneCounts = NULL;
				boneCounts = (i32*)OPallocZero(sizeof(i32) * mesh->mNumVertices);
				for (ui32 boneInd = 0; boneInd < mesh->mNumBones; boneInd++) {
					const aiBone* bone = mesh->mBones[boneInd];
					for (int boneWeightInd = 0; boneWeightInd < bone->mNumWeights; boneWeightInd++) {
						const aiVertexWeight* weight = &bone->mWeights[boneWeightInd];
						boneCounts[weight->mVertexId]++;
					}
				}

				ui32 maxBones = 0;
				for (ui32 maxBoneInd = 0; maxBoneInd < mesh->mNumVertices; maxBoneInd++) {
					if (maxBones < boneCounts[maxBoneInd]) {
						maxBones = boneCounts[maxBoneInd];
					}
				}
				for (ui32 boneCountInd = 0; boneCountInd < mesh->mNumVertices; boneCountInd++) {
					boneCounts[boneCountInd] = 0;
				}

				// maxBones is now the largest number of bones per vertex

				if (maxBones > 4) {
					OPlogErr("Can't handle more than 4 weights right now");
					return;
				}


				allWeights = (OPfloat*)OPallocZero(sizeof(OPfloat) * mesh->mNumVertices * 4);
				allBoneIndexes = (i32*)OPallocZero(sizeof(i32) * mesh->mNumVertices * 4);
				for (ui32 boneInd = 0; boneInd < mesh->mNumBones; boneInd++) {
					const aiBone* bone = mesh->mBones[boneInd];
					for (int boneWeightInd = 0; boneWeightInd < bone->mNumWeights; boneWeightInd++) {
						const aiVertexWeight* weight = &bone->mWeights[boneWeightInd];
						ui32 offset = boneCounts[weight->mVertexId]++;
						allWeights[(weight->mVertexId * 4) + offset] = weight->mWeight;
						allBoneIndexes[(weight->mVertexId * 4) + offset] = boneInd;
					}
				}

				OPfree(boneCounts);

			}


			aiVector3D verts[4];
			aiVector3D normals[4];
			aiVector3D uvs[4];
			aiColor4D colors[4];
			aiVector3D bitangents[4];
			aiVector3D tangents[4];
			BoneWeight boneWeights[4];

			for (ui32 k = 0; k < face.mNumIndices; k++) {
				verts[k] = mesh->mVertices[face.mIndices[k]];

				OPvec3 point = OPvec3(verts[k].x * scale, verts[k].y * scale, verts[k].z * scale);

				if (point.x < boundingBox.min.x) boundingBox.min.x = point.x;
				if (point.y < boundingBox.min.y) boundingBox.min.y = point.y;
				if (point.z < boundingBox.min.z) boundingBox.min.z = point.z;
				if (point.x > boundingBox.max.x) boundingBox.max.x = point.x;
				if (point.y > boundingBox.max.y) boundingBox.max.y = point.y;
				if (point.z > boundingBox.max.z) boundingBox.max.z = point.z;

				if (mesh->HasNormals()) {
					normals[k] = mesh->mNormals[face.mIndices[k]];
				}
				if (mesh->HasTextureCoords(0)) {
					// Only supporting 1 layer of texture coordinates right now
					uvs[k] = mesh->mTextureCoords[0][face.mIndices[k]];
				}
				if (mesh->HasVertexColors(0)) {
					// Only supporting 1 layer of colors right now
					colors[k] = mesh->mColors[0][face.mIndices[k]];
				}
				if (mesh->HasTangentsAndBitangents()) {
					bitangents[k] = mesh->mBitangents[face.mIndices[k]];
					tangents[k] = mesh->mTangents[face.mIndices[k]];
				}
				if (mesh->HasBones() && featureBones) {
					boneWeights[k].weights[0] = allWeights[face.mIndices[k] * 4 + 0];
					boneWeights[k].weights[1] = allWeights[face.mIndices[k] * 4 + 1];
					boneWeights[k].weights[2] = allWeights[face.mIndices[k] * 4 + 2];
					boneWeights[k].weights[3] = allWeights[face.mIndices[k] * 4 + 3];
					boneWeights[k].bones[0] = allBoneIndexes[face.mIndices[k] * 4 + 0];
					boneWeights[k].bones[1] = allBoneIndexes[face.mIndices[k] * 4 + 1];
					boneWeights[k].bones[2] = allBoneIndexes[face.mIndices[k] * 4 + 2];
					boneWeights[k].bones[3] = allBoneIndexes[face.mIndices[k] * 4 + 3];
				}
			}

			if (allWeights != NULL) OPfree(allWeights);
			if (allBoneIndexes != NULL) OPfree(allBoneIndexes);

			// Write each vertex
			for (ui32 k = 0; k < face.mNumIndices; k++) {

				// Position
				writeF32(&myFile, verts[k].x * scale);
				writeF32(&myFile, verts[k].y * scale);
				writeF32(&myFile, verts[k].z * scale);

				// Normal
				if (mesh->HasNormals() && features[Model_Normals]) {
					writeF32(&myFile, normals[k].x);
					writeF32(&myFile, normals[k].y);
					writeF32(&myFile, normals[k].z);
				}

				if (mesh->HasTangentsAndBitangents() && features[Model_Tangents]) {
					writeF32(&myFile, tangents[k].x);
					writeF32(&myFile, tangents[k].y);
					writeF32(&myFile, tangents[k].z);
				}

				if (mesh->HasTangentsAndBitangents() && features[Model_Bitangents]) {
					writeF32(&myFile, bitangents[k].x);
					writeF32(&myFile, bitangents[k].y);
					writeF32(&myFile, bitangents[k].z);
				}

				if (mesh->HasTextureCoords(0) && features[Model_UVs]) {
					writeF32(&myFile, uvs[k].x);
					writeF32(&myFile, uvs[k].y);
				}

				if (mesh->HasBones() && features[Model_Bones]) {
					// TODO
					writeU16(&myFile, boneWeights[k].bones[0]);
					writeU16(&myFile, boneWeights[k].bones[1]);
					writeU16(&myFile, boneWeights[k].bones[2]);
					writeU16(&myFile, boneWeights[k].bones[3]);
					writeF32(&myFile, boneWeights[k].weights[0]);
					writeF32(&myFile, boneWeights[k].weights[1]);
					writeF32(&myFile, boneWeights[k].weights[2]);
					writeF32(&myFile, boneWeights[k].weights[3]);
				}

				if (mesh->HasVertexColors(0) && features[Model_Colors]) {
					writeF32(&myFile, colors[k].r);
					writeF32(&myFile, colors[k].g);
					writeF32(&myFile, colors[k].b);
				}
				else if (features[Model_Colors]) {
					writeF32(&myFile, 0);
					writeF32(&myFile, 0);
					writeF32(&myFile, 0);
				}
			}
		}

		for (ui32 j = 0; j < mesh->mNumFaces; j++) {
			aiFace face = mesh->mFaces[j];
			if (face.mNumIndices > 4) {
				OPlogErr("Only Supporting 3 and 4 point faces.");
				continue;
			}

			if (indexSize == OPindexSize::INT) {
				if (face.mNumIndices == 3) {
					//OPlog("Triangle");
					writeU32(&myFile, offset++);
					writeU32(&myFile, offset++);
					writeU32(&myFile, offset++);
				}
				else {
					//OPlog("Quad");
					writeU32(&myFile, offset + 0);
					writeU32(&myFile, offset + 1);
					writeU32(&myFile, offset + 2);
					writeU32(&myFile, offset + 0);
					writeU32(&myFile, offset + 2);
					writeU32(&myFile, offset + 3);
					offset += 4;
				}
			} else {
				if (face.mNumIndices == 3) {
					//OPlog("Triangle");
					writeU16(&myFile, offset++);
					writeU16(&myFile, offset++);
					writeU16(&myFile, offset++);
				}
				else {
					//OPlog("Quad");
					writeU16(&myFile, offset + 0);
					writeU16(&myFile, offset + 1);
					writeU16(&myFile, offset + 2);
					writeU16(&myFile, offset + 0);
					writeU16(&myFile, offset + 2);
					writeU16(&myFile, offset + 3);
					offset += 4;
				}
			}
		}

		writeF32(&myFile, boundingBox.min.x);
		writeF32(&myFile, boundingBox.min.y);
		writeF32(&myFile, boundingBox.min.z);
		writeF32(&myFile, boundingBox.max.x);
		writeF32(&myFile, boundingBox.max.y);
		writeF32(&myFile, boundingBox.max.z);

		if (model != NULL) {
			writeU32(&myFile, model->meshes[i].meshMeta->count);
			for (ui32 j = 0; j < model->meshes[i].meshMeta->count; j++) {
				writeU32(&myFile, (ui32)model->meshes[i].meshMeta->metaType[j]);
				OPchar* test = model->meshes[i].meshMeta->data[j]->String();
				writeString(&myFile, test);
				//write(&myFile, model->meshes[i].meshMeta->data->Data, model->meshes[i].meshMeta->data->Length);
			}
		}
		else {
			writeU32(&myFile, 0);
		}

	}

	myFile.close();

	// Now create skeleton
	if (exportSkeleton) {

		for (ui32 i = 0; i < scene->mNumMeshes; i++) {
			const aiMesh* mesh = scene->mMeshes[i];
			char buffer[20];
			OPchar* skelFileNum = OPstringCreateMerged(outputFinal, itoa(i, buffer, 10));
			OPchar* skeletonOutput = OPstringCreateMerged(skelFileNum, ".skel");
			ofstream skelFile(outputFinal, ios::binary);

			writeI16(&skelFile, mesh->mNumBones);

			for (ui32 i = 0; i < mesh->mNumBones; i++) {
				const aiBone* bone = mesh->mBones[i];
				writeI16(&skelFile, i);
				writeString(&skelFile, bone->mName.C_Str());
				for (ui32 j = 0; j < 4; j++) {
					for (ui32 k = 0; k < 4; k++) {
						writeF32(&skelFile, bone->mOffsetMatrix[j][k]);
					}
				}
			}

			skelFile.close();

		}
	}

	// Now create animations
	if (exportAnimations && scene->HasAnimations()) {
		for (ui32 i = 0; i < scene->mNumAnimations; i++) {
			const aiAnimation* animation = scene->mAnimations[i];

			OPchar* animationWithName = OPstringCreateMerged(outputFinal, animation->mName.C_Str());
			OPchar* animationOutput = OPstringCreateMerged(animationWithName, ".anim");
			ofstream animFile(animationOutput, ios::binary);

			writeI16(&animFile, animation->mNumChannels);
			writeString(&animFile, animation->mName.C_Str());
			writeU32(&animFile, (ui32)animation->mDuration);

			for (ui32 j = 0; j < scene->mNumMeshes; j++) {
				const aiMesh* mesh = scene->mMeshes[j];
			}

			for (ui32 j = 0; j < animation->mNumChannels; j++) {
				const aiNodeAnim* data = animation->mChannels[j];

			}

			animFile.close();
		}

	}

	OPlog(output);
    #endif
}
