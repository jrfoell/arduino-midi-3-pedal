// colors.h
// Named NeoPixel color constants used throughout the sketch.
// Values match Adafruit_NeoPixel::Color(r, g, b) packing (0x00RRGGBB).
// The library handles GRB reordering on output — use these everywhere.

#pragma once

#define PIXEL_OFF     ((uint32_t)0)
#define PIXEL_RED     ((uint32_t)0x00C80000)  // Color(200,   0,   0)
#define PIXEL_GREEN   ((uint32_t)0x0000C800)  // Color(  0, 200,   0)
#define PIXEL_BLUE    ((uint32_t)0x000000C8)  // Color(  0,   0, 200)
#define PIXEL_VIOLET  ((uint32_t)0x009400D3)  // Color(148,   0, 211)
#define PIXEL_ORANGE  ((uint32_t)0x00DC5000)  // Color(220,  80,   0)
#define PIXEL_YELLOW  ((uint32_t)0x00DCB400)  // Color(220, 180,   0)
