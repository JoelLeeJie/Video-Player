/*
	File Name: Display.cpp

	Brief: Provides utility functions to handle the program's window display.
*/


#include "Display.hpp"
#include <string>
#include <exception>
#include "types.hpp"
#include <iostream>
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

/*---------------
DisplayWindow class*/
SDL_Window* DisplayWindow::mainWindow;
SDL_Renderer* DisplayWindow::mainWindow_renderer;
int DisplayWindow::window_dimensions[2];
SDL_DisplayMode DisplayWindow::device_dimensions;
SDL_Texture* DisplayWindow::videoDisplayTexture;
SDL_Rect DisplayWindow::videoDisplayRect;


bool DisplayWindow::Initialize()
{
	//_putenv("SDL_AUDIODRIVER=directsound");
	//==========Initialize SDL window system.
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER | SDL_INIT_EVENTS) < 0)
	{
		SDL_Log("Failed to initialise SDL");
		return false;
	}

	//Get computer's dimensions.
	if (SDL_GetDesktopDisplayMode(0, &device_dimensions) != 0)
	{
		SDL_Log("Failed to get computer dimensions");
		return false;
	}

	//Temporary starting dimensions of program's window.
	window_dimensions[0] = 1400;
	window_dimensions[1] = 700;

	//Initialize the basic window/renderer/texture/rect to draw video on.
	mainWindow = SDL_CreateWindow("Joel's Video Player", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, window_dimensions[0], window_dimensions[1], SDL_WINDOW_RESIZABLE);
	mainWindow_renderer = SDL_CreateRenderer(mainWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE);
	SDL_SetRenderDrawColor(mainWindow_renderer, 0, 0, 0, 255);
	if (!mainWindow || !mainWindow_renderer)
	{
		SDL_Log("Failed to init main window/renderer");
		return false;
	}

	//Video display texture for now will be same as program window.
	videoDisplayTexture = SDL_CreateTexture(mainWindow_renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, window_dimensions[0], window_dimensions[1]);
	if (!videoDisplayTexture)
	{
		SDL_Log("Failed to init main window's texture");
		return false;
	}
	//The actual dimensions the video can display in, for now is set to window_dimensions.
	videoDisplayRect = { 0, 0, window_dimensions[0], window_dimensions[1] };
}


void DisplayWindow::Free()
{
	if (mainWindow_renderer) SDL_DestroyRenderer(mainWindow_renderer);
	if (mainWindow) SDL_DestroyWindow(mainWindow);
	if (videoDisplayTexture) SDL_DestroyTexture(videoDisplayTexture);
}

void DisplayWindow::DisplayMessageBox(std::string message)
{
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Joel's Video Player", message.c_str(), mainWindow);
}

bool DisplayWindow::DrawAVFrame(AVFrame** video_frame)
{
	DisplayUtility::YUV420P_TO_SDLTEXTURE(*video_frame, videoDisplayTexture, &videoDisplayRect);
	DisplayUtility::DrawTexture(mainWindow_renderer, videoDisplayTexture);
	//Successful drawing of the frame.
	return true;
}

bool DisplayWindow::PlayAVFrame(AVFrame** audio_frame, const AVCodecContext* audio_codec_context)
{
	if (!audio_frame || !audio_codec_context)
	{
		std::cout << "Accessing non-existent codec context/audio frame in DisplayWindow::PlayAVFrame()\n";
		return false;
	}
	//Taken from https://stackoverflow.com/questions/55438697/playing-sound-from-a-video-using-ffmpeg-and-sdl-queueaudio-results-in-high-pitch

	SwrContext* resampler = swr_alloc_set_opts(NULL,
		audio_codec_context->channel_layout,
		AV_SAMPLE_FMT_S16,
		44100,
		audio_codec_context->channel_layout,
		audio_codec_context->sample_fmt,
		audio_codec_context->sample_rate,
		0,
		NULL);
	swr_init(resampler);

	
	SDL_AudioSpec want, have;
	SDL_zero(want);
	SDL_zero(have);
	want.freq = 44100;
	want.channels = audio_codec_context->channels;
	want.format = AUDIO_S16SYS;
	want.silence = 0;
	//want.samples = 1024;
	static SDL_AudioDeviceID device = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
	if (device == 0)
	{
		std::cout << SDL_GetError();
		return false;
	}
	SDL_PauseAudioDevice(device, 0);

	SDL_QueueAudio(device, (*audio_frame)->data[0], (*audio_frame)->linesize[0]);
	//int ret = 0;
	//AVFrame* frame = *audio_frame;
	//AVFrame* audioframe = av_frame_alloc();
	//int dst_samples = frame->channels * av_rescale_rnd(
	//	swr_get_delay(resampler, frame->sample_rate)
	//	+ frame->nb_samples,
	//	44100,
	//	frame->sample_rate,
	//	AV_ROUND_UP);
	//uint8_t* audiobuf = NULL;
	//ret = av_samples_alloc(&audiobuf,
	//	NULL,
	//	1,
	//	dst_samples,
	//	AV_SAMPLE_FMT_S16,
	//	1);
	//dst_samples = frame->channels * swr_convert(
	//	resampler,
	//	&audiobuf,
	//	dst_samples,
	//	(const uint8_t**)frame->data,
	//	frame->nb_samples);
	//ret = av_samples_fill_arrays(audioframe->data,
	//	audioframe->linesize,
	//	audiobuf,
	//	1,
	//	dst_samples,
	//	AV_SAMPLE_FMT_S16,
	//	1);
	//SDL_QueueAudio(device,
	//	audioframe->data[0],
	//	audioframe->linesize[0]);


	swr_free(&resampler);
	//SDL_CloseAudioDevice(device);
}

namespace DisplayUtility
{
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
}
