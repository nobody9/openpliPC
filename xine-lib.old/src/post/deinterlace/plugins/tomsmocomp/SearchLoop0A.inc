// -*- c++ -*-

// Searches just the center pixel, in both the old
//  and new fields, but takes averages. This is an even
// pixel address. Any chroma match will be used. (YUY2)
// We best like finding 0 motion so we will bias everything we found previously
// up by a little, and adjust later

#ifdef IS_SSE2
		"paddusb "MANGLE(ONES)", %%xmm7\n\t"				// bias toward no motion
#else
		"paddusb "MANGLE(ONES)", %%mm7\n\t" 				// bias toward no motion
#endif

        MERGE4PIXavg("(%%"XDI", %%"XCX")", "(%%"XSI", %%"XCX")")  // center, in old and new
