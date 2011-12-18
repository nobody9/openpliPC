/**
 * Copyright (c) 2000 John Adcock, Tom Barry, Steve Grimm  All rights reserved.
 * port copyright (c) 2003 Miguel Freitas
 *
 * This code is ported from DScaler: http://deinterlace.sf.net/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>

#if HAVE_INTTYPES_H
#include <inttypes.h>
#else
#include <stdint.h>
#endif

#include <xine/attributes.h>
#include <xine/xineutils.h>
#include "deinterlace.h"
#include "speedtools.h"
#include "speedy.h"
#include "plugins.h"

// debugging feature
// output the value of mm4 at this point which is pink where we will weave
// and green were we are going to bob
// uncomment next line to see this
//#define CHECK_BOBWEAVE

static int GreedyTwoFrameThreshold = 4;
static int GreedyTwoFrameThreshold2 = 8;

#define IS_SSE 1
#include "greedy2frame_template.c"
#undef IS_SSE

static deinterlace_method_t greedy2framemethod =
{
    "Greedy 2-frame (DScaler)",
    "Greedy2Frame",
    4,
    MM_ACCEL_X86_MMXEXT,
    0,
    0,
    0,
    0,
    DeinterlaceGreedy2Frame_SSE,
    1,
    NULL
};

deinterlace_method_t *greedy2frame_get_method( void )
{
    return &greedy2framemethod;
}

