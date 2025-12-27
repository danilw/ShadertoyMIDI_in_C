struct Instrument { vec4 oscilators, octaves, adsr, fx; };

const Instrument instruments[2] = Instrument[](
    // 0: "Acoustic Grand Piano" (program 0)
    Instrument( vec4(1.250, 1.125, 0.000, 0.000), vec4(1.000, 2.000, 1.000, 1.000), vec4(0.005, 0.005, 0.250, 0.100), vec4(0.000, 0.504, 0.000, 0.000) ),

    // 1: "Electric Guitar (clean)" (program 27)
    Instrument( vec4(0.990, 0.500, 1.500, 0.000), vec4(2.000, 1.001, 1.000, 1.000), vec4(0.001, 0.001, 0.100, 0.200), vec4(0.000, 0.504, 0.000, 0.000) )
);

// Tuples of: [time begin, time end, program + panning, note + invVelocity]...
const vec4 noteEvents[18] = vec4[](
    vec4(0.000, 0.250, 0.50, 38.21), vec4(0.250, 0.500, 0.50, 39.21), vec4(0.500, 0.750, 0.50, 40.21), vec4(0.750, 1.000, 0.50, 41.21), vec4(1.000, 1.250, 0.50, 42.21), 
    vec4(1.250, 1.500, 0.50, 43.21), vec4(1.500, 1.750, 0.50, 44.21), vec4(1.750, 2.000, 0.50, 45.21), vec4(2.000, 2.250, 0.50, 46.21), vec4(2.250, 2.500, 1.50, 46.21), 
    vec4(2.500, 2.750, 1.50, 45.21), vec4(2.750, 3.000, 1.50, 44.21), vec4(3.000, 3.250, 1.50, 43.21), vec4(3.250, 3.500, 1.50, 42.21), vec4(3.500, 3.750, 1.50, 41.21), 
    vec4(3.750, 4.000, 1.50, 40.21), vec4(4.000, 4.250, 1.50, 39.21), vec4(4.250, 4.500, 1.50, 38.21)
);

// First usable noteEvent index for every second
const ivec2 timeEventRanges[6] = ivec2[](
    ivec2(0, 5), ivec2(0, 9), ivec2(3, 13), ivec2(7, 17), ivec2(11, 18), 
    ivec2(15, 18)
);

const int songLengthSeconds = 4;
