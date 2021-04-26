static int initDisplay() { return 0; }
static void teardownDisplay() {}
static void clearDisplay() {}

static void pixelPush(unsigned char *frameBuf, int linesize, int width, int height) {
	static uint index = 0;
	FILE *f;
	int i;
	char frame_filename[1024];

	snprintf(frame_filename, sizeof(frame_filename), "%s-%d.pgm", "frame", index);
	index++;
	f = fopen(frame_filename,"w");

	// writing the minimal required header for a pgm file format
	// portable graymap format -> https://en.wikipedia.org/wiki/Netpbm_format#PGM_example
	fprintf(f, "P5\n%d %d\n%d\n", width, height, 255);

	// writing line by line
	for (i = 0; i < height; i++)
	    fwrite(frameBuf + i * linesize, 1, width, f);
	fclose(f);

	printf("Wrote frame pgm file\n");
}
