

#ifndef DSP_H
#define DSP_H
typedef struct {
    int32_t b0, b1, b2;
    int32_t a1, a2;
} BiquadCoefficients;

typedef struct {
    int32_t s1, s2;
} BiquadState;

static inline int32_t biquad_process_coeffstate(const BiquadCoefficients *c, BiquadState *s, int32_t x)
{
    int32_t y;

    y = ((int64_t)c->b0 * x >> 30) + s->s1;

    s->s1 = ((int64_t)c->b1 * x >> 30)
          - ((int64_t)c->a1 * y >> 30)
          + s->s2;

    s->s2 = ((int64_t)c->b2 * x >> 30)
          - ((int64_t)c->a2 * y >> 30);

    return y;
}

typedef struct {
    const BiquadCoefficients *coeff;
    BiquadState state;
} BiquadFilter;

static inline int32_t biquad_process(BiquadFilter *f, int32_t x)
{
    return biquad_process_coeffstate(f->coeff, &f->state, x);
}




#endif