#ifndef DISPLAY_HPP
#define DISPLAY_HPP

#include "types.hpp"
/*
Converts a video frame to an sdl texture, only within the rectangular area.
Call before changing the texture.
*/
void YUV420P_TO_SDLTEXTURE(AVFrame* imageFrame, SDL_Texture* texture, const SDL_Rect* image_displayArea);

/*
Inserts sdl texture into the renderer and presents it.
Call when texture is finished, as it clears renderer.
Will stretch the texture to fill the entire renderer.
*/
void DrawTexture(SDL_Renderer* renderer, SDL_Texture* texture);

/*
	Will return size of video display rectangle limited to limitDimensions, following aspect ratio.
	isTrueSize --> True to follow video size, false to expand video to limitDimensions(fit-to-following aspect ratio)
*/
SDL_Rect AdjustRectangle(SDL_Rect videoDimensions, SDL_Rect limitDimensions, bool isTrueSize = false);
#endif