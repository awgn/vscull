/*
    Copyright (c) 2009 Nicola Bonelli <n.bonelli@netresults.it>
 
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#ifndef _VSCULL_PALETTE_H_
#define _VSCULL_PALETTE_H_ 

#define PALETTE(n) ( (n < sizeof(palette_str)/sizeof(palette_str[0])) ? (palette_str[n]) : (palette_str[0]) )

#ifndef VIDEO_PALETTE_GREY
#define VIDEO_PALETTE_GREY  1           // Linear greyscale 
#define VIDEO_PALETTE_HI240 2           // High 240 cube (BT848) 
#define VIDEO_PALETTE_RGB565    3       // 565 16 bit RGB 
#define VIDEO_PALETTE_RGB24 4           // 24bit RGB 
#define VIDEO_PALETTE_RGB32 5           // 32bit RGB 
#define VIDEO_PALETTE_RGB555    6       // 555 15bit RGB 
#define VIDEO_PALETTE_YUV422    7       // YUV422 capture 
#define VIDEO_PALETTE_YUYV  8
#define VIDEO_PALETTE_UYVY  9           // The great thing about standards is ... 
#define VIDEO_PALETTE_YUV420    10
#define VIDEO_PALETTE_YUV411    11      // YUV411 capture 
#define VIDEO_PALETTE_RAW   12          // RAW capture (BT848) 
#define VIDEO_PALETTE_YUV422P   13      // YUV 4:2:2 Planar 
#define VIDEO_PALETTE_YUV411P   14      // YUV 4:1:1 Planar 
#define VIDEO_PALETTE_YUV420P   15      // YUV 4:2:0 Planar 
#define VIDEO_PALETTE_YUV410P   16      // YUV 4:1:0 Planar 
#define VIDEO_PALETTE_PLANAR    13      // start of planar entries 
#define VIDEO_PALETTE_COMPONENT 7       // start of component entries 
#endif

#ifndef __cplusplus
const char *palette_str[] =
{
    [0]                     = "UNKNOWN",
    [VIDEO_PALETTE_GREY]    = "GREY",
    [VIDEO_PALETTE_HI240]   = "High 240 cube (BT848)",
    [VIDEO_PALETTE_RGB565]  = "565 16 bit RGB",
    [VIDEO_PALETTE_RGB24]   = "24bit RGB",
    [VIDEO_PALETTE_RGB32]   = "32bit RGB",
    [VIDEO_PALETTE_RGB555]  = "555 15bit RGB",
    [VIDEO_PALETTE_YUV422]  = "YUV422 capture",
    [VIDEO_PALETTE_YUYV]    = "YUYV",
    [VIDEO_PALETTE_UYVY]    = "UYVY",
    [VIDEO_PALETTE_YUV420]  = "YUV420",
    [VIDEO_PALETTE_YUV411]  = "YUV411",
    [VIDEO_PALETTE_RAW]     = "RAW (BT848)",
    [VIDEO_PALETTE_YUV422P] = "YUV 4:2:2 Planar",
    [VIDEO_PALETTE_YUV411P] = "YUV 4:1:1 Planar",
    [VIDEO_PALETTE_YUV420P] = "YUV 4:2:0 Planar",
    [VIDEO_PALETTE_YUV410P] = "YUV 4:1:0 Planar"
};
#else
const char *palette_str[] =
{
    "UNKNOWN",
    "GREY",
    "High 240 cube (BT848)",
    "565 16 bit RGB",
    "24bit RGB",
    "32bit RGB",
    "555 15bit RGB",
    "YUV422 capture",
    "YUYV",
    "UYVY",
    "YUV420",
    "YUV411",
    "RAW (BT848)",
    "YUV 4:2:2 Planar",
    "YUV 4:1:1 Planar",
    "YUV 4:2:0 Planar",
    "YUV 4:1:0 Planar"
};

#endif
#endif /* _VSCULL_PALETTE_H_ */
