

typedef struct {
    uint8_t Characteristic;
    uint8_t Attack;
    int Decay;
    int Sustain;
    int Release;
    uint8_t WaveSelect;
    uint8_t Scale;
    uint8_t Level;
} OPL2Operator;

typedef struct {
    OPL2Operator Modulator;
    int Feedback;
    OPL2Operator Carrier;
    int NoteOffset;
} OPL2Voice;

typedef struct {
    int BankIndex;
    char* Name;
    int Flags;
    int FineTune;
    int NoteNumber;
    OPL2Voice Voices[2];
} OPL2Instrument;

OPL2Operator CreateOperator(FILE* file) {
    OPL2Operator result;

    uint8_t characteristic = fgetc(file);
    uint8_t attack = fgetc(file);

    result.Characteristic = characteristic;
    result.Attack = attack >> 4;
    result.Decay = attack & 0x0F;

    uint8_t sustain = fgetc(file);
    result.Sustain = sustain >> 4;
    result.Release = sustain & 0x0F;

    result.WaveSelect = fgetc(file);
    result.Scale = fgetc(file);
    result.Level = fgetc(file);

    return result;
}

OPL2Voice CreateVoice(FILE* file) {
    OPL2Voice result;

    result.Modulator = CreateOperator(file);
    result.Feedback = fgetc(file);
    result.Carrier = CreateOperator(file);
    fgetc(file); // Reserved, unused
    fread(&result.NoteOffset, sizeof(int16_t), 1, file);

    return result;
}

char* OPL2ToGLSLString(OPL2Instrument* instrument) {
    char* result = malloc(10240);
    if (result == NULL) {
        return NULL;
    }

    strcpy(result, "OPL2Instrument(");

    for (int i = 0; i < 2; ++i) {
        OPL2Voice v = instrument->Voices[i];

        char temp[256];
        sprintf(temp, "vec4(%d, %d, %d, %d), ",
                v.Modulator.Characteristic,
                v.Modulator.WaveSelect,
                v.Modulator.Scale,
                v.Modulator.Level);
        strcat(result, temp);

        sprintf(temp, "vec4(%d, %d, %d, %d), ",
                v.Modulator.Attack,
                v.Modulator.Decay,
                v.Modulator.Sustain,
                v.Modulator.Release);
        strcat(result, temp);

        sprintf(temp, "vec4(%d, %d, %d, %d), ",
                v.Carrier.Characteristic,
                v.Carrier.WaveSelect,
                v.Carrier.Scale,
                v.Carrier.Level);
        strcat(result, temp);

        sprintf(temp, "vec4(%d, %d, %d, %d)",
                v.Carrier.Attack,
                v.Carrier.Decay,
                v.Carrier.Sustain,
                v.Carrier.Release);
        strcat(result, temp);

        if (i == 0) {
            strcat(result, ", ");
        }
    }

    strcat(result, ")");
    return result;
}

OPL2Instrument ParseFromBank(int bankIndex, FILE* file) {
    OPL2Instrument result;
    result.BankIndex = bankIndex;
    result.Name = NULL;

    uint16_t flags;
    fread(&flags, sizeof(uint16_t), 1, file);
    result.Flags = flags;

    result.FineTune = fgetc(file);
    result.NoteNumber = fgetc(file);

    for (int i = 0; i < 2; ++i) {
        result.Voices[i] = CreateVoice(file);
    }

    return result;
}

OPL2Instrument* LoadBankFromFile(const char* fileName, int* count) {
    FILE* file = fopen(fileName, "rb");
    if (file == NULL) {
        *count = 0;
        return NULL;
    }

    char header[8];
    fread(header, sizeof(char), 8, file);
    if (memcmp(header, "#OPL_II#", 8) != 0) {
        fclose(file);
        *count = 0;
        return NULL;
    }

    OPL2Instrument* instruments = malloc(175 * sizeof(OPL2Instrument));
    if (instruments == NULL) {
        fclose(file);
        *count = 0;
        return NULL;
    }

    for (int i = 0; i < 175; ++i) {
        instruments[i] = ParseFromBank(i, file);
    }

    for (int i = 0; i < 175; ++i) {
        char nameBytes[32];
        fread(nameBytes, sizeof(char), 32, file);

        int length = 0;
        while (length < 32 && nameBytes[length] >= 32) {
            length++;
        }

        instruments[i].Name = malloc(length + 1);
        if (instruments[i].Name != NULL) {
            strncpy(instruments[i].Name, nameBytes, length);
            instruments[i].Name[length] = '\0';
        }
    }

    fclose(file);
    *count = 175;
    return instruments;
}

void FreeInstruments(OPL2Instrument* instruments, int count) {
    if (instruments == NULL) {
        return;
    }

    for (int i = 0; i < count; ++i) {
        if (instruments[i].Name != NULL) {
            free(instruments[i].Name);
        }
    }

    free(instruments);
}
