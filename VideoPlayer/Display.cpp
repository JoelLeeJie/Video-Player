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
	if (SDL_UpdateYUVTexture(texture, image_displayArea,
		imageFrame->data[0], imageFrame->linesize[0],
		imageFrame->data[1], imageFrame->linesize[1],
		imageFrame->data[2], imageFrame->linesize[2]) != 0)
	{
		std::string msg = "Unable to convert avframe to texture: ";
		msg += SDL_GetError();
		throw std::exception(msg.c_str());
	}
}

void DrawTexture(SDL_Renderer* renderer, SDL_Texture* texture)
{
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);
}
