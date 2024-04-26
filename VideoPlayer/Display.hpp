#ifndef DISPLAY_HPP
#define DISPLAY_HPP

#include "types.hpp"
int YUV420P_TO_SDLTEXTURE(AVFrame* imageFrame, SDL_Texture* texture, const SDL_Rect* image_displayArea);

void DrawTexture(SDL_Renderer* renderer, SDL_Texture* texture);

#endif