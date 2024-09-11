#ifndef SAMPLE_H
#define SAMPLE_H

#include <vector>

struct Sample {
    std::vector<std::vector<float>> data;  // Multi-channel sample data
    int sampleRate;  // Sample rate of the audio file
    int length;  // Length of the sample in frames
};

#endif // SAMPLE_H