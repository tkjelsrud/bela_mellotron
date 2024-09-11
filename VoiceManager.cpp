#include "VoiceManager.h"

int kMaxVoices = 8;

VoiceManager::VoiceManager(float sampleRate) {
    for (int i = 0; i < kMaxVoices; i++) {
        voices.push_back(Voice(sampleRate));
    }
}

void VoiceManager::noteOn(int sampleIndex, float amplitude) {
    for (auto& voice : voices) {
        if (!voice.active) {
            voice.start(sampleIndex, amplitude);
            break;
        }
    }
}

void VoiceManager::noteOff(int sampleIndex) {
    for (auto& voice : voices) {
        if (voice.sampleIndex == sampleIndex && voice.active) {
            voice.stop();  // Start release phase
        }
    }
}

void VoiceManager::render(float* outputBuffer, unsigned int bufferSize, std::map<int, Sample>& sampleMap) {
    for (auto& voice : voices) {
        if (voice.active && sampleMap.find(voice.sampleIndex) != sampleMap.end()) {
            voice.update(outputBuffer, bufferSize, sampleMap[voice.sampleIndex]);
        }
    }
}