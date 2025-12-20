
// modification of https://www.shadertoy.com/view/ftySWm

//----------------------------------------------------------------------------------------------
float GetNoteFrequency(float note)
{
    return 440.0 * pow(2.0, (note - 69.0) / 12.0);
}

//----------------------------------------------------------------------------------------------
float CalculateADSRVolume(in float time, in vec4 adsr, in float noteDuration)
{
    float result = 1.0;
    
    float t = min(time, noteDuration);
    
    if (t < adsr.x)
        result = t / adsr.x;
    else if (t < adsr.x + adsr.y)
        result = adsr.z + (1.0 - adsr.z) * (1.0 - (t - adsr.x) / adsr.y);
    else
        result = adsr.z;
        
    if (time > noteDuration)
    {
        if (adsr.w > 0.0)
            result *= max(1.0 - ((time - noteDuration) / adsr.w), 0.0);
        else
            result = 0.0;
    }
    
    return result;
}

//----------------------------------------------------------------------------------------------
vec2 RenderNoteEvent(in float time, vec4 e)
{
    Instrument instr = instruments[int(e.z)];

    float vibrato = instr.fx.x * sin(e.w + time * 4.0 * 6.28);
    float freq = GetNoteFrequency(floor(e.w) + vibrato);
    float s = 0.0;
    
    for (int i = 0; i < 4; ++i)
    {
        float oscilatorVolume = fract(instr.oscilators[i]);
        if (oscilatorVolume > 0.0)
        {
            int oscilatorType = int(instr.oscilators[i]);
            float oscilatorOctave = instr.octaves[i];
            
            if (oscilatorType == 0)
                s += oscilatorVolume * sin(oscilatorOctave * 6.2831 * time * freq);
            else if (oscilatorType == 1)
                s += oscilatorVolume * (2.0 * fract(oscilatorOctave * time * freq) - 1.0);
            else if (oscilatorType == 2)
                s += oscilatorVolume * sign(sin(oscilatorOctave * 6.2831 * time * freq));
        }
    }
    
    float volume = CalculateADSRVolume(time, instr.adsr, e.y - e.x);
    s *= volume * (1.0 - fract(e.w));

    float pan = fract(e.z);
    return vec2(s * sqrt(pan), s * sqrt(1.0 - pan));
}

//----------------------------------------------------------------------------------------------
vec2 mainSound( int samp, float time )
{
    vec2 stereoSample = vec2(0, 0);

    time = mod(time, float(songLengthSeconds));
    ivec2 eventRange = timeEventRanges[int(time)];

    for (int i = eventRange.x; i < eventRange.y; ++i)
    {
        vec4 e = noteEvents[i];
        if (time < e.x || time >= e.y + 1.0)
            continue;
       
        stereoSample += RenderNoteEvent(time - e.x, e);
    }
    
    return stereoSample*1.;
}
