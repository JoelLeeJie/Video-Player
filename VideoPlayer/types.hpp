/*
	File Name: types.hpp

	Brief: Provides basic includes and type definitions used by most files.
*/

#ifndef TYPES_HPP
#define TYPES_HPP

extern "C" //Required for compatibility with c++.
{
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
	#include <libavutil/avutil.h>
	#include <libswscale/swscale.h>
	#include <libavformat/avformat.h>
	#include <libavutil/dict.h>
	#include <libavutil/imgutils.h>
}
#include <SDL.h>
#include <SDL_thread.h>
#endif