#pragma once

/*
    TARGA Vision file header for TARGA 2 file format
*/

#ifndef TARGA_H
#define TARGA_H

#include <cstdint>


#pragma pack(push, 1)
typedef struct _tagTGAHDR
{
    uint8_t	    tgaIdentSize;          // size of ID field that follows 18 byte header (0 usually)
    uint8_t	    tgaColorMapType;      // type of colour map 0=none, 1=has palette
    uint8_t	    tgaImageType;          // type of image 0=none,1=indexed,2=rgb,3=grey,+8=rle packed

    uint16_t	tgaColorMapOffset;     // first colour map entry in palette
    uint16_t   	tgaColorMapLength;    // number of colours in palette
    uint8_t	    tgaColorMapBits;      // number of bits per palette entry 15,16,24,32

    uint16_t	tgaXStart;             // image x origin
    uint16_t	tgaYStart;             // image y origin
    uint16_t	tgaWidth;              // image width in pixels
    uint16_t	tgaHeight;             // image height in pixels
    uint8_t	    tgaBits;               // image bits per pixel 8,16,24,32
    uint8_t	    tgaDescriptor;         // image descriptor bits (vh flip bits)
} TARGAINFOHEADER;
#pragma pack(pop)


#endif