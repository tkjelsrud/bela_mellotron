#ifndef VOICE_H
#define VOICE_H

#include <vector>
#include <map>
#include "Sample.h"  // Assuming Sample is defined in your main file or a separate Sample.h file

class Voice {
public:
    bool active = false;
    int sampleIndex = -1;
    float position = 0.0;
    float amplitude = 1.0;
    float frequency = 1.0;
    float attackTime = 0.01;   // Attack time in seconds
    float releaseTime = 0.2;   // Release time in seconds
    float currentAmplitude = 0.0;  // Current envelope value
    bool releasing = false;    // Whether the note is in release phase
    bool looping = true;       // Whether to loop the sample when it reaches the end

    // Sample rate of the project
    float sampleRate;

    Voice(float sr);

    // Start playing the sample with an attack envelope
    void start(int newSampleIndex, float newAmplitude = 1.0);

    // Stop playing the sample with a release envelope
    void stop();

    // Process the audio, applying looping and envelope
    void update(float* outputBuffer, unsigned int bufferSize, const Sample& sample);
};

#endif // VOICE_H