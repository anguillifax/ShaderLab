#version 450 core

// ==========
// Attributes
// ==========

layout (location = 0) uniform ivec2 u_WindowSize;
layout (location = 1) uniform float u_Time;
layout (location = 2) uniform ivec2 u_MousePos;

out vec4 FragColor;

// =========
// Constants
// =========

const float PI = 3.14159265359;
const float PI2 = 6.28318530718;

const float SMOOTH_DIST = 1.2;

const float BAND_WIDTH = 30.0;

const float DISTORT_RADIUS = 300.0;
const float DISTORT_FACTOR = 50.0;

// =========
// Variables
// =========

vec3 position;
vec2 uv;

// ================
// Helper Functions
// ================

float smoothed(float min, float max, float value)
{
    return
        smoothstep(min, min + SMOOTH_DIST, value) -
        smoothstep(max - SMOOTH_DIST, max, value);
}

float circle_mask(vec2 center, float radius, float thickness)
{
    float dist = distance(position.xy, center) - radius;
    return smoothed(
        -thickness / 2,
        +thickness / 2,
        dist);
}

// ==========
// Main Logic
// ==========

float get_radius_modulate(float magnitude, float offset)
{
    return magnitude * sin(u_Time * PI2 / 4.0 + offset);
}

void main()
{
    // Setup global variables

    position = gl_FragCoord.xyz;
    uv = position.xy / u_WindowSize;


    // Draw cursor

    float density = 0.0;

    position = gl_FragCoord.xyz;

    density += circle_mask(
        vec2(u_MousePos),
        40.0 + get_radius_modulate(3.0, 0.0),
        10.0);

    density += circle_mask(
        vec2(u_MousePos),
        55.0 + get_radius_modulate(2.5, -0.2),
        7.0);
        
    density += circle_mask(
        vec2(u_MousePos),
        67.0 + get_radius_modulate(2.0, -0.4),
        5.0);


    // Calculate foreground and background colors

    vec3 fg_color = vec3(uv, 1.0);

    vec3 bg_color = mix(
        vec3(0.07),
        vec3(0.09),
        smoothed(0.0, BAND_WIDTH, mod(position.x + position.y, BAND_WIDTH * 2)));


    // Draw output color

    FragColor = vec4(
        mix(bg_color, fg_color, density),
        1.0);
}