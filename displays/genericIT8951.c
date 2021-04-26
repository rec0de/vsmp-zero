#include "IT8951.h"
#include "IT8951.c"

static int initDisplay() {
	return IT8951_Init();
}

static void teardownDisplay() {
	IT8951_Cancel();
}

static void wakeDisplay() {
	IT8951SystemRun();
	IT8951WaitForDisplayReady();
}

static void standbyDisplay() {
	IT8951WaitForDisplayReady();
	IT8951Sleep();
}

static void clearDisplay() {
	IT8951Clear();
}

static void pixelPush(unsigned char *frameBuf, int linesize, int width, int height) {
	IT8951LdImgInfo stLdImgInfo;
	IT8951AreaImgInfo stAreaImgInfo;

	//Setting Load image information
	stLdImgInfo.ulStartFBAddr    = (uint32_t) frameBuf; // Pointer to frame buffer
	stLdImgInfo.usRotate         = IT8951_ROTATE_0;
	stLdImgInfo.ulImgBufBaseAddr = gulImgBufAddr; // just leave as is i guess

	//Set Load Area
	// center the frame on the panel as well as possible
	stAreaImgInfo.usX      = ((gstI80DevInfo.usPanelW - width) >> 2) << 1; // Uneven x-offsets mess things up
	stAreaImgInfo.usY      = (gstI80DevInfo.usPanelH - height) / 2;
	stAreaImgInfo.usWidth  = width;
	stAreaImgInfo.usHeight = height;
	stAreaImgInfo.usLinesize = linesize;

	wakeDisplay();
	
	//Load Image from Host to IT8951 Image Buffer
	IT8951HostAreaPackedPixelWrite(&stLdImgInfo, &stAreaImgInfo);

	//Display Area (x,y,w,h) with mode 2 for fast gray clear mode - depends on current waveform 
	IT8951DisplayArea(stAreaImgInfo.usX, stAreaImgInfo.usY, stAreaImgInfo.usWidth, stAreaImgInfo.usHeight, 2);

	standbyDisplay();
}
