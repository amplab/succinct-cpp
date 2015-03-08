#ifndef SAMPLING_SCHEME_H
#define SAMPLING_SCHEME_H

typedef enum sampling_scheme {
    FLAT_SAMPLE_BY_INDEX = 0,
    FLAT_SAMPLE_BY_VALUE = 1,
    LAYERED_SAMPLE_BY_INDEX = 2,
    OPPORTUNISTIC_LAYERED_SAMPLE_BY_INDEX = 3
} SamplingScheme;

#endif
