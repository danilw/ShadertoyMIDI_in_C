
typedef struct {
    float Oscilators[4];
    float Octaves[4];
    float ADSR[4];
    float FX[4];
} MidiInstrument;

const char* Names[] = {
    "Acoustic Grand Piano",
    "Bright Acoustic Piano",
    "Electric Grand Piano",
    "Honky-tonk Piano",
    "Electric Piano 1",
    "Electric Piano 2",
    "Harpsichord",
    "Clavi",
    "Celesta",
    "Glockenspiel",
    "Music Box",
    "Vibraphone",
    "Marimba",
    "Xylophone",
    "Tubular Bells",
    "Dulcimer",
    "Drawbar Organ",
    "Percussive Organ",
    "Rock Organ",
    "Church Organ",
    "Reed Organ",
    "Accordion",
    "Harmonica",
    "Tango Accordion",
    "Acoustic Guitar (nylon)",
    "Acoustic Guitar (steel)",
    "Electric Guitar (jazz)",
    "Electric Guitar (clean)",
    "Electric Guitar (muted)",
    "Overdriven Guitar",
    "Distortion Guitar",
    "Guitar harmonics",
    "Acoustic Bass",
    "Electric Bass (finger)",
    "Electric Bass (pick)",
    "Fretless Bass",
    "Slap Bass 1",
    "Slap Bass 2",
    "Synth Bass 1",
    "Synth Bass 2",
    "Violin",
    "Viola",
    "Cello",
    "Contrabass",
    "Tremolo Strings",
    "Pizzicato Strings",
    "Orchestral Harp",
    "Timpani",
    "String Ensemble 1",
    "String Ensemble 2",
    "SynthStrings 1",
    "SynthStrings 2",
    "Choir Aahs",
    "Voice Oohs",
    "Synth Voice",
    "Orchestra Hit",
    "Trumpet",
    "Trombone",
    "Tuba",
    "Muted Trumpet",
    "French Horn",
    "Brass Section",
    "SynthBrass 1",
    "SynthBrass 2",
    "Soprano Sax",
    "Alto Sax",
    "Tenor Sax",
    "Baritone Sax",
    "Oboe",
    "English Horn",
    "Bassoon",
    "Clarinet",
    "Piccolo",
    "Flute",
    "Recorder",
    "Pan Flute",
    "Blown Bottle",
    "Shakuhachi",
    "Whistle",
    "Ocarina",
    "Lead 1 (square)",
    "Lead 2 (sawtooth)",
    "Lead 3 (calliope)",
    "Lead 4 (chiff)",
    "Lead 5 (charang)",
    "Lead 6 (voice)",
    "Lead 7 (fifths)",
    "Lead 8 (bass + lead)",
    "Pad 1 (new age)",
    "Pad 2 (warm)",
    "Pad 3 (polysynth)",
    "Pad 4 (choir)",
    "Pad 5 (bowed)",
    "Pad 6 (metallic)",
    "Pad 7 (halo)",
    "Pad 8 (sweep)",
    "FX 1 (rain)",
    "FX 2 (soundtrack)",
    "FX 3 (crystal)",
    "FX 4 (atmosphere)",
    "FX 5 (brightness)",
    "FX 6 (goblins)",
    "FX 7 (echoes)",
    "FX 8 (sci-fi)",
    "Sitar",
    "Banjo",
    "Shamisen",
    "Koto",
    "Kalimba",
    "Bag pipe",
    "Fiddle",
    "Shanai",
    "Tinkle Bell",
    "Agogo",
    "Steel Drums",
    "Woodblock",
    "Taiko Drum",
    "Melodic Tom",
    "Synth Drum",
    "Reverse Cymbal",
    "Guitar Fret Noise",
    "Breath Noise",
    "Seashore",
    "Bird Tweet",
    "Telephone Ring",
    "Helicopter",
    "Applause",
    "Gunshot"
};

MidiInstrument MidiInstrument_CreateDefault() {
    MidiInstrument instrument;
    instrument.Oscilators[0] = 0.5;
    instrument.Oscilators[1] = 0.0;
    instrument.Oscilators[2] = 0.0;
    instrument.Oscilators[3] = 0.0;

    instrument.Octaves[0] = 1.0;
    instrument.Octaves[1] = 1.0;
    instrument.Octaves[2] = 1.0;
    instrument.Octaves[3] = 1.0;

    instrument.ADSR[0] = 0.005;
    instrument.ADSR[1] = 0.005;
    instrument.ADSR[2] = 0.5;
    instrument.ADSR[3] = 0.1;

    instrument.FX[0] = 0.0;
    instrument.FX[1] = 0.0;
    instrument.FX[2] = 0.0;
    instrument.FX[3] = 0.0;

    return instrument;
}

MidiInstrument MidiInstrument_Create(
    float osc0, float osc1, float osc2, float osc3,
    float oct0, float oct1, float oct2, float oct3,
    float a, float d, float s, float r,
    float fx0, float fx1, float fx2, float fx3) {

    MidiInstrument instrument;
    instrument.Oscilators[0] = osc0;
    instrument.Oscilators[1] = osc1;
    instrument.Oscilators[2] = osc2;
    instrument.Oscilators[3] = osc3;

    instrument.Octaves[0] = oct0;
    instrument.Octaves[1] = oct1;
    instrument.Octaves[2] = oct2;
    instrument.Octaves[3] = oct3;

    instrument.ADSR[0] = a;
    instrument.ADSR[1] = d;
    instrument.ADSR[2] = s;
    instrument.ADSR[3] = r;

    instrument.FX[0] = fx0;
    instrument.FX[1] = fx1;
    instrument.FX[2] = fx2;
    instrument.FX[3] = fx3;

    return instrument;
}

MidiInstrument MidiInstrument_CreateFromGeneralMidiProgram(int generalMidiProgram) {
    if (generalMidiProgram >= 0 && generalMidiProgram < 8) { // Piano
        return MidiInstrument_Create(
            1.25, 1.125, 0.0, 0.0,
            1.0, 2.0, 1.0, 1.0,
            0.005, 0.005, 0.25, 0.1,
            0.0, 0.0, 0.0, 0.0);
    }
    else if (generalMidiProgram >= 8 && generalMidiProgram < 16) { // Chromatic Percussion
        
    }
    else if (generalMidiProgram >= 16 && generalMidiProgram < 24) { // Organ
        
    }
    else if (generalMidiProgram >= 24 && generalMidiProgram < 32) { // Guitar
        return MidiInstrument_Create(
            0.99, 0.5, 1.5, 0.0,
            2.0, 1.001, 1.0, 1.0,
            0.001, 0.001, 0.1, 0.2,
            0.0, 0.0, 0.0, 0.0);
    }
    else if (generalMidiProgram >= 32 && generalMidiProgram < 40) { // Bass
        return MidiInstrument_Create(
            0.99, 0.5, 0.0, 0.0,
            1.0, 2.0, 1.0, 1.0,
            0.002, 0.001, 0.75, 0.1,
            0.0, 0.0, 0.0, 0.0);
    }
    else if (generalMidiProgram >= 40 && generalMidiProgram < 48) { // Strings
        if (generalMidiProgram == 46) { // Orchestral Harp
            return MidiInstrument_Create(
                1.25, 1.0625, 0.0, 0.0,
                1.0, 2.00001, 1.001, 1.0,
                0.001, 0.01, 0.75, 0.5,
                0.0, 0.0, 0.0, 0.0);
        }

        return MidiInstrument_Create(
            0.125, 1.0625, 0.0, 0.0,
            1.0, 2.00001, 1.001, 1.0,
            0.5, 0.5, 0.75, 0.5,
            0.0, 0.0, 0.0, 0.0);
    }
    else if (generalMidiProgram >= 48 && generalMidiProgram < 56) { // Ensemble
        return MidiInstrument_Create(
            0.125, 1.0625, 0.0, 0.0,
            1.0, 2.00001, 1.001, 1.0,
            0.5, 0.5, 0.75, 0.5,
            0.0, 0.0, 0.0, 0.0);
    }
    else if (generalMidiProgram >= 56 && generalMidiProgram < 64) { // Brass
        
    }
    else if (generalMidiProgram >= 64 && generalMidiProgram < 72) { // Reed
        if (generalMidiProgram == 66) { // Tenor Sax
            return MidiInstrument_Create(
                1.5, 1.25, 2.25, 0.0,
                1.0, 0.5, 0.501, 1.0,
                0.01, 0.01, 0.5, 0.1,
                0.025, 0.0, 0.0, 0.0);
        }

        return MidiInstrument_Create(
            0.5, 1.25, 2.25, 0.0,
            1.0, 1.0, 1.001, 1.0,
            0.001, 0.01, 0.5, 0.05,
            0.02, 0.0, 0.0, 0.0);
    }
    else if (generalMidiProgram >= 72 && generalMidiProgram < 80) { // Pipe
        
    }
    else if (generalMidiProgram >= 80 && generalMidiProgram < 96) { // Pads
        if (generalMidiProgram == 90) { // Pad 3 (polysynth)
            return MidiInstrument_Create(
                2.125, 0.5, 0.0, 0.0,
                1.0, 2.0001, 1.0, 1.0,
                0.005, 0.005, 0.25, 0.1,
                0.0025, 0.0, 0.0, 0.0);
        }
    }

    printf("Unknown instrument %d \"%s\", using generic!\n", generalMidiProgram, Names[generalMidiProgram]);
    return MidiInstrument_CreateDefault();
}
