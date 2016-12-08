#include "OPMconvert.h"
#include "./Data/include/OPstring.h"
#include "./Human/include/Rendering/OPMvertex.h"

void OPexporter::Init(const OPchar* filename, OPmodel* desc) {
	path = filename;
	existingModel = desc;

	// And have it read the given file with some example postprocessing
	// Usually - if speed is not the most important aspect for you - you'll
	// propably to request more postprocessing than we do in this example.
	scene = importer.ReadFile(filename,
		aiProcess_CalcTangentSpace |
		aiProcess_Triangulate |
		aiProcess_JoinIdenticalVertices |
		aiProcess_SortByPType/* |
		aiProcess_OptimizeGraph |
		aiProcess_OptimizeMeshes*/
	);

	// If the import failed, report it
	if (!scene)
	{
		OPlogErr("Failed");
		return;
	}

	HasAnimations = Export_Animations = scene->HasAnimations();
	if (scene->HasMeshes()) {
		aiMesh* mesh = scene->mMeshes[0];
		Feature_Normals = mesh->HasNormals();
		Feature_UVs = mesh->HasTextureCoords(0);
		Feature_Tangents = Feature_BiTangents = mesh->HasTangentsAndBitangents();
		Feature_Colors = mesh->HasVertexColors(0);
		Feature_Bones = Export_Skeleton = mesh->HasBones();
		Export_Model = true;
	}
}

ui32 OPexporter::_findBoneIndex(const OPchar* name) {
	ui32 ind = 0;

	for (ui32 i = 0; i < scene->mNumMeshes; i++) {
		const aiMesh* mesh = scene->mMeshes[i];

		if (!mesh->HasBones()) continue;

		for (ui32 j = 0; j < mesh->mNumBones; j++) {
			if (OPstringEquals(mesh->mBones[j]->mName.C_Str(), name)) {
				// This is the same bone
				return ind;
			}
			ind++;
		}
	}
}

OPmat4 OPexporter::_findBoneOffset(ui32 boneInd) {
	ui32 ind = 0;

	for (ui32 i = 0; i < scene->mNumMeshes; i++) {
		const aiMesh* mesh = scene->mMeshes[i];

		if (!mesh->HasBones()) continue;
		for (ui32 j = 0; j < mesh->mNumBones; j++) {
			if (ind == boneInd) { // This is the bone we're looking for
				OPmat4 result;
				for (ui32 j = 0; j < 4; j++) {
					for (ui32 k = 0; k < 4; k++) {
						result[j][k] = mesh->mBones[j]->mOffsetMatrix[j][k];
						//writeF32(&skelFile, bone->mOffsetMatrix[j][k]);
					}
				}
				return result;
			}
			ind++;
		}
	}

	return OPMAT4_IDENTITY;
}

OPmat4 OPexporter::_findBoneOffset(const OPchar* name) {
	return _findBoneOffset(_findBoneIndex(name));
}

OPmat4 aiMatrixToOPmat4(aiMatrix4x4 mat) {
	OPmat4 result;
	for (ui32 i = 0; i < 4; i++) {
		for (ui32 j = 0; j < 4; j++) {
			result[i][j] = mat[i][j];
		}
	}
	return result;
}

void OPexporter::_loadBones() {
	boneMapping.clear();
	_setHierarchy();

	for (ui32 m = 0; m < scene->mNumMeshes; m++) {
		aiMesh* mesh = scene->mMeshes[m];

		for (ui32 i = 0; i < mesh->mNumBones; i++) {
			string BoneName(mesh->mBones[i]->mName.data);
			ui32 BoneIndex = boneMapping[BoneName]; 
			boneInfo[BoneIndex].BoneOffset = aiMatrixToOPmat4(mesh->mBones[i]->mOffsetMatrix);
			
			//for (ui32 j = 0; j < mesh->mBones[i]->mNumWeights; j++) {
			//	ui32 VertexID = entries[m].BaseVertex + mesh->mBones[i]->mWeights[j].mVertexId;
			//	float Weight = mesh->mBones[i]->mWeights[j].mWeight;
			//	Bones[VertexID].AddBoneData(BoneIndex, Weight);
			//}
		}
	}

}

ui32 index = 0;
void OPexporter::_setHierarchy(aiNode* node, i32 parent) {
	string NodeName(node->mName.data);
	boneMapping[NodeName] = index;
	BoneInfo bi;
	boneInfo.push_back(bi);
	boneInfo[index].ParentIndex = parent;
	boneInfo[index].Name = node->mName.C_Str();
	index++;
	numBones++;
	for (ui32 i = 0; i < node->mNumChildren; i++) {
		_setHierarchy(node->mChildren[i], index);
	}
}

void OPexporter::_setHierarchy() {
	index = 0;
	numBones = 0;
	_setHierarchy(scene->mRootNode, -1);
}


OPskeleton* OPexporter::LoadSkeleton(OPstream* stream) {

	_loadBones();

	OPskeleton* result = NULL;
	OPstream* str = OPstream::Create(256);
	
	str->I16(numBones); // bone count

	// Global Inverse Transform
	for (ui32 j = 0; j < 4; j++) {
		for (ui32 k = 0; k < 4; k++) {
			str->F32((f32)OPMAT4_IDENTITY.cols[j].row[k]);
		}
	}

	for (ui32 i = 0; i < numBones; i++) {
		str->I16(boneInfo[i].ParentIndex);
		str->WriteString(boneInfo[i].Name);

		// Bind pose
		for (ui32 j = 0; j < 4; j++) {
			for (ui32 k = 0; k < 4; k++) {
				str->F32((f32)OPMAT4_IDENTITY.cols[j].row[k]);
			}
		}

		// Offset
		for (ui32 j = 0; j < 4; j++) {
			for (ui32 k = 0; k < 4; k++) {
				//str->F32((f32)OPMAT4_IDENTITY.cols[j].row[k]);
				str->F32((f32)boneInfo[i].BoneOffset[j][k]);
			}
		}
	}

	//for (ui32 i = 0; i < scene->mNumMeshes; i++) {
	//	const aiMesh* mesh = scene->mMeshes[i];

	//	if (!mesh->HasBones()) continue;

	//	str->I16(numBones);

	//	// Global Inverse Transform
	//	for (ui32 j = 0; j < 4; j++) {
	//		for (ui32 k = 0; k < 4; k++) {
	//			str->F32(OPMAT4_IDENTITY.cols[j].row[k]);
	//		}
	//	}

	//	for (ui32 i = 0; i < mesh->mNumBones; i++) {
	//		const aiBone* bone = mesh->mBones[i];

	//		str->I16(0);
	//		str->WriteString(bone->mName.C_Str());

	//		// Bind Bose
	//		for (ui32 j = 0; j < 4; j++) {
	//			for (ui32 k = 0; k < 4; k++) {
	//				str->F32(OPMAT4_IDENTITY.cols[j].row[k]);
	//			}
	//		}

	//		// Offset
	//		for (ui32 j = 0; j < 4; j++) {
	//			for (ui32 k = 0; k < 4; k++) {
	//				str->F32(bone->mOffsetMatrix[j][k]);
	//			}
	//		}
	//	}
	//}

	str->Reset();
	OPloaderOPskeletonLoad(str, &result);

	return result;
}

OPskeletonAnimationResult OPexporter::LoadAnimations(OPstream* stream) {
	OPskeletonAnimationResult result;	
	result.AnimationsCount = scene->mNumAnimations;
	result.Animations = OPALLOC(OPskeletonAnimation*, scene->mNumAnimations);
	result.AnimationNames = OPALLOC(OPchar*, scene->mNumAnimations);

	for (ui32 i = 0; i < scene->mNumAnimations; i++) {
		const aiAnimation* animation = scene->mAnimations[i];
		
		result.Animations[i] = OPNEW(OPskeletonAnimation());
		result.AnimationNames[i] = OPstringCopy(animation->mName.C_Str());

		result.Animations[i]->BoneCount = animation->mNumChannels;
		result.Animations[i]->FrameCount = (ui32)animation->mDuration;

		//writeI16(&animFile, animation->mNumChannels);
		//writeString(&animFile, animation->mName.C_Str());
		//writeU32(&animFile, (ui32)animation->mDuration);

		//for (ui32 j = 0; j < scene->mNumMeshes; j++) {
		//	const aiMesh* mesh = scene->mMeshes[j];
		//}

		//for (ui32 j = 0; j < animation->mNumChannels; j++) {
		//	const aiNodeAnim* data = animation->mChannels[j];

		//}

	}
	return result;
}

void OPexporter::Export(const OPchar* output) {
	OPchar* out;
	if (output == NULL) {
		out = OPstringCopy(path);
	}
	else {
		out = OPstringCopy(output);
	}

	OPstring check = OPstring(out);
	check.ToLower();
	OPint contains = check.Contains(".opm");
	if (contains > 0) {
		out[contains] = NULL;
	}

	// Now we can access the file's contents
	OPchar* outputFinal = OPstringCreateMerged(out, ".opm");

	_write(outputFinal);

	OPfree(out);
}

void OPexporter::Export() {
	Export(NULL);
}

void OPexporter::_setFeatures() {
	OPbzero(features, sizeof(ui32) * MAX_FEATURES);
	features[Model_Positions] = 1;
	features[Model_Normals] = Feature_Normals;
	features[Model_UVs] = Feature_UVs;
	features[Model_Tangents] = Feature_Tangents;
	features[Model_Bitangents] = Feature_BiTangents;
	features[Model_Indices] = 1;
	features[Model_Colors] = Feature_Colors;
	features[Model_Bones] = Feature_Bones;
	features[Model_Skinning] = Feature_Bones;
	features[Model_Skeletons] = Export_Skeleton;
	features[Model_Animations] = Export_Animations;
	features[Model_Meta] = 0;


	// All of the meshes must have the same layout
	// We'll turn features off if we have to
	for (ui32 i = 0; i < scene->mNumMeshes; i++) {
		aiMesh* mesh = scene->mMeshes[i];

		ui32 stride = 0;
		if (features[Model_Normals] && !mesh->HasNormals()) {
			OPlogErr("Mesh %d didn't have 'Normals' to add: '%s'", i, mesh->mName.C_Str());
			features[Model_Normals] = 0;
		}
		if (features[Model_UVs] && !mesh->HasTextureCoords(0)) {
			OPlogErr("Mesh %d didn't have 'UVs' to add: '%s'", i, mesh->mName.C_Str());
			features[Model_UVs] = 0;
		}
		if (features[Model_Tangents] && !mesh->HasTangentsAndBitangents()) {
			OPlogErr("Mesh %d didn't have 'Tangents' to add: '%s'", i, mesh->mName.C_Str());
			features[Model_Tangents] = 0;
		}
		if (features[Model_Bitangents] && !mesh->HasTangentsAndBitangents()) {
			OPlogErr("Mesh %d didn't have 'Bitangents' to add: '%s'", i, mesh->mName.C_Str());
			features[Model_Bitangents] = 0;
		}
		if (features[Model_Bones] && !mesh->HasBones()) {
			OPlogErr("Mesh %d didn't have 'Bones' to add: '%s'", i, mesh->mName.C_Str());
			//features[Model_Bones] = 0;
		}
	}
}

ui32 OPexporter::_getFeaturesFlag() {
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

	return featureFlags;
}

ui32 OPexporter::_getTotalVertices() {
	ui32 totalVerticesEntireModel = 0;

	for (ui32 i = 0; i < scene->mNumMeshes; i++) {
		aiMesh* mesh = scene->mMeshes[i];
		if (features[Model_Bones] && !mesh->HasBones()) {
			continue;
		}

		for (ui32 j = 0; j < mesh->mNumFaces; j++) {
			aiFace face = mesh->mFaces[j];
			totalVerticesEntireModel += face.mNumIndices;
		}
	}

	return totalVerticesEntireModel;
}

ui32 OPexporter::_getTotalIndices() {
	ui32 totalIndicesEntireModel = 0;

	for (ui32 i = 0; i < scene->mNumMeshes; i++) {
		aiMesh* mesh = scene->mMeshes[i];
		if (features[Model_Bones] && !mesh->HasBones()) {
			continue;
		}

		for (ui32 j = 0; j < mesh->mNumFaces; j++) {
			aiFace face = mesh->mFaces[j];
			if (face.mNumIndices == 3) {
				totalIndicesEntireModel += 3;
			}
			else if (face.mNumIndices == 4) {
				totalIndicesEntireModel += 6;
			}
		}
	}

	return totalIndicesEntireModel;
}

ui32 OPexporter::_getTotalVertices(aiMesh* mesh) {
	ui32 totalVertices = 0;
	for (ui32 j = 0; j < mesh->mNumFaces; j++) {
		aiFace face = mesh->mFaces[j];
		totalVertices += face.mNumIndices;
	}
	return totalVertices;
}

ui32 OPexporter::_getTotalIndices(aiMesh* mesh) {
	ui32 totalIndices = 0;
	for (ui32 j = 0; j < mesh->mNumFaces; j++) {
		aiFace face = mesh->mFaces[j];
		if (face.mNumIndices == 3) {
			totalIndices += 3;
		}
		else if (face.mNumIndices == 4) {
			totalIndices += 6;
		}
	}
	return totalIndices;
}


void OPexporter::_setBoneData(aiMesh* mesh) {
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

	boneWeights = (OPfloat*)OPallocZero(sizeof(OPfloat) * mesh->mNumVertices * 4);
	boneIndices = (i32*)OPallocZero(sizeof(i32) * mesh->mNumVertices * 4);
	for (ui32 boneInd = 0; boneInd < mesh->mNumBones; boneInd++) {
		const aiBone* bone = mesh->mBones[boneInd];
		string BoneName(bone->mName.data);
		ui32 index = boneMapping[BoneName];

		for (int boneWeightInd = 0; boneWeightInd < bone->mNumWeights; boneWeightInd++) {
			const aiVertexWeight* weight = &bone->mWeights[boneWeightInd];
			ui32 offset = boneCounts[weight->mVertexId]++;

			boneWeights[(weight->mVertexId * 4) + offset] = weight->mWeight;
			boneIndices[(weight->mVertexId * 4) + offset] = index;
		}
	}

	OPfree(boneCounts);
}

void OPexporter::_writeMeshData(ofstream* myFile) {

	OPlogInfo("Model Export\n========================");
	ui32 offset = 0;
	
	_loadBones();

	for (ui32 i = 0; i < scene->mNumMeshes; i++) {
		aiMesh* mesh = scene->mMeshes[i];

		if (features[Model_Bones] && !mesh->HasBones()) {
			continue;
		}

		// Mesh name
		writeString(myFile, mesh->mName.C_Str());

		// Total Vertices and Indices in Mesh
		ui32 totalVertices = _getTotalVertices(mesh);
		ui32 totalIndices = _getTotalIndices(mesh);
		writeU32(myFile, totalVertices);
		writeU32(myFile, totalIndices);


		OPboundingBox3D boundingBox;


		if (mesh->HasBones() && Feature_Bones) {
			_setBoneData(mesh);
		}

		for (ui32 j = 0; j < mesh->mNumFaces; j++) {
			aiFace face = mesh->mFaces[j];
			if (face.mNumIndices > 4) {
				OPlogErr("Only Supporting 3 and 4 point faces.");
				continue;
			}

			aiVector3D verts[4];
			aiVector3D normals[4];
			aiVector3D uvs[4];
			aiColor4D colors[4];
			aiVector3D bitangents[4];
			aiVector3D tangents[4];
			BoneWeight boneWeightIndex[4];

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
				if (mesh->HasBones() && Feature_Bones) {
					boneWeightIndex[k].weights[0] = boneWeights[face.mIndices[k] * 4 + 0];
					boneWeightIndex[k].weights[1] = boneWeights[face.mIndices[k] * 4 + 1];
					boneWeightIndex[k].weights[2] = boneWeights[face.mIndices[k] * 4 + 2];
					boneWeightIndex[k].weights[3] = boneWeights[face.mIndices[k] * 4 + 3];
					boneWeightIndex[k].bones[0] = boneIndices[face.mIndices[k] * 4 + 0];
					boneWeightIndex[k].bones[1] = boneIndices[face.mIndices[k] * 4 + 1];
					boneWeightIndex[k].bones[2] = boneIndices[face.mIndices[k] * 4 + 2];
					boneWeightIndex[k].bones[3] = boneIndices[face.mIndices[k] * 4 + 3];
				}
			}

			// Write each vertex
			for (ui32 k = 0; k < face.mNumIndices; k++) {

				// Position
				writeF32(myFile, verts[k].x * scale);
				writeF32(myFile, verts[k].y * scale);
				writeF32(myFile, verts[k].z * scale);

				// Normal
				if (mesh->HasNormals() && features[Model_Normals]) {
					writeF32(myFile, normals[k].x);
					writeF32(myFile, normals[k].y);
					writeF32(myFile, normals[k].z);
				}

				if (mesh->HasTangentsAndBitangents() && features[Model_Tangents]) {
					writeF32(myFile, tangents[k].x);
					writeF32(myFile, tangents[k].y);
					writeF32(myFile, tangents[k].z);
				}

				if (mesh->HasTangentsAndBitangents() && features[Model_Bitangents]) {
					writeF32(myFile, bitangents[k].x);
					writeF32(myFile, bitangents[k].y);
					writeF32(myFile, bitangents[k].z);
				}

				if (mesh->HasTextureCoords(0) && features[Model_UVs]) {
					writeF32(myFile, uvs[k].x);
					writeF32(myFile, uvs[k].y);
				}

				if (mesh->HasBones() && features[Model_Bones]) {
					// TODO
					writeU16(myFile, boneWeightIndex[k].bones[0]);
					writeU16(myFile, boneWeightIndex[k].bones[1]);
					writeU16(myFile, boneWeightIndex[k].bones[2]);
					writeU16(myFile, boneWeightIndex[k].bones[3]);
					writeF32(myFile, boneWeightIndex[k].weights[0]);
					writeF32(myFile, boneWeightIndex[k].weights[1]);
					writeF32(myFile, boneWeightIndex[k].weights[2]);
					writeF32(myFile, boneWeightIndex[k].weights[3]);
				}

				if (mesh->HasVertexColors(0) && features[Model_Colors]) {
					writeF32(myFile, colors[k].r);
					writeF32(myFile, colors[k].g);
					writeF32(myFile, colors[k].b);
				}
				else if (features[Model_Colors]) {
					writeF32(myFile, 0);
					writeF32(myFile, 0);
					writeF32(myFile, 0);
				}
			}
		}

		if (mesh->HasBones() && Feature_Bones) {
			if (boneWeights != NULL) OPfree(boneWeights);
			if (boneIndices != NULL) OPfree(boneIndices);
		}

		OPlogInfo(" Mesh[%d] Offset %d", i, offset);

		for (ui32 j = 0; j < mesh->mNumFaces; j++) {
			aiFace face = mesh->mFaces[j];
			if (face.mNumIndices > 4) {
				OPlogErr("Only Supporting 3 and 4 point faces.");
				continue;
			}

			if (indexSize == OPindexSize::INT) {
				if (face.mNumIndices == 3) {
					//OPlog("Triangle");
					writeU32(myFile, offset++);
					writeU32(myFile, offset++);
					writeU32(myFile, offset++);
				}
				else {
					//OPlog("Quad");
					writeU32(myFile, offset + 0);
					writeU32(myFile, offset + 1);
					writeU32(myFile, offset + 2);
					writeU32(myFile, offset + 0);
					writeU32(myFile, offset + 2);
					writeU32(myFile, offset + 3);
					offset += 4;
				}
			}
			else {
				if (face.mNumIndices == 3) {
					//OPlog("Triangle");
					writeU16(myFile, offset++);
					writeU16(myFile, offset++);
					writeU16(myFile, offset++);
				}
				else {
					//OPlog("Quad");
					writeU16(myFile, offset + 0);
					writeU16(myFile, offset + 1);
					writeU16(myFile, offset + 2);
					writeU16(myFile, offset + 0);
					writeU16(myFile, offset + 2);
					writeU16(myFile, offset + 3);
					offset += 4;
				}
			}
		}



		writeF32(myFile, boundingBox.min.x);
		writeF32(myFile, boundingBox.min.y);
		writeF32(myFile, boundingBox.min.z);
		writeF32(myFile, boundingBox.max.x);
		writeF32(myFile, boundingBox.max.y);
		writeF32(myFile, boundingBox.max.z);

		if (existingModel != NULL && existingModel->meshes[i].materialDesc != NULL) {

			if (existingModel->meshes[i].materialDesc->diffuse != NULL) {
				writeString(myFile, existingModel->meshes[i].materialDesc->diffuse);
			}
			else {
				writeString(myFile, "");
			}

			writeString(myFile, "");
			writeString(myFile, "");
			writeString(myFile, "");
			writeString(myFile, "");
			writeString(myFile, "");
			writeString(myFile, "");
			writeString(myFile, "");
			writeString(myFile, "");
			writeString(myFile, "");
			writeString(myFile, "");
			writeString(myFile, "");
			writeString(myFile, "");
			writeString(myFile, "");
		}
		else {
			writeString(myFile, "");
			writeString(myFile, "");
			writeString(myFile, "");
			writeString(myFile, "");
			writeString(myFile, "");
			writeString(myFile, "");
			writeString(myFile, "");
			writeString(myFile, "");
			writeString(myFile, "");
			writeString(myFile, "");
			writeString(myFile, "");
			writeString(myFile, "");
			writeString(myFile, "");
			writeString(myFile, "");
		}

		// Write Meta Data

		//if (model != NULL) {
		//	writeU32(myFile, model->meshes[i].meshMeta->count);
		//	for (ui32 j = 0; j < model->meshes[i].meshMeta->count; j++) {
		//		writeU32(myFile, (ui32)model->meshes[i].meshMeta->metaType[j]);
		//		OPchar* test = model->meshes[i].meshMeta->data[j]->String();
		//		writeString(myFile, test);
		//		//write(&myFile, model->meshes[i].meshMeta->data->Data, model->meshes[i].meshMeta->data->Length);
		//	}
		//}
		//else {
			writeU32(myFile, 0);
		//}

	}

	OPlogInfo(" Offset %d", offset);

}

void OPexporter::_write(const OPchar* outputFinal) {

	ofstream myFile(outputFinal, ios::binary);

	//OPstream* str = OPstream::Create(1024);

	_setFeatures();

	writeU16(&myFile, 3); // OPM File Format Version

	writeString(&myFile, scene->mRootNode->mName.C_Str()); // Model name

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


	// Features in the OPM
	ui32 featureFlags = _getFeaturesFlag();
	writeU32(&myFile, featureFlags);


	// Vertex Mode
	// 1 == Vertex Stride ( Pos/Norm/Uv )[]
	// 2 == Vertex Arrays ( Pos )[] ( Norm )[] ( Uv )[]
	writeU16(&myFile, 1);


	ui32 totalVerticesEntireModel = _getTotalVertices();
	ui32 totalIndicesEntireModel = _getTotalIndices();

	writeU32(&myFile, totalVerticesEntireModel);
	writeU32(&myFile, totalIndicesEntireModel);

	// Index Size SHORT (16) or INT (32)
	indexSize = OPindexSize::INT;
	writeU8(&myFile, (ui8)indexSize);

	_writeMeshData(&myFile);

	myFile.close();

	// Now create skeleton
	if (Export_Skeleton) {
		_writeSkeleton(outputFinal);
	}

	// Now create animations
	if (Export_Animations && scene->HasAnimations()) {
		_writeAnimations(outputFinal);
	}

	OPlog(output);
}

void OPexporter::_writeSkeleton(const OPchar* outputFinal) {
	for (ui32 i = 0; i < scene->mNumMeshes; i++) {
		const aiMesh* mesh = scene->mMeshes[i];
		char buffer[20];
        sprintf(buffer, "%d", i);
		OPchar* skelFileNum = OPstringCreateMerged(outputFinal, buffer);
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

void OPexporter::_writeAnimations(const OPchar* outputFinal) {
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

ui32 GetAvailableTracks(const OPchar* filename, OPchar** buff, double* durations, ui32 max) {
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
}
