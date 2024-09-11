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
std::map<int, Sample> sampleMap;

// Global voice manager, initialized with a dummy sample rate (will be properly initialized in setup)
VoiceManager voiceManager(0);

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
bool loadSamples(const std::string& folderPath) {
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
                int sampleRate;
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
    if (sampleMap.find(noteNumber) != sampleMap.end()) {
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

    // Load samples from the specified folder
    std::string sampleFolder = "/root/Bela/projects/mellotron/samples/trombone";
    if (!loadSamples(sampleFolder)) {
        return false;
    }

    return true;
}

// Bela render function: handles MIDI input and audio output
void render(BelaContext *context, void *userData) {
    // Process MIDI messages
    while (gMidi.getParser()->numAvailableMessages() > 0) {
        MidiChannelMessage message = gMidi.getParser()->getNextChannelMessage();

        // Log incoming MIDI messages
        //rt_printf("MIDI message received: Type = %d, Note = %d, Velocity = %d\n",
        //          message.getType(), message.getDataByte(0), message.getDataByte(1));

        int noteNumber = message.getDataByte(0);  // Note number is always in message[0]
        int velocity = message.getDataByte(1);    // Velocity is in message[1]

        // Handle noteOn and noteOff messages
        if (message.getType() == kmmNoteOn && velocity > 0) {
            // Note On with velocity > 0
            noteOn(noteNumber, velocity);
        } else if (message.getType() == kmmNoteOff || (message.getType() == kmmNoteOn && velocity == 0)) {
            // Note Off (either explicit noteOff or noteOn with velocity 0)
            rt_printf("Note Off processed for note: %d\n", noteNumber);
            noteOff(noteNumber);
        }
    }

    // Render audio output
    for (unsigned int frame = 0; frame < context->audioFrames; ++frame) {
        float output = 0.0f;
        voiceManager.render(&output, 1, sampleMap);
        for (unsigned int channel = 0; channel < context->audioOutChannels; channel++) {
            audioWrite(context, frame, channel, output);
        }
    }
}

// Bela cleanup function
void cleanup(BelaContext *context, void *userData) {
    // Cleanup resources if necessary
}