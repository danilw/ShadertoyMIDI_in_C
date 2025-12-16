
typedef enum {
    NoteOff,
    NoteOn,
    KeyAfterTouch,
    ControlChange,
    ProgramChange,
    ChannelAfterTouch,
    PitchWheelChange,
    Tempo,
} MidiEventType;

typedef struct {
    int track;
    double time;
    int ticks;
    int channel;
    MidiEventType type;
    int value0;
    int value1;
} MidiEvent;

typedef struct {
    int index;
    char* name;
    MidiEvent* events;
    int event_count;
    int event_capacity;
} MidiTrack;

typedef struct {
    uint16_t ticksPerQuarterNote;
    int tempo;
    int trackCount;
    int trackCapacity;
    MidiTrack* tracks;
} MidiFile;

static int read_var_length_int(FILE* file) {
    int value_byte = fgetc(file);
    int result = value_byte & 0x7F;

    if ((value_byte & 0x80) != 0) {
        value_byte = fgetc(file);
        result <<= 7;
        result += (value_byte & 0x7F);
        if ((value_byte & 0x80) != 0) {
            value_byte = fgetc(file);
            result <<= 7;
            result += (value_byte & 0x7F);
            if ((value_byte & 0x80) != 0) {
                value_byte = fgetc(file);
                result <<= 7;
                result += (value_byte & 0x7F);
                if ((value_byte & 0x80) != 0) {
                    value_byte = fgetc(file);
                    result <<= 7;
                    result += (value_byte & 0x7F);
                    if ((value_byte & 0x80) != 0) {
                        fprintf(stderr, "Variable length value too big!\n");
                        exit(EXIT_FAILURE);
                    }
                }
            }
        }
    }

    return result;
}

uint32_t convertToLittleEndianUInt32(uint32_t value) {
    return ((value & 0xFF) << 24) | ((value & 0xFF00) << 8) | ((value & 0xFF0000) >> 8) | ((value & 0xFF000000) >> 24);
}
int16_t convertToLittleEndianInt16(int16_t value) {
    return (value >> 8) | (value << 8);
}

uint16_t convertToLittleEndianUInt16(uint16_t value) {
    return (value >> 8) | (value << 8);
}

void initMidiTrack(MidiTrack* track) {
    track->event_count = 0;
    track->event_capacity = 10;
    track->events = (MidiEvent*)malloc(track->event_capacity * sizeof(MidiEvent));
}

void addEventToTrack(MidiTrack* track, MidiEvent event) {
    if (track->event_count >= track->event_capacity) {
        track->event_capacity += 10;
        track->events = (MidiEvent*)realloc(track->events, track->event_capacity * sizeof(MidiEvent));
    }
    track->events[track->event_count++] = event;
}

MidiTrack parse_midi_track(FILE* file, int track_index) {
    MidiTrack result;
    initMidiTrack(&result);
    result.index = track_index;
    result.name = strdup("");

    char header[4];
    if (fread(header, 1, 4, file) != 4 || memcmp(header, "MTrk", 4) != 0) {
        fprintf(stderr, "Invalid MIDI track header!\n");
        exit(EXIT_FAILURE);
    }

    uint32_t track_length;
    if (fread(&track_length, 4, 1, file) != 1) {
        fprintf(stderr, "Failed to read track length\n");
        exit(EXIT_FAILURE);
    }
    track_length = convertToLittleEndianUInt32(track_length);
    long end_position = ftell(file) + track_length;

    int time_ticks = 0;
    int prev_event_type = 0;

    while (ftell(file) < end_position) {
        int delta = read_var_length_int(file);
        time_ticks += delta;

        MidiEvent e = {0};
        e.track = track_index;
        e.ticks = time_ticks;

        int event_type = fgetc(file);

        if (event_type < 128) {
            fseek(file, -1, SEEK_CUR);
            event_type = prev_event_type;
        }

        prev_event_type = event_type;

        if ((event_type & 0xF0) == 0x80) { // Note off
            e.channel = event_type & 0x0F;
            e.type = NoteOff;
            e.value0 = fgetc(file);
            e.value1 = fgetc(file);
            addEventToTrack(&result, e);
        }
        else if ((event_type & 0xF0) == 0x90) { // Note on
            e.channel = event_type & 0x0F;
            e.type = NoteOn;
            e.value0 = fgetc(file);
            e.value1 = fgetc(file);

            if (e.value1 == 0)
                e.type = NoteOff;

            addEventToTrack(&result, e);
        }
        else if ((event_type & 0xF0) == 0xB0) { // Control change
            e.channel = event_type & 0x0F;
            e.type = ControlChange;
            e.value0 = fgetc(file);
            e.value1 = fgetc(file);
            addEventToTrack(&result, e);
        }
        else if ((event_type & 0xF0) == 0xC0) { // Program change
            e.channel = event_type & 0x0F;
            e.type = ProgramChange;
            e.value0 = fgetc(file);
            addEventToTrack(&result, e);
        }
        else if ((event_type & 0xF0) == 0xE0) { // Pitch wheel change
            e.channel = event_type & 0x0F;
            e.type = PitchWheelChange;
            e.value0 = fgetc(file);
            e.value1 = fgetc(file);
            addEventToTrack(&result, e);
        }
        else if (event_type == 0xF0) { // System exclusive
            int sys_excl_length = read_var_length_int(file);
            fseek(file, sys_excl_length, SEEK_CUR);
        }
        else if (event_type == 0xFF) { // Meta event
            int meta_event_type = fgetc(file);
            int meta_event_length = read_var_length_int(file);
            uint8_t* meta_bytes = malloc(meta_event_length);
            if (meta_bytes == NULL) {
                fprintf(stderr, "Memory allocation failed\n");
                exit(EXIT_FAILURE);
            }
            if (fread(meta_bytes, 1, meta_event_length, file) != meta_event_length) {
                fprintf(stderr, "Failed to read meta event data\n");
                free(meta_bytes);
                exit(EXIT_FAILURE);
            }

            if (meta_event_type == 3) { // Track name
                free(result.name);
                result.name = malloc(meta_event_length + 1);
                if (result.name == NULL) {
                    fprintf(stderr, "Memory allocation failed\n");
                    free(meta_bytes);
                    exit(EXIT_FAILURE);
                }
                memcpy(result.name, meta_bytes, meta_event_length);
                result.name[meta_event_length] = '\0';
            }
            else if (meta_event_type == 81) { // Tempo
                e.channel = 0;
                e.type = Tempo;
                e.value0 = meta_bytes[2] + meta_bytes[1] * 256 + meta_bytes[0] * 65536;
                addEventToTrack(&result, e);
            }

            free(meta_bytes);
        }
        else {
            fprintf(stderr, "Unknown MIDI event type!\n");
            exit(EXIT_FAILURE);
        }
    }

    return result;
}

void initMidiFile(MidiFile* file) {
    file->ticksPerQuarterNote = 0;
    file->tempo = 0;
    file->trackCount = 0;
    file->trackCapacity = 10;
    file->tracks = (MidiTrack*)malloc(file->trackCapacity * sizeof(MidiTrack));
}

void addTrackToFile(MidiFile* file, MidiTrack track) {
    if (file->trackCount >= file->trackCapacity) {
        file->trackCapacity += 10;
        file->tracks = (MidiTrack*)realloc(file->tracks, file->trackCapacity * sizeof(MidiTrack));
    }
    file->tracks[file->trackCount++] = track;
}

void quantizeTicks(MidiFile* file, int quarterNoteSubdiv) {
    for (int i = 0; i < file->trackCount; ++i) {
        MidiTrack* track = &file->tracks[i];
        for (int j = 0; j < track->event_count; ++j) {
            MidiEvent* e = &track->events[j];

            int ticksAlign = e->ticks % quarterNoteSubdiv;
            if (ticksAlign < quarterNoteSubdiv / 2)
                e->ticks -= ticksAlign;
            else
                e->ticks += quarterNoteSubdiv - ticksAlign;

            e->ticks /= quarterNoteSubdiv;
        }
    }

    file->tempo *= quarterNoteSubdiv;
    file->ticksPerQuarterNote = file->ticksPerQuarterNote / quarterNoteSubdiv;
}

void resolveEventsRangeTempo(MidiFile* file, int ticksFrom, int ticksTo, int tempo) {
    // tempo = microseconds per quarter note
    double secsPerQuarterNote = tempo / 1000000.0;
    for (int i = 0; i < file->trackCount; ++i) {
        MidiTrack* track = &file->tracks[i];
        for (int j = 0; j < track->event_count; ++j) {
            MidiEvent* e = &track->events[j];
            if (e->ticks < ticksFrom || e->ticks >= ticksTo)
                continue;

            e->time = ((double)e->ticks / file->ticksPerQuarterNote) * secsPerQuarterNote;
        }
    }
}

void resolveEventsTiming(MidiFile* file) {
    for (int i = 0; i < file->trackCount; ++i) {
        MidiTrack* track = &file->tracks[i];
        int tempoTicks = -1;

        for (int j = 0; j < track->event_count; ++j) {
            MidiEvent* e = &track->events[j];
            if (e->type == Tempo) {
                if (file->tempo != 0.0)
                    printf("Song has multiple tempos specified: previous = %d, new = %d\n", file->tempo, e->value0);
                else {
                    file->tempo = e->value0;
                    tempoTicks = e->ticks;
                }
            }
        }
        
        if (file->tempo > 0 && tempoTicks >= 0)
            resolveEventsRangeTempo(file, tempoTicks, INT_MAX, file->tempo);
    }
}

MidiFile parseMidiFile(FILE* rf) {
    MidiFile file;
    initMidiFile(&file);
    
    char header[4];
    if (fread(header, 1, 4, rf) != 4){ 
        fprintf(stderr, "error reading MIDI header!\n");
        exit(EXIT_FAILURE);
    }
    if(memcmp(header, "MThd", 4) != 0) {
        fprintf(stderr, "Invalid MIDI header!\n");
        exit(EXIT_FAILURE);
    }
    
    uint32_t header_length;
    if (fread(&header_length, 4, 1, rf) != 1) {
        fprintf(stderr, "error reading MIDI header length!\n");
        exit(EXIT_FAILURE);
    }
    header_length = convertToLittleEndianUInt32(header_length);
    
    if (header_length != 6) {
        fprintf(stderr, "Invalid MIDI header length!\n");
        exit(EXIT_FAILURE);
    }
    
    uint16_t trackFileFormat;
    if (fread(&trackFileFormat, 2, 1, rf) != 1) {
        fprintf(stderr, "error reading trackFileFormat!\n");
        exit(EXIT_FAILURE);
    }
    trackFileFormat = convertToLittleEndianUInt16(trackFileFormat);
    
    uint16_t numTracks;
    if (fread(&numTracks, 2, 1, rf) != 1) {
        fprintf(stderr, "error reading numTracks!\n");
        exit(EXIT_FAILURE);
    }
    numTracks = convertToLittleEndianUInt16(numTracks);
    
    if (fread(&file.ticksPerQuarterNote, 2, 1, rf) != 1) {
        fprintf(stderr, "error reading ticksPerQuarterNote!\n");
        exit(EXIT_FAILURE);
    }
/*
    C# original 
    result.TicksPerQuarterNote = BitUtils.ConvertToLittleEndian(br.ReadInt16());
    why it is ReadInt16 - if I change file.ticksPerQuarterNote to int16_t and convertToLittleEndianInt16 - negative value broken result - idk
*/
    file.ticksPerQuarterNote = convertToLittleEndianUInt16(file.ticksPerQuarterNote);
    
    for (int i = 0; i < numTracks; ++i) {
        MidiTrack track = parse_midi_track(rf, i);
        if (track.event_count > 0)
            addTrackToFile(&file, track);
    }
    
    quantizeTicks(&file, file.ticksPerQuarterNote / 8);
    resolveEventsTiming(&file);
    return file;
}

void freeMidiTrack(MidiTrack* track) {
    if (track->name != NULL) {
        free(track->name);
    }
    if (track->events != NULL) {
        free(track->events);
    }
}

void freeMidiFile(MidiFile* file) {
    for (int i = 0; i < file->trackCount; ++i) {
        freeMidiTrack(&file->tracks[i]);
    }
    free(file->tracks);
}








