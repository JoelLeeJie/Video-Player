#include "Display.hpp"
#include <string>
#include <exception>

/*
 //init
  SDL_Init(SDL_INIT_VIDEO);
  SDL_Window *win;
  SDL_Renderer *rend;
  SDL_Texture *texture;

  //define
  win = SDL_CreateWindow("C.H.A.O.S.",
			 SDL_WINDOWPOS_CENTERED,
			 SDL_WINDOWPOS_CENTERED,
			 800, 800,
			 SDL_WINDOW_BORDERLESS);

  rend = SDL_CreateRenderer(win,
							-1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

  int t_length = 2;
  int t_width = 2;
  texture = SDL_CreateTexture(rend,
				  SDL_PIXELFORMAT_ARGB8888,
				  SDL_TEXTUREACCESS_STREAMING,
				  t_length,
				  t_width);


  //pixels
  uint8_t *pixels;
  int pitch = t_width * t_length* 4;

  SDL_LockTexture(texture, NULL, (void **)&pixels, &pitch);

  for (int i=0; i<t_width * t_length * 4; i++){
	*(pixels + i) = 0;
  }

  for (int i=0; i<t_width; i++) {
	*(pixels + i) = 255;
  }


  SDL_UnlockTexture(texture);

  SDL_RenderClear(rend);
  SDL_RenderCopy(rend, texture, NULL, NULL);
  SDL_RenderPresent(rend);

  SDL_Delay(3000);

  SDL_DestroyWindow(win);

  SDL_Quit();

  return 0;


*/
//https://stackoverflow.com/questions/17579286/sdl2-0-alternative-for-sdl-overlay

void YUV420P_TO_SDLTEXTURE(AVFrame* imageFrame, SDL_Texture* texture, const SDL_Rect* image_displayArea)
{
	if (!imageFrame || !texture || !image_displayArea) return;
	if (SDL_UpdateYUVTexture(texture, image_displayArea,
		imageFrame->data[0], imageFrame->linesize[0],
		imageFrame->data[1], imageFrame->linesize[1],
		imageFrame->data[2], imageFrame->linesize[2]) != 0)
	{
		std::string msg = "Unable to convert avframe to texture: ";
		msg += SDL_GetError(); 
		msg += "\n";
		throw std::exception(msg.c_str());
	}
}
void DrawTexture(SDL_Renderer* renderer, SDL_Texture* texture)
{
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);
}

//Fits video into dimensions, following original aspect ratio.
SDL_Rect AdjustRectangle(SDL_Rect videoDimensions, SDL_Rect limitDimensions, bool isTrueSize)
{
	//If height too big, or width too big, then readjust.
	double aspectRatio_widthHeight = static_cast<double>(videoDimensions.w) / videoDimensions.h;
	//Expand video to fit max dimensions first.
	if (!isTrueSize)
	{
		videoDimensions = limitDimensions;
		videoDimensions.w *= 2; //To trigger the aspect ratio adjustment
	}
	//Limit the width
	if (videoDimensions.w > limitDimensions.w)
	{
		videoDimensions.w = limitDimensions.w;
		videoDimensions.h = videoDimensions.w / aspectRatio_widthHeight;
	}
	//Limit the height. Note that this comes after it's already limited by above.
	if (videoDimensions.h > limitDimensions.h)
	{
		videoDimensions.h = limitDimensions.h;
		videoDimensions.w = videoDimensions.h * aspectRatio_widthHeight;
	}
	//Center video if it doesn't fit perfectly.
	videoDimensions.x = limitDimensions.x + (limitDimensions.w - videoDimensions.w) / 2.0;
	videoDimensions.y = limitDimensions.y + (limitDimensions.h - videoDimensions.h) / 2.0;

	return videoDimensions;
}