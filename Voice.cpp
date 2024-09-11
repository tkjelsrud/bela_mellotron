#include "Voice.h"

Voice::Voice(float sr) : sampleRate(sr) {}

void Voice::start(int newSampleIndex, float newAmplitude) {
    sampleIndex = newSampleIndex;
    position = 0.0;
    active = true;
    amplitude = newAmplitude;
    currentAmplitude = 0.0; // Start at 0 for attack
    releasing = false;
}

void Voice::stop() {
    releasing = true;
}

void Voice::update(float* outputBuffer, unsigned int bufferSize, const Sample& sample) {
    if (!active) return;

    for (unsigned int n = 0; n < bufferSize; n++) {
        if (position >= sample.length) {
            if (looping) {
                position = 0.0;  // Loop the sample when reaching the end
            } else {
                stop();  // If not looping, stop the voice
                break;
            }
        }

        // Attack phase
        if (!releasing) {
            currentAmplitude += (1.0 / (attackTime * sampleRate));  // Linear attack
            if (currentAmplitude > amplitude) {
                currentAmplitude = amplitude;
            }
        }

        // Release phase
        if (releasing) {
            currentAmplitude -= (1.0 / (releaseTime * sampleRate));  // Linear release
            if (currentAmplitude <= 0.0) {
                currentAmplitude = 0.0;
                active = false;  // Stop the voice after the release phase
                return;  // Exit the function as the voice is inactive
            }
        }

        // Apply the current amplitude to the sample and update position
        outputBuffer[n] += sample.data[0][(int)position] * currentAmplitude;  // Assuming mono
        position += frequency;
    }
}