# ShadertoyMIDI_in_C

**What is this** - this is port to C from C# of original https://github.com/P-i-N/ShadertoyMIDI

**Use case** - to display/convert timings of notes. Create small segments of music and convert timings or small repetitive segments.

## Limitations:

- it does work
- but...
- "audio quality" is very different(*bad*) to original midi - because used simple sine-wave as audio generator
- and **drums not supported**

## Use example:

- *As midi-web-editor use* - https://github.com/ryohey/signal *or web* https://signalmidi.app/edit or [exact same web midi app](https://www.aisongmaker.io/midi-editor)
- Add notes in web editor or drop midi file - click file-save
- build this ShadertoyMIDI_in_C
- `gcc midi2glsl.c -lm -o midi2glsl`
- convert midi with
- `./midi2glsl test.mid out.glsl`
- open in test editor out.glsl
- replace Common code with generated in https://www.shadertoy.com/view/ftySWm
- *or same shader code in example here*
- if too large - around 1000 vec4 array should work everywhere - cut length of song to smaller to fit
- and compare to converted to shadertoy "sound quality" - it will be bad
- but maybe atleast "timings" can be used for something
- as I said in use case - converting small parts of musics is probably only one use case



