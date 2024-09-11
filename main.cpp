#include <Bela.h>
#include <libraries/Midi/Midi.h>
#include <libraries/AudioFile/AudioFile.h>
#include <dirent.h>
#include <map>
#include <string>
#include <vector>
#include "VoiceManager.h"  // Include the refactored VoiceManager

// MIDI device setup
Midi gMidi;
const char* gMidiPort0 = "hw:1,0,0";  // MIDI device port

// Sample map to store the loaded samples
std::map<int, Sample> sampleMapSet1;  // First set of samples
std::map<int, Sample> sampleMapSet2;  // Second set of samples
std::map<int, Sample>* currentSampleMap = &sampleMapSet1;  // Pointer to the currently active set

// Global voice manager, initialized with a dummy sample rate (will be properly initialized in setup)
VoiceManager voiceManager(0);

float fadeOutTime = 0.1;   // Time in seconds for the fade-out (e.g., 10ms)
float fadeInTime = 0.1;    // Time in seconds for the fade-in (e.g., 10ms)
float currentFadeLevel = 1.0;  // Starts fully faded in
bool switchingSamples = false;  // Flag to indicate when switching sample sets

// Function to map note names like A2, A#2, etc. to MIDI note numbers
int mapNoteToMidi(const std::string& noteName) {
    static std::map<std::string, int> noteToMidiMap = {
        {"C", 0}, {"C#", 1}, {"D", 2}, {"D#", 3}, {"E", 4}, {"F", 5},
        {"F#", 6}, {"G", 7}, {"G#", 8}, {"A", 9}, {"A#", 10}, {"B", 11}
    };
    std::string note = noteName.substr(0, noteName.length() - 1);
    int octave = noteName.back() - '0';
    return 12 * (octave + 1) + noteToMidiMap[note];
}

// Load .wav files from a directory and store them in the sampleMap
bool loadSamples(const std::string& folderPath, std::map<int, Sample>& sampleMap) {
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir(folderPath.c_str())) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            std::string filename = ent->d_name;
            if (filename.find(".wav") != std::string::npos) {
                std::string noteName = filename.substr(0, filename.find(".wav"));
                int midiNote = mapNoteToMidi(noteName);

                // Load the .wav file using AudioFileUtilities
                std::vector<std::vector<float>> audioBuffer;
                int sampleRate = 22050;  // Assuming a default sample rate (replace if needed)
                int startFrame = 0;
                int endFrame = -1;
                
                audioBuffer = AudioFileUtilities::load((folderPath + "/" + filename).c_str(), endFrame - startFrame, startFrame);

                if (!audioBuffer.empty()) {
                    sampleMap[midiNote] = {audioBuffer, sampleRate, (int)audioBuffer[0].size()};
                } else {
                    rt_printf("Error loading %s\n", filename.c_str());
                }
            }
        }
        closedir(dir);
    } else {
        rt_printf("Could not open directory %s\n", folderPath.c_str());
        return false;
    }
    return true;
}
// Function called when a MIDI note on is received
void noteOn(int noteNumber, int velocity) {
    // Dereference the currentSampleMap pointer to access the map
    if (currentSampleMap->find(noteNumber) != currentSampleMap->end()) {
        float velocityAmplitude = velocity / 127.0f;  // Map velocity to amplitude (0-1)
        voiceManager.noteOn(noteNumber, velocityAmplitude);
    }
}

// Function called when a MIDI note off is received
void noteOff(int noteNumber) {
    // Check if the note is in the active voices and stop it
    voiceManager.noteOff(noteNumber);
}

// Bela setup function
bool setup(BelaContext *context, void *userData) {
    // Initialize the VoiceManager with the sample rate from Bela context
    voiceManager = VoiceManager(context->audioSampleRate);

    // Initialize MIDI device
    if (gMidi.readFrom(gMidiPort0) < 0) {
        rt_printf("Unable to read from MIDI port %s\n", gMidiPort0);
        return false;
    }
    gMidi.writeTo(gMidiPort0);
    gMidi.enableParser(true);

    // Load the first set of samples
    std::string sampleFolderSet1 = "/root/Bela/projects/mellotron/samples/trombone";
    if (!loadSamples(sampleFolderSet1, sampleMapSet1)) {
        return false;
    }

    // Load the second set of samples (another folder with different sounds)
    std::string sampleFolderSet2 = "/root/Bela/projects/mellotron/samples/flute";
    if (!loadSamples(sampleFolderSet2, sampleMapSet2)) {
        return false;
    }

    return true;
}

// Bela render function: handles MIDI input and audio output
void render(BelaContext *context, void *userData) {
    // Process MIDI messages
    while (gMidi.getParser()->numAvailableMessages() > 0) {
        MidiChannelMessage message = gMidi.getParser()->getNextChannelMessage();

        int noteNumber = message.getDataByte(0);  // Note number is in message[0]
        int velocity = message.getDataByte(1);    // Velocity or CC value is in message[1]

        // Handle Control Change (CC) messages
        if (message.getType() == kmmControlChange) {
            int ccNumber = noteNumber;  // CC number is in the first data byte
            int ccValue = velocity;     // CC value is in the second data byte

            // Assuming we're using CC #1 for switching sample sets
            if (ccNumber == 1) {
                if (ccValue < 64) {
                    currentSampleMap = &sampleMapSet1;  // Switch to set 1
                } else {
                    currentSampleMap = &sampleMapSet2;  // Switch to set 2
                }
            }
        }

        // Handle noteOn and noteOff messages
        if (message.getType() == kmmNoteOn && velocity > 0) {
            noteOn(noteNumber, velocity);
        } else if (message.getType() == kmmNoteOff || (message.getType() == kmmNoteOn && velocity == 0)) {
            noteOff(noteNumber);
        }
    }

    // Render mono audio output
    for (unsigned int frame = 0; frame < context->audioFrames; ++frame) {
        float output = 0.0f;

        // Render audio using the currently selected sample map
        voiceManager.render(&output, 1, *currentSampleMap);

        // Write the mono output to all audio output channels
        for (unsigned int channel = 0; channel < context->audioOutChannels; channel++) {
            audioWrite(context, frame, channel, output);
        }
    }
}
// Bela cleanup function
void cleanup(BelaContext *context, void *userData) {
    // Cleanup resources if necessary
}