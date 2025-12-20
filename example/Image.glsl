
// modification of https://www.shadertoy.com/view/ftySWm

//----------------------------------------------------------------------------------------------
const vec3 programColors[8] = vec3[](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0),
    vec3(0.0, 1.0, 1.0),
    vec3(1.0, 0.0, 1.0),
    vec3(1.0, 1.0, 0.0),
    vec3(1.0, 1.0, 1.0),
    vec3(0.5, 0.5, 0.5)
);

//----------------------------------------------------------------------------------------------
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    fragColor = vec4(0, 0, 0, 1);

    float y = (fragCoord.y / iResolution.y);
    float time = mod(iTime, float(songLengthSeconds));
    
    float noteDist = 0.5 + 0.5 * sin(y * 3.14159265358);
    float fragNote = floor((fragCoord.x / iResolution.x) * 127.0 * noteDist + 63.5 * (1.0 - noteDist));
    float fragTime = (y - 0.5) * 4.0 + time;

    ivec2 eventRange = timeEventRanges[int(fragTime) % songLengthSeconds];
    for (int i = eventRange.x; i < eventRange.y; ++i)
    {
        vec4 e = noteEvents[i];
       
        if (fragTime >= e.x && fragTime < e.y)
        {
            if (floor(e.w) == fragNote && fragTime >= e.x && fragTime < e.y - 0.025)
            {
                fragColor.xyz = programColors[int(e.z) % 8] * (1.0 - abs(y - 0.5) / 0.5);
                
                if (time < e.x)
                    fragColor.xyz *= 0.25;
                else if (time >= e.y)
                    fragColor.xyz *= max(1.0 - (time - e.y) * 4.0, 0.25);
            }
        }
    }
    
    if (abs(y - 0.5) < 0.002)
        fragColor.xyz = vec3(0.25, 0.25, 0.25);
}























