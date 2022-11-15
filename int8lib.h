// mostly ported from FastLED

typedef uint8_t fract8;

// add with ceiling
static inline uint8_t qadd8( uint8_t i, uint8_t j)
{
    unsigned int t = i + j;
    if( t > 255) t = 255;
    return t;
}

// subtract with floor
static inline uint8_t qsub8( uint8_t i, uint8_t j)
{
    int t = i - j;
    if( t < 0) t = 0;
    return t;
}

static inline uint8_t scale8_video(uint8_t i, fract8 scale)
{
  uint8_t nonzeroscale = (scale != 0) ? 1 : 0;
  uint8_t j = (i == 0) ? 0 : (((int)i * (int)(scale) ) >> 8) + nonzeroscale;
  return j;
}

uint16_t rand16seed; // really should use extern in a header file, but...

/// Generate an 8-bit random number
static inline uint8_t random8()
{
    rand16seed = (rand16seed * (uint16_t)2053) + (uint16_t)13849;
    // return the sum of the high and low bytes, for better
    //  mixing and non-sequential correlation
    return (uint8_t)(((uint8_t)(rand16seed & 0xFF)) +
                     ((uint8_t)(rand16seed >> 8)));
}

static inline uint8_t random8(uint8_t lim)
{
    uint8_t r = random8();
    r = (r*lim) >> 8;
    return r;
}

static inline uint8_t random8(uint8_t min, uint8_t lim)
{
    uint8_t delta = lim - min;
    uint8_t r = random8(delta) + min;
    return r;
}
