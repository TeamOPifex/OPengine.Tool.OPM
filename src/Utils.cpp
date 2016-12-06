#include "Utils.h"

bool IsImageFile(const OPchar* ext) {
	OPstring str = OPstring(ext);
	str.ToLower();

	const OPchar* fileTypes[7] = {
		".png",
		".jpg",
		".jpeg",
		".tga",
		".bmp",
		".psd",
		".gif"
	};

	for (ui32 i = 0; i < 7; i++) {
		if (str.Equals(fileTypes[i])) {
			return true;
		}
	}

	return false;
}

bool IsAnimationFile(const OPchar* ext) {
	OPstring str = OPstring(ext);
	str.ToLower();

	const OPchar* fileTypes[1] = {
		".txt"
	};

	for (ui32 i = 0; i < 1; i++) {
		if (str.Equals(fileTypes[i])) {
			return true;
		}
	}

	return false;
}

bool IsModelFile(const OPchar* ext) {
	OPstring str = OPstring(ext);
	str.ToLower();

	const OPchar* fileTypes[4] = {
		".opm",
		".fbx",
		".obj",
		".dae"
	};

	for (ui32 i = 0; i < 4; i++) {
		if (str.Equals(fileTypes[i])) {
			return true;
		}
	}

	return false;
}

OPtexture* LoadTexture(const OPchar* dir, const OPchar* tex) {
	if (tex == NULL) return NULL;

	OPstring outputRoot(dir);
	// Get rid of any filenames on the filename path will leave only the directory the file is in
    #ifdef OPIFEX_WINDOWS
	   OPint pos = outputRoot.IndexOfLast('\\');
    #else
	   OPint pos = outputRoot.IndexOfLast('/');
    #endif
	if (pos != -1) {
		outputRoot.Resize(pos + 1, false);
	}

	outputRoot.Add(tex);

	OPtexture* result = (OPtexture*)OPCMAN.LoadFromFile(outputRoot.C_Str());
	return result;
}

OPstring* GetFilenameOPM(const OPchar* filename) {

	// Get just the filename
    #ifdef OPIFEX_WINDOWS
	   const OPchar* ext = strrchr(filename, '\\');
    #else
	   const OPchar* ext = strrchr(filename, '/');
    #endif
	if (ext == NULL) {
		ext = filename;
	}

	OPstring* outputFilename = OPstring::Create(&ext[1]);
	outputFilename->Resize(outputFilename->IndexOfLast('.'), false);
	outputFilename->Add(".opm");

	return outputFilename;
}


OPstring* GetAbsolutePathOPM(const OPchar* filename) {
	OPstring outputRoot = OPstring(filename);
    #ifdef OPIFEX_WINDOWS
    	OPint pos = outputRoot.IndexOfLast('\\');
    #else
    	OPint pos = outputRoot.IndexOfLast('/');
    #endif
	if (pos != -1) {
		outputRoot.Resize(pos + 1, false);
	}

	// Get just the filename
    #ifdef OPIFEX_WINDOWS
	   const OPchar* ext = strrchr(filename, '\\');
    #else
	   const OPchar* ext = strrchr(filename, '/');
    #endif
	if (ext == NULL) {
		ext = filename;
	}

	OPstring outputFilename = OPstring(&ext[1]);
	outputFilename.Resize(outputFilename.IndexOfLast('.'), false);
	outputFilename.Add(".opm");


	OPstring* outputAbsolutePath = outputRoot.Copy();
	outputAbsolutePath->Add(&outputFilename);
	return outputAbsolutePath;
}
