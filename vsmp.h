// Set to one to not actually use the display but output to a .pgm file
// Can be used to compile / test on a faster machine
#define DRYRUN 0

#define BITS_PER_PIXEL 4
#define FRAMES_PER_HOUR 24 // refresh the display this many times per hour
#define FRAME_STEP_SIZE 1  // on every display refresh, move this many frames forward in the source file
#define WHITE_VALUE 150

// Use RPi hardware acceleration to decode video frames
// requires custom-compiled ffmpeg and a h264 encoded video and no funky pixel format (8bpp grayscale works)
// also requires ~128 MB graphics memory on the pi
#define HWACCEL 1

// Enable lighsense to only update the display if some ambient light is detected
#define LIGHSENSE 1
#define LIGHTSENSE_THRESHOLD 1000000
#define LIGHTSENSE_GPIO_PIN RPI_V2_GPIO_P1_07

// Colour depth conversion things
#define BPP_MUL (256 / ((1 << BITS_PER_PIXEL) - 1))
#define BPP_BIAS (BPP_MUL / 2)
#define BPP_CLIP (255 - BPP_BIAS)