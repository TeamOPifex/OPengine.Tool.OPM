#pragma once

#include "./Core/include/OPtypes.h"

#include <iostream>
#include <fstream>
using namespace std;

inline void write(ofstream* stream, void* data, i32 size) {
	stream->write((char*)data, size);
}

inline void writeU8(ofstream* stream, ui8 val) {
	write(stream, &val, sizeof(ui8));
}

inline void writeI8(ofstream* stream, i8 val) {
	write(stream, &val, sizeof(i8));
}

inline void writeF32(ofstream* stream, f32 val) {
	write(stream, &val, sizeof(f32));
}

inline void writeI16(ofstream* stream, i16 val) {
	write(stream, &val, sizeof(i16));
}

inline void writeU16(ofstream* stream, ui16 val) {
	write(stream, &val, sizeof(ui16));
}

inline void writeI32(ofstream* stream, i32 val) {
	write(stream, &val, sizeof(i32));
}

inline void writeU32(ofstream* stream, ui32 val) {
	write(stream, &val, sizeof(ui32));
}

inline void writeString(ofstream* stream, const OPchar* val) {
	ui32 len = strlen(val);
	writeU32(stream, len);
	if (len == 0) return;
	write(stream, (void*)val, len * sizeof(OPchar));
}