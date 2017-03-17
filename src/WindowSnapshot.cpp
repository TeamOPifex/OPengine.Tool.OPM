#include "WindowSnapshot.h"

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

WindowSnapshot::WindowSnapshot() {
	ui32 width = OPRENDERER_ACTIVE->OPWINDOW_ACTIVE->WindowWidth;
	ui32 height = OPRENDERER_ACTIVE->OPWINDOW_ACTIVE->WindowHeight;

	/* Allocate our buffer for the image */
	if ((image = (ui8*)OPalloc(3 * width*height * sizeof(char))) == NULL) {
		OPlogErr("Failed to allocate memory for image");
		return;
	}

	if ((image2 = (ui8*)OPalloc(3 * width*height * sizeof(char))) == NULL) {
		OPlogErr("Failed to allocate memory for image");
		return;
	}
}

void WindowSnapshot::Snapshot(OPstring* out) {
	int i, j;
	FILE *fptr;
	static int counter = 0; /* This supports animation sequences */
	char fname[32];

	ui32 width = OPRENDERER_ACTIVE->OPWINDOW_ACTIVE->WindowWidth;
	ui32 height = OPRENDERER_ACTIVE->OPWINDOW_ACTIVE->WindowHeight;

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