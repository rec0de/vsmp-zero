static unsigned char nearestPaletteColor(unsigned char pixel) {
  if(pixel > BPP_CLIP)
    return 255;
  return ((pixel + BPP_BIAS) / BPP_MUL) * BPP_MUL;
}

static unsigned char clippedAdd(unsigned char base, int8_t bias) {
  int16_t intermediate = bias + base;
  
  if(intermediate > 255)
    return 255;
  else if(intermediate < 0)
    return 0;
  else
    return (unsigned char) intermediate;
}

static int8_t quantizePixel(unsigned char *buf, uint idx) {
  unsigned char oldpixel = buf[idx];
  unsigned char newpixel = nearestPaletteColor(oldpixel);
  buf[idx] = newpixel;
  return (int8_t) oldpixel - newpixel;
}

static void diffuseError(unsigned char *buf, uint idx, int8_t error, int8_t weight, int8_t base) {
  buf[idx] = clippedAdd(buf[idx], (int8_t) ((((int) error) * weight) / base));
}

static uint8_t* loadNoise() {
  FILE *in = fopen("bluenoise.bin", "r");
  if (!in) {
    printf("Failed to open noise file bluenoise.bin\n");
    exit(0);
  }

  uint8_t *buffer = malloc(128 * 128 * sizeof(uint8_t));
  fread(buffer, sizeof(uint8_t), 128 * 128, in);
  fclose(in);
  return buffer;
}

// Floyd-Steinberg dithering
static void floydSteinberg(unsigned char *frameBuf, int linesize, int width, int height) {
  uint32_t i,j,idx;
  int8_t quantError;
    
  for(j = 0; j < height; j++) {
    for(i = 0; i < width; i++) {
      idx = j * linesize + i;
      quantError = quantizePixel(frameBuf, idx);

      if(i != width-1)
        diffuseError(frameBuf, idx + 1, quantError, 7, 16);

      if(j != height-1) {
        diffuseError(frameBuf, idx + linesize, quantError, 5, 16);

        if(i != width-1)
          diffuseError(frameBuf, idx + linesize + 1, quantError, 1, 16);
        if(i != 0)
          diffuseError(frameBuf, idx + linesize - 1, quantError, 3, 16);
      }
    }
  }
}

// Serpentine Floyd-Steinberg dithering
// Essentially reverses dithering direction every line for more even texture
static void floydSteinbergSerpentine(unsigned char *frameBuf, int linesize, int width, int height) {
  uint32_t i,j,idx;
  int8_t quantError;
    
  for(j = 0; j < height; j++) {
    for(i = 0; i < width; i++) {
      idx = j * linesize + i;
      quantError = quantizePixel(frameBuf, idx);

      if(i != width-1)
        diffuseError(frameBuf, idx + 1, quantError, 7, 16);

      if(j != height-1) {
        diffuseError(frameBuf, idx + linesize, quantError, 5, 16);

        if(i != width-1)
          diffuseError(frameBuf, idx + linesize + 1, quantError, 1, 16);
        if(i != 0)
          diffuseError(frameBuf, idx + linesize - 1, quantError, 3, 16);
      }
    }

    j++;
    if(j == height)
      continue;

    for(i = width; i > 0; i--) {
      idx = j * linesize + i - 1;
      quantError = quantizePixel(frameBuf, idx); // For cool glitch, quantize everything to 0 here

      if(i != 0)
        diffuseError(frameBuf, idx - 1, quantError, 7, 16);

      if(j != height-1) {
        diffuseError(frameBuf, idx + linesize, quantError, 5, 16);

        if(i != 0)
          diffuseError(frameBuf, idx + linesize - 1, quantError, 1, 16);
        if(i != width - 1)
          diffuseError(frameBuf, idx + linesize + 1, quantError, 3, 16);
      }
    }
  }
}

static void interleavedGradient(unsigned char *frameBuf, int linesize, int width, int height) {
  const float c1 = 52.9829189;
  const float cx = 0.06711056;
  const float cy = 0.00583715;
  uint32_t i,j,idx;
  uint8_t noise;
  float inner, outer;

  for(j = 0; j < height; j++) {
    for(i = 0; i < width; i++) {
      idx = j * linesize + i;
      inner = cx * i + cy * j;
      outer = c1 * (inner - (int) inner);
      noise = (uint8_t) ((outer - (int) outer) * BPP_MUL);

      frameBuf[idx] = clippedAdd(frameBuf[idx], noise - BPP_BIAS);
      quantizePixel(frameBuf, idx);
    }
  }
}

static void blueNoise(unsigned char *frameBuf, int linesize, int width, int height) {
  static uint8_t initialized = 0;
  static uint8_t *noiseBuf;
  // uhh pretty sure that's not how you're supposed to do things
  // allocating memory that's never freed? I'm sure it'll be fine...
  if(!initialized) {
    noiseBuf = loadNoise();
    initialized = 1;
  }

  uint32_t i,j,idx,noiseidx;
  uint8_t noise;

  for(j = 0; j < height; j++) {
    for(i = 0; i < width; i++) {
      idx = j * linesize + i;
      noiseidx = (j % 128) * 128 + (i % 128);
      
      noise = (((int) noiseBuf[noiseidx]) * BPP_MUL) >> 8;
      frameBuf[idx] = clippedAdd(frameBuf[idx], noise - BPP_BIAS);
      quantizePixel(frameBuf, idx);
    }
  }
}

static void whiteNoise(unsigned char *frameBuf, int linesize, int width, int height) {
  uint32_t i,j,idx;

  int8_t *randomMask = (int8_t *) malloc(width * height * sizeof(int8_t));
  FILE *devRandom = fopen("/dev/urandom", "r");  

  fread(randomMask, sizeof(int8_t), width * height, devRandom);
  fclose(devRandom);
  
    
  for(j = 0; j < height; j++) {
    for(i = 0; i < width; i++) {
      idx = j * linesize + i;
      frameBuf[idx] = clippedAdd(frameBuf[idx], randomMask[j * width + i]);
      quantizePixel(frameBuf, idx);
    }
  }

  free(randomMask);
}

static void atkinson(unsigned char *frameBuf, int linesize, int width, int height) {
  uint32_t i,j,idx;
  int8_t quantError;
    
  for(j = 0; j < height; j++) {
    for(i = 0; i < width; i++) {
      idx = j * linesize + i;
      quantError = quantizePixel(frameBuf, idx);

      // Same row diffusion
      if(i != width-1)
        diffuseError(frameBuf, idx+1, quantError, 1, 8);
      if(i < width-2)
        diffuseError(frameBuf, idx+2, quantError, 1, 8);

      if(j == height-1)
        continue;

      // Next row diffusion      
      diffuseError(frameBuf, idx + linesize, quantError, 1, 8);
      if(i != 0)
        diffuseError(frameBuf, idx + linesize - 1, quantError, 1, 8);
      if(i != width-1)
        diffuseError(frameBuf, idx + linesize + 1, quantError, 1, 8);

      // Third row diffusion
      if(j != height-2)
        diffuseError(frameBuf, idx + linesize * 2, quantError, 1, 8);
    }
  }
}

static void fullSierra(unsigned char *frameBuf, int linesize, int width, int height) {
  uint32_t i,j,idx;
  int8_t quantError;
    
  for(j = 0; j < height; j++) {
    for(i = 0; i < width; i++) {
      idx = j * linesize + i;
      quantError = quantizePixel(frameBuf, idx);

      // Same row diffusion
      if(i != width-1)
        diffuseError(frameBuf, idx+1, quantError, 5, 32);
      if(i < width-2)
        diffuseError(frameBuf, idx+2, quantError, 3, 32);

      if(j == height-1)
        continue;

      // Next row diffusion      
      diffuseError(frameBuf, idx + linesize, quantError, 5, 32);
      if(i != 0)
        diffuseError(frameBuf, idx + linesize - 1, quantError, 4, 32);
      if(i > 1)
        diffuseError(frameBuf, idx + linesize - 2, quantError, 2, 32);
      if(i != width-1)
        diffuseError(frameBuf, idx + linesize + 1, quantError, 4, 32);
      if(i < width-2)
        diffuseError(frameBuf, idx + linesize + 2, quantError, 2, 32);

      if(j == height-2)
        continue;

      // Third row diffusion
      diffuseError(frameBuf, idx + linesize * 2, quantError, 3, 32);
      if(i != 0)
        diffuseError(frameBuf, idx + linesize * 2 - 1, quantError, 2, 32);
      if(i != width-1)
        diffuseError(frameBuf, idx + linesize * 2 + 1, quantError, 2, 32);
    }
  }
}

static void twoRowSierra(unsigned char *frameBuf, int linesize, int width, int height) {
  uint32_t i,j,idx;
  int8_t quantError;
    
  for(j = 0; j < height; j++) {
    for(i = 0; i < width; i++) {
      idx = j * linesize + i;
      quantError = quantizePixel(frameBuf, idx);

      // Same row diffusion
      if(i != width-1)
        diffuseError(frameBuf, idx+1, quantError, 4, 16);
      if(i < width-2)
        diffuseError(frameBuf, idx+2, quantError, 3, 16);

      if(j == height-1)
        continue;

      // Next row diffusion      
      diffuseError(frameBuf, idx + linesize, quantError, 3, 16);
      if(i != 0)
        diffuseError(frameBuf, idx + linesize - 1, quantError, 2, 16);
      if(i > 1)
        diffuseError(frameBuf, idx + linesize - 2, quantError, 1, 16);
      if(i != width-1)
        diffuseError(frameBuf, idx + linesize + 1, quantError, 2, 16);
      if(i < width-2)
        diffuseError(frameBuf, idx + linesize + 2, quantError, 1, 16);
    }
  }
}

static void stucki(unsigned char *frameBuf, int linesize, int width, int height) {
  uint32_t i,j,idx;
  int8_t quantError;
    
  for(j = 0; j < height; j++) {
    for(i = 0; i < width; i++) {
      idx = j * linesize + i;
      quantError = quantizePixel(frameBuf, idx);

      // Same row diffusion
      if(i != width-1)
        diffuseError(frameBuf, idx+1, quantError, 8, 42);
      if(i < width-2)
        diffuseError(frameBuf, idx+2, quantError, 4, 42);

      if(j == height-1)
        continue;

      // Next row diffusion      
      diffuseError(frameBuf, idx + linesize, quantError, 8, 42);
      if(i != 0)
        diffuseError(frameBuf, idx + linesize - 1, quantError, 4, 42);
      if(i > 1)
        diffuseError(frameBuf, idx + linesize - 2, quantError, 2, 42);
      if(i != width-1)
        diffuseError(frameBuf, idx + linesize + 1, quantError, 4, 42);
      if(i < width-2)
        diffuseError(frameBuf, idx + linesize + 2, quantError, 2, 42);

      if(j == height-2)
        continue;

      // Third row diffusion
      diffuseError(frameBuf, idx + linesize * 2, quantError, 4, 42);
      if(i != 0)
        diffuseError(frameBuf, idx + linesize * 2 - 1, quantError, 2, 42);
      if(i > 1)
        diffuseError(frameBuf, idx + linesize * 2 - 2, quantError, 1, 42);
      if(i != width-1)
        diffuseError(frameBuf, idx + linesize * 2 + 1, quantError, 2, 42);
      if(i < width-2)
        diffuseError(frameBuf, idx + linesize * 2 + 2, quantError, 1, 42);
    }
  }
}