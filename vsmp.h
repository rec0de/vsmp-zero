// Set to one to not actually use the display but output to a .pgm file
// Can be used to compile / test on a faster machine
#define DRYRUN 0

#define BITS_PER_PIXEL 4
#define TRANSPORT_BPP 4 // Bit packing used for transfer to the display controller - set equal to or higher than BPP to avoid quality loss. Supported values are 2, 4 and 8
#define FRAMES_PER_HOUR 24 // refresh the display this many times per hour
#define FRAME_STEP_SIZE 1  // on every display refresh, move this many frames forward in the source file
#define WHITE_VALUE 255

// Use RPi hardware acceleration to decode video frames
// requires custom-compiled ffmpeg and a h264 encoded video and no funky pixel format (8bpp grayscale works)
// also requires ~128 MB graphics memory on the pi
#define HWACCEL 0

// Enable lighsense to only update the display if some ambient light is detected
#define LIGHSENSE 0
#define LIGHTSENSE_THRESHOLD 1000000
#define LIGHTSENSE_GPIO_PIN RPI_V2_GPIO_P1_07

/* Choose dithering algorithm, one of:
1. floydSteinberg             very common, slight patterns / artifacts
2. floydSteinbergSerpentine   less artifacts than regular floyd-steinberg, more even dot-spacing
3. interleavedGradient        somewhat similar to Bayer dithering, noticable hatching patterns
4. blueNoise                  a bit more memory-intensive, very organic looking
5. whiteNoise                 naive random-noise dithering, very noisy / grainy
6. fullSierra                 rather large diffusion matrix, good quality
7. twoRowSierra               simplified version of Sierra dithering, more artifacts
8. stucki                     very large diffusion matrix, good quality, slight patterns
9. atkinson                   reduced color-bleed, less detail in light / dark regions

Further reading:
1-2: https://en.wikipedia.org/wiki/Floyd%E2%80%93Steinberg_dithering
3  : https://bartwronski.com/2016/10/30/dithering-part-three-real-world-2d-quantization-dithering/ > Interleaved Gradient Noise
4-5: https://surma.dev/things/ditherpunk
6-9: https://tannerhelland.com/2012/12/28/dithering-eleven-algorithms-source-code.html
*/
#define DITHER floydSteinbergSerpentine

// Automatically calculated definitions, please do not change

// Colour depth conversion things
#define BPP_MUL (256 / ((1 << BITS_PER_PIXEL) - 1))
#define BPP_BIAS (BPP_MUL / 2)
#define BPP_CLIP (255 - BPP_BIAS)

// Transport BPP flag conversion
#if TRANSPORT_BPP == 2
	#define TRANSPORT_BPP_FLAG 0
#elif TRANSPORT_BPP == 4
	#define TRANSPORT_BPP_FLAG 2
#else
	#define TRANSPORT_BPP_FLAG 3
#endif
