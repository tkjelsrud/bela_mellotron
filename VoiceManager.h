#ifndef VOICEMANAGER_H
#define VOICEMANAGER_H

#include "Voice.h"
#include <vector>
#include <map>

class VoiceManager {
public:
    VoiceManager(float sampleRate);

    void noteOn(int sampleIndex, float amplitude);
    void noteOff(int sampleIndex);
    void render(float* outputBuffer, unsigned int bufferSize, std::map<int, Sample>& sampleMap);

private:
    std::vector<Voice> voices;
};

#endif // VOICEMANAGER_H