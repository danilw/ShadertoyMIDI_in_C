

/*
 * 
 * self https://github.com/danilw/ShadertoyMIDI_in_C
 * License - ShadertoyMIDI_in_C is licensed under The Unlicense
 * 
 * This is port of original https://github.com/P-i-N/ShadertoyMIDI/tree/main/ShadertoyMIDI
 * port from C# to C
 * 
 * NOT IMPLEMENTED:
 * FilterTimeRange ApplyTimeOffset - search below here
 * 
 * 
*/


/*

gcc midi2glsl.c -lm -o midi2glsl
./midi2glsl test.mid out.glsl

*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>
#include <string.h>
#include <math.h>

#include "OPL2Instrument.h"
#include "MidiFile.h"
#include "MidiInstrument.h"

void print_usage(char *name)
{
    printf("Usage: %s \n"
           "\t midi2glsl file.midi out_file.glsl \n",
           name);
}

typedef struct {
    double TimeBegin;
    double TimeEnd;
    int TicksBegin;
    int TicksEnd;
    int Program;
    int Channel;
    int Note;
    int Velocity;
    int Panning;
} MergedMidiEvent;

typedef struct {
    int Program;
    int Panning;
} MidiChannelState;

typedef struct {
    int minIndex;
    int maxIndex;
} ListTuple_int_int;

bool IsInTimeRange(MergedMidiEvent* event, double fromTime, double toTime, double releaseTime) {
    return fromTime <= (event->TimeEnd + releaseTime) && toTime >= event->TimeBegin;
}

char* ToGLSLVec4String(const MergedMidiEvent* event) {
    char* buffer = malloc(128);
    snprintf(buffer, 128, "vec4(%.3f, %.3f, %.2f, %.2f)",
        event->TimeBegin,
        event->TimeEnd,
        (double)event->Program + (double)event->Panning / 127.0,
        (double)event->Note + ((double)(127 - event->Velocity) / 127.0));
    return buffer;
}

char* ToGLSLiVec2String(const ListTuple_int_int* event) {
    char* buffer = malloc(32);
    snprintf(buffer, 32, "ivec2(%d, %d)", event->minIndex, event->maxIndex);
    return buffer;
}

#define BITS_TICKS_BEGIN 11
#define BITS_DURATION 7
#define BITS_INSTRUMENT_INDEX 3
#define BITS_NOTE 7
#define BITS_VELOCITY 3

char* ToGLSLUIntString(const MergedMidiEvent* event) {
    char* buffer = malloc(32);

    uint32_t timeInfo = (uint32_t)(event->TicksBegin & ((1 << BITS_TICKS_BEGIN) - 1));
    timeInfo |= (uint32_t)((event->TicksEnd - event->TicksBegin) & ((1 << BITS_DURATION) - 1)) << BITS_TICKS_BEGIN;

    uint32_t noteInfo = (uint32_t)(event->Note & ((1 << BITS_NOTE) - 1));
    noteInfo |= (uint32_t)(event->Program & ((1 << BITS_INSTRUMENT_INDEX) - 1)) << BITS_NOTE;
    noteInfo |= (uint32_t)((event->Velocity >> (7 - BITS_VELOCITY)) & ((1 << BITS_VELOCITY) - 1)) << (BITS_NOTE + BITS_INSTRUMENT_INDEX);

    snprintf(buffer, 32, "0x%08Xu", timeInfo | (noteInfo << (BITS_TICKS_BEGIN + BITS_DURATION)));
    return buffer;
}

void WriteGLSLArray(FILE* file, const char* arrayType, const char* arrayName, int arraySize,
                   char* (*itemCallback)(const MergedMidiEvent*), const MergedMidiEvent* events, int itemsPerLine) {
    fprintf(file, "const %s %s[%d] = %s[](\n    ", arrayType, arrayName, arraySize, arrayType);

    for (int i = 0; i < arraySize; ++i) {
        char* item = itemCallback(&events[i]);
        fprintf(file, "%s%s", item, (i < arraySize - 1) ? ", " : "");
        free(item);

        if (((i + 1) % itemsPerLine) == 0) {
            if (i < arraySize - 1) {
                fprintf(file, "\n    ");
            } else {
                fprintf(file, " ");
            }
        }
    }

    fprintf(file, "\n);\n");
}

void WriteGLSLArray_int_int(FILE* file, const char* arrayType, const char* arrayName, int arraySize,
                   char* (*itemCallback)(const ListTuple_int_int* ), const ListTuple_int_int* events, int itemsPerLine) {
    fprintf(file, "const %s %s[%d] = %s[](\n    ", arrayType, arrayName, arraySize, arrayType);

    for (int i = 0; i < arraySize; ++i) {
        char* item = itemCallback(&events[i]);
        fprintf(file, "%s%s", item, (i < arraySize - 1) ? ", " : "");
        free(item);

        if (((i + 1) % itemsPerLine) == 0) {
            if (i < arraySize - 1) {
                fprintf(file, "\n    ");
            } else {
                fprintf(file, " ");
            }
        }
    }

    fprintf(file, "\n);\n");
}

void WriteGLSLUIntArray(FILE* file, const char* arrayName, const MergedMidiEvent* mergedMidiEvents, int count, int eventsPerLine) {
    WriteGLSLArray(file, "uint", arrayName, count, ToGLSLUIntString, mergedMidiEvents, eventsPerLine);
}

void WriteGLSLvec4Array(FILE* file, const char* arrayName, const MergedMidiEvent* mergedMidiEvents, int count, int eventsPerLine) {
    WriteGLSLArray(file, "vec4", arrayName, count, ToGLSLVec4String, mergedMidiEvents, eventsPerLine);
}

void WriteGLSLivec2Array(FILE* file, const char* arrayName, const ListTuple_int_int* timeEventRanges, int count, int eventsPerLine) {
    WriteGLSLArray_int_int(file, "ivec2", arrayName, count, ToGLSLiVec2String, timeEventRanges, eventsPerLine);
}

#define MAX_EVENTS_PER_TRACK 10000
void MergeMidiEvents(MidiFile* mf, MergedMidiEvent** mergedEvents, int* mergedEvents_size, int* mergedEventCount) {
    *mergedEventCount = 0;
    
    for (int trackIdx = 0; trackIdx < mf->trackCount; trackIdx++) {
        MidiTrack* track = &mf->tracks[trackIdx];
        const int max_channelStates = 16;
        MidiChannelState channelStates[max_channelStates];
        MidiEvent* noteOnEvents;
        noteOnEvents = (MidiEvent*)malloc(MAX_EVENTS_PER_TRACK * sizeof(MidiEvent));
        for (int i = 0; i < MAX_EVENTS_PER_TRACK; i++) {
            noteOnEvents[i].track=0;
            noteOnEvents[i].time=0;
            noteOnEvents[i].ticks=0;
            noteOnEvents[i].channel=0;
            noteOnEvents[i].type=0;
            noteOnEvents[i].value0=0;
            noteOnEvents[i].value1=0;
        }
        int noteOnEventCount = 0;

        for (int i = 0; i < max_channelStates; i++) {
            channelStates[i].Program = 0;
            channelStates[i].Panning = 64;
        }

        for (int eventIdx = 0; eventIdx < track->event_count; eventIdx++) {
            MidiEvent* e = &track->events[eventIdx];
            
            if(noteOnEventCount >= MAX_EVENTS_PER_TRACK) {
                printf("error noteOnEventCount %d larger than MAX_EVENTS_PER_TRACK %d\n", noteOnEventCount, MAX_EVENTS_PER_TRACK);
                printf("exit\n");
                exit(EXIT_FAILURE);
            }
            
            if (e->type == NoteOn) {
                if (noteOnEventCount < MAX_EVENTS_PER_TRACK) {
                    noteOnEvents[noteOnEventCount++] = *e;
                }
            } else if (e->type == NoteOff) {
                int index = -1;
                for (int i = 0; i < noteOnEventCount; i++) {
                    if (noteOnEvents[i].value0 == e->value0 && noteOnEvents[i].channel == e->channel) {
                        index = i;
                        break;
                    }
                }

                if (index >= 0) {
                    // Skip notes for channel 10 (drums)
                    if (e->channel == 9) {
                        for (int i = index; i < noteOnEventCount - 1; i++) {
                            noteOnEvents[i] = noteOnEvents[i + 1];
                        }
                        noteOnEventCount--;
                        continue;
                    }
                    if (*mergedEventCount >= *mergedEvents_size){
                        (*mergedEvents_size)+=1;
                        if (*mergedEventCount >= *mergedEvents_size){printf("error in mergedEventCount\n");exit(EXIT_FAILURE);}
                        *mergedEvents = (MergedMidiEvent*)realloc(*mergedEvents, (*mergedEvents_size) * sizeof(MergedMidiEvent));
                    }
                    MergedMidiEvent* mme = &(*mergedEvents)[*mergedEventCount];
                    mme->TimeBegin = noteOnEvents[index].time;
                    mme->TimeEnd = e->time;
                    mme->TicksBegin = noteOnEvents[index].ticks;
                    mme->TicksEnd = e->ticks;
                    mme->Program = channelStates[e->channel].Program;
                    mme->Channel = e->channel;
                    mme->Note = e->value0;
                    mme->Velocity = noteOnEvents[index].value1;
                    mme->Panning = channelStates[e->channel].Panning;
                    
                    if (mme->TicksBegin == mme->TicksEnd) {
                        mme->TicksEnd += 1;
                    }

                    for (int i = index; i < noteOnEventCount - 1; i++) {
                        noteOnEvents[i] = noteOnEvents[i + 1];
                    }
                    noteOnEventCount++;

                    (*mergedEventCount)++;
                }
            } else if (e->type == ProgramChange) {
                channelStates[e->channel].Program = e->value0;
            } else if (e->type == ControlChange) {
                if (e->value0 == 10) { // Panning
                    channelStates[e->channel].Panning = e->value1;
                }
            }
        }
        free(noteOnEvents);
    }

    for (int i = 0; i < *mergedEventCount - 1; i++) {
        for (int j = 0; j < *mergedEventCount - i - 1; j++) {
            if ((*mergedEvents)[j].TimeBegin > (*mergedEvents)[j + 1].TimeBegin) {
                MergedMidiEvent temp = (*mergedEvents)[j];
                (*mergedEvents)[j] = (*mergedEvents)[j + 1];
                (*mergedEvents)[j + 1] = temp;
            }
        }
    }
}

int compare(const void *a, const void *b) {
    int diffA = ((MergedMidiEvent*)a)->TicksEnd - ((MergedMidiEvent*)a)->TicksBegin;
    int diffB = ((MergedMidiEvent*)b)->TicksEnd - ((MergedMidiEvent*)b)->TicksBegin;
    return diffA - diffB;
}

MergedMidiEvent* MaxBy(MergedMidiEvent* arr, int size) {
    if (size == 0) return NULL;

    MergedMidiEvent *maxElem = &arr[0];
    for (int i = 1; i < size; i++) {
        if (compare(&arr[i], maxElem) > 0) {
            maxElem = &arr[i];
        }
    }
    return maxElem;
}

int min(int a, int b) {return (a < b) ? a : b;}
int max(int a, int b) {return (a > b) ? a : b;}

#define MaximumNoteReleaseTime 1.0
void BuildTimeEventRanges(MergedMidiEvent *mergedMidiEvents, int mergedEventCount , ListTuple_int_int** timeEventRanges, int* timeEventRanges_size)
{
    const double rangeLength = 1.0;
    double minTime = mergedMidiEvents[0].TimeBegin;
    double maxTime = minTime;
    for (int i=0; i<mergedEventCount;i++)
    {
        MergedMidiEvent* mme = &mergedMidiEvents[i];
        if (mme->TimeEnd + MaximumNoteReleaseTime > maxTime)
            maxTime = mme->TimeEnd + MaximumNoteReleaseTime;
    }
    int numRanges = (int)ceil(maxTime / rangeLength);
    
    for (int i = 0; i < numRanges; ++i)
    {
        double fromTime = i * rangeLength;
        double toTime = (i + 1) * rangeLength;

        int j = 0;
        int minIndex = INT_MAX;
        int maxIndex = INT_MIN;

        for (int i=0; i<mergedEventCount;i++)
        {
            MergedMidiEvent* mme = &mergedMidiEvents[i];
            if (IsInTimeRange(mme, fromTime, toTime, MaximumNoteReleaseTime))
            {
                minIndex = min(j, minIndex);
                maxIndex = max(j + 1, maxIndex);
            }

            ++j;
        }

        if (minIndex < maxIndex)
        {
            (*timeEventRanges)[(*timeEventRanges_size)-1].minIndex=minIndex;
            (*timeEventRanges)[(*timeEventRanges_size)-1].maxIndex=maxIndex;
        }
        else
        {
            (*timeEventRanges)[(*timeEventRanges_size)-1].minIndex=0;
            (*timeEventRanges)[(*timeEventRanges_size)-1].maxIndex=0;
        }
        (*timeEventRanges_size)+=1;
        *timeEventRanges = (ListTuple_int_int*)realloc(*timeEventRanges, (*timeEventRanges_size) * sizeof(ListTuple_int_int));
    }
}

typedef struct {
    int key;
    int value;
} DictionaryEntry;

typedef struct {
    DictionaryEntry* entries;
    int count;
    int capacity;
} Dictionary;

void Dictionary_Init(Dictionary* dict, int initialCapacity) {
    dict->entries = (DictionaryEntry*)malloc(initialCapacity * sizeof(DictionaryEntry));
    dict->count = 0;
    dict->capacity = initialCapacity;
}

void Dictionary_Add(Dictionary* dict, int key, int value) {
    if (dict->count >= dict->capacity) {
        dict->capacity += 1;
        dict->entries = (DictionaryEntry*)realloc(dict->entries, dict->capacity * sizeof(DictionaryEntry));
    }

    dict->entries[dict->count].key = key;
    dict->entries[dict->count].value = value;
    dict->count++;
}

bool Dictionary_ContainsKey(Dictionary* dict, int key) {
    for (int i = 0; i < dict->count; i++) {
        if (dict->entries[i].key == key) {
            return true;
        }
    }
    return false;
}

int Dictionary_Get(Dictionary* dict, int key) {
    for (int i = 0; i < dict->count; i++) {
        if (dict->entries[i].key == key) {
            return dict->entries[i].value;
        }
    }
    return -1;
}

void Dictionary_Free(Dictionary* dict) {
    free(dict->entries);
    dict->entries = NULL;
    dict->count = 0;
    dict->capacity = 0;
}

void RemapMidiPrograms(MergedMidiEvent *mergedMidiEvents, int mergedEventCount , Dictionary* programMappings)
{
    for (int i = 0; i < mergedEventCount; i++) {
        MergedMidiEvent* mme = &mergedMidiEvents[i];

        if (!Dictionary_ContainsKey(programMappings, mme->Program)) {
            Dictionary_Add(programMappings, mme->Program, programMappings->count);
        }

        mme->Program = Dictionary_Get(programMappings, mme->Program);
    }
}

bool ConvertMIDI(OPL2Instrument* OPL2InstrumentsBank, const int OPL2_count, const char* file_in, const char* file_out)
{
    FILE* f_in = fopen(file_in, "rb");
    if (f_in == NULL) {
        printf("file in error %s\n", file_in);
        return false;
    }
    FILE *f_out = fopen(file_out, "w");
    if (f_out == NULL) {
        fclose(f_in);
        printf("file write error %s\n", file_out);
        return false;
    }
    
    printf("Converting %s into %s:\n", file_in, file_out);
    
    MidiFile mf = parseMidiFile(f_in);
    
    MergedMidiEvent *mergedEvents;
    int mergedEvents_size = 1;
    mergedEvents = (MergedMidiEvent*)malloc(mergedEvents_size * sizeof(MergedMidiEvent));
    int mergedEventCount = 0;
    
    MergeMidiEvents(&mf, &mergedEvents, &mergedEvents_size, &mergedEventCount);
/*
    for (int i = 0; i < mergedEventCount; i++) {
        printf("Event %d: TimeBegin=%.2f, TimeEnd=%.2f, Note=%d\n",
               i, mergedEvents[i].TimeBegin, mergedEvents[i].TimeEnd, mergedEvents[i].Note);
    }
*/
    if(mergedEventCount<1)
    {
        printf("error mergedEventCount<1\n");
        exit(EXIT_FAILURE);
    }
    
    printf("Last Event %d: TimeBegin=%.2f, TimeEnd=%.2f, Note=%d, Tick: %d\n", 
            mergedEventCount-1, mergedEvents[mergedEventCount-1].TimeBegin, mergedEvents[mergedEventCount-1].TimeEnd, 
            mergedEvents[mergedEventCount-1].Note, mergedEvents[mergedEventCount-1].TicksEnd);
    
    MergedMidiEvent *maxTicksDuration = MaxBy(mergedEvents, mergedEventCount);
    printf("Max ticks duration: %d\n", maxTicksDuration->TicksEnd - maxTicksDuration->TicksBegin);
    
    int firstEventTime = mergedEvents[0].TimeBegin;
    int firstEventTicks = mergedEvents[0].TicksBegin;
    
/*
    not implemented 
    FilterTimeRange ApplyTimeOffset
    this only "cut midi length to 120sec" - just cut outside in some midi editor
    
    orginal C# code:
    mergedEvents = FilterTimeRange(mergedEvents, firstEventTime, firstEventTime + 120.0).ToList();
    ApplyTimeOffset(mergedEvents, -firstEventTime, -firstEventTicks);
*/
    
    ListTuple_int_int* timeEventRanges;
    int timeEventRanges_size = 1;
    timeEventRanges = (ListTuple_int_int*)malloc(timeEventRanges_size * sizeof(ListTuple_int_int));
    BuildTimeEventRanges(mergedEvents, mergedEventCount, &timeEventRanges, &timeEventRanges_size);
    Dictionary programMappings;
    Dictionary_Init(&programMappings, 1);
    RemapMidiPrograms(mergedEvents, mergedEventCount, &programMappings);

    int* programMappingList;
    programMappingList = (int*)malloc(programMappings.count * sizeof(int));
    for (int i = 0; i < programMappings.count; ++i)
        programMappingList[i]=0;
    for (int i = 0; i < programMappings.count; ++i)
        programMappingList[programMappings.entries[i].value] = programMappings.entries[i].key;
    
    // idk what it is - not used in shader code
/*
    fprintf(f_out, "struct OPL2Operator { vec4 params, adsr; };\n");
    fprintf(f_out, "struct OPL2Voice { OPL2Operator mod, car; int feedback, noteOffset; };\n");

    fprintf(f_out, "struct OPL2Instrument\n{\n");
    fprintf(f_out, "    vec4 mod0Params, mod0ADSR, car0Params, car0ADSR;\n");
    fprintf(f_out, "    vec4 mod1Params, mod1ADSR, car1Params, car1ADSR;\n");
    fprintf(f_out, "};\n\n");
    
    fprintf(f_out, "const OPL2Instrument opl2instruments[%d] = OPL2Instrument[](", programMappings.count);
    for (int i = 0; i < programMappings.count; ++i)
    {
        int gmIndex = programMappingList[i];

        OPL2Instrument* instr = (gmIndex < OPL2_count)
            ? &OPL2InstrumentsBank[gmIndex]
            : &OPL2InstrumentsBank[0];

        if (i > 0)
            fprintf(f_out, "\n");

        fprintf(f_out, "    // %d: \"%s\" (program %d)\n", i, instr->Name, programMappingList[i]);
        char* tstr = OPL2ToGLSLString(instr);
        fprintf(f_out, "    %s", tstr);
        free(tstr);

        if (i < programMappings.count - 1)
            fprintf(f_out, ",\n");
        else
            fprintf(f_out, "\n");
    }
    fprintf(f_out, ");\n");
*/

    fprintf(f_out, "struct Instrument { vec4 oscilators, octaves, adsr, fx; };\n\n");
    fprintf(f_out, "const Instrument instruments[%d] = Instrument[](\n", programMappings.count);

    for (int i = 0; i < programMappings.count; ++i)
    {
        if (i > 0)
            fprintf(f_out, "\n");

        fprintf(f_out, "    // %d: \"%s\" (program %d)\n",
        i, Names[programMappingList[i]], programMappingList[i]);

        MidiInstrument instr = MidiInstrument_CreateFromGeneralMidiProgram(programMappingList[i]);

        // Determine average panning
        {
            int avgPanning = 0;
            int count = 0;

            for (int j = 0; j < mergedEvents_size; j++)
            {
                if (mergedEvents[i].Program == i)
                {
                    avgPanning += mergedEvents[i].Panning;
                    ++count;
                }
            }

            if (count > 0)
                avgPanning /= count;
            else
                avgPanning = 64;

            instr.FX[1] = ((float)avgPanning) / 127.0;
        }

        fprintf(f_out, "    Instrument( vec4(%.3f, %.3f, %.3f, %.3f), vec4(%.3f, %.3f, %.3f, %.3f), vec4(%.3f, %.3f, %.3f, %.3f), vec4(%.3f, %.3f, %.3f, %.3f) )",
            instr.Oscilators[0],
            instr.Oscilators[1],
            instr.Oscilators[2],
            instr.Oscilators[3],
            instr.Octaves[0],
            instr.Octaves[1],
            instr.Octaves[2],
            instr.Octaves[3],
            instr.ADSR[0],
            instr.ADSR[1],
            instr.ADSR[2],
            instr.ADSR[3],
            instr.FX[0],
            instr.FX[1],
            instr.FX[2],
            instr.FX[3]);

        if (i < programMappings.count - 1)
            fprintf(f_out, ",\n");
        else
            fprintf(f_out, "\n");
    }
    fprintf(f_out, ");\n\n");

    
    // it does not include Panning
    //WriteGLSLUIntArray(f_out, "noteEvents", mergedEvents, mergedEventCount, 15);
    
    // this works 
    WriteGLSLvec4Array(f_out, "noteEvents", mergedEvents, mergedEventCount, 15);
    fprintf(f_out, "\n");
    
    fprintf(f_out, "// First usable noteEvent index for every second\n");
    WriteGLSLivec2Array(f_out, "timeEventRanges", timeEventRanges, timeEventRanges_size-1, 5);
    fprintf(f_out, "\n");
    
    // when WriteGLSLUIntArray used
/*
    fprintf(f_out, "const float secsPerTick = %.5f;\n",
        (((double)mf.tempo / 1000000.0) / (double)mf.ticksPerQuarterNote));

    fprintf(f_out, "const float ticksPerSec = %.5f;\n\n",
        ((double)mf.ticksPerQuarterNote / ((double)mf.tempo / 1000000.0)));
    
    fprintf(f_out, "void FillNoteEvent(in int i, out uvec4 e)\n{\n");
    fprintf(f_out, "    uint n = noteEvents[i];\n\n");
    fprintf(f_out, "    // %d bits for note begin in ticks\n", BITS_TICKS_BEGIN);
    fprintf(f_out, "    e.x = n & %du; n = n >> %d;\n\n", (1 << BITS_TICKS_BEGIN) - 1, BITS_TICKS_BEGIN);
    fprintf(f_out, "    // %d bits for note duration in ticks\n", BITS_DURATION);
    fprintf(f_out, "    e.y = e.x + (n & %du); n = n >> %d;\n\n", (1 << BITS_DURATION) - 1, BITS_DURATION);
    fprintf(f_out, "    // %d bits for note\n", BITS_NOTE);
    fprintf(f_out, "    e.z = n & %du; n = n >> %d;\n\n", (1 << BITS_NOTE) - 1, BITS_NOTE);
    fprintf(f_out, "    // %d bits for instrument index with %d bits for velocity\n", BITS_INSTRUMENT_INDEX, BITS_VELOCITY);
    fprintf(f_out, "    e.w = (n & %du) | ((n & ~%du) << %d);\n", (1 << BITS_INSTRUMENT_INDEX) - 1, (1 << BITS_INSTRUMENT_INDEX) - 1, 4 - BITS_INSTRUMENT_INDEX);
    fprintf(f_out, "}\n");
*/

    freeMidiFile(&mf);
    free(mergedEvents);
    free(timeEventRanges);
    free(programMappingList);
    Dictionary_Free(&programMappings);
    
    fclose(f_in);
    fclose(f_out);
    
    return true;
}

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        printf("No input files specified!\n");
        print_usage(argv[0]);
        return 0;
    }
    if (argc > 1)
    {
        if (strcmp(argv[1], "--help") == 0)
        {
            print_usage(argv[0]);
            return 0;
        }
    }
    
    char *file_in = argv[1];
    char *file_out = argv[2];
    
    char *file_op2 = "GENMIDI.op2";
    int OPL2_count = 0;
    OPL2Instrument* OPL2InstrumentsBank = LoadBankFromFile(file_op2, &OPL2_count);
    
    if(OPL2InstrumentsBank==NULL)
    {
        printf("Could not load GENMIDI.op2!\n");
        return 0;
    }
    
    if(!ConvertMIDI(OPL2InstrumentsBank, OPL2_count, file_in, file_out))
    {
        printf("error in ConvertMIDI\n");
        return 0;
    }
    
    printf("ConvertMIDI done - file name %s\n", file_out);
    
    FreeInstruments(OPL2InstrumentsBank,OPL2_count);
    
}








