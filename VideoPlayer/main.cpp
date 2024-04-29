#include <iostream>
#include "Display.hpp"        
#include "ffmpeg_videoFileFunctions.hpp"
#include "types.hpp"
#include <chrono>
#include <ctime> 
#include <exception>

int windowDimensions[2]{};
double videoCurrTime{};

/*
	If not called for a long period, may wish to ignore the first value provided.
*/
double GetTimeSinceLastCall()
{
	static std::chrono::system_clock::time_point lastCalledTime = std::chrono::system_clock::now();
	std::chrono::duration<double> elapsed_seconds = std::chrono::system_clock::now() - lastCalledTime;
	lastCalledTime = std::chrono::system_clock::now();
	return elapsed_seconds.count();
}

int main(int argc, char** argv)
{
	//Has to be done for sdl to be used.
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER | SDL_INIT_EVENTS) < 0) return 1;

	//Get computer's dimensions.
	SDL_DisplayMode dm;
	if (SDL_GetDesktopDisplayMode(0, &dm) != 0)
	{
		SDL_Log("Failed to get computer dimensions"); 
		return 1;
	}
	windowDimensions[0] = dm.w;
	windowDimensions[1] = dm.h;
	//TODO: remove, for testing purposes.
	windowDimensions[0] = 1000;
	windowDimensions[1] = 500;

	//Create the basic window/renderer/texture/rect to draw video on.
	SDL_Window* mainWindow = SDL_CreateWindow("Joel's Video Player", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, windowDimensions[0], windowDimensions[1], SDL_WINDOW_RESIZABLE);
	SDL_Renderer* mainWindow_renderer = SDL_CreateRenderer(mainWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE);
	SDL_SetRenderDrawColor(mainWindow_renderer, 0, 0, 0, 255);
	//This can be changed to different dimensions when window is resized.
	SDL_Texture* mainWindow_texture = SDL_CreateTexture(mainWindow_renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, windowDimensions[0], windowDimensions[1]);

	//TODO: use window's file manager to choose the filepath instead.
	//Init the video file and streams etc.
	VideoFile videoFile{ "C:/Users/onwin/source/repos/VideoPlayer/Assets/Demo/Everlasting Flames - Honkai Impact 3rd.mp4" };
	std::string errMsg{};
	if (!videoFile.checkIsValid(errMsg))
	{
		std::cout << errMsg;
	}
	videoFile.PrintDetails(std::cout);

	//SDL_Rect videoDisplayRect = videoFile.GetVideoDimensions();
	SDL_Rect videoDisplayRect = { windowDimensions[0]/4, windowDimensions[1]/4, windowDimensions[0]/2, windowDimensions[1]/2};


	int videoStreamIndex = videoFile.GetVideoStreamIndex();
	int audioStreamIndex = videoFile.GetAudioStreamIndex();
	if (videoStreamIndex == -1)
	{
		SDL_Log("Failed to Read Video from File");
	}
	if (audioStreamIndex == -1)
	{
		SDL_Log("Failed to Read Audio from File");
	}

	
	//TODO: Set it so that video seeking(e.g. going backwards/forwards in the video/audio) is allowed.
	AVFrame* next_videoFrame{};
	for (;videoCurrTime < static_cast<double>(videoFile.GetVideoDuration());)
	{
		double elapsedTime = GetTimeSinceLastCall();
		//e.g. if the video was paused, jic GetTimeSinceLastCall wasn't called for a long time.
		videoCurrTime += (elapsedTime<0.3f)? elapsedTime:0.0;
		//===For video===//
		
		//Do until an actual frame is available. May cause an infinite loop but just for testing.
		//Gets the next frame when its time.
		if (videoFile.GetCurrentPTSTIME(videoStreamIndex) < videoCurrTime)
		{
			while ((next_videoFrame = videoFile.GetFrame(videoStreamIndex)) == nullptr)
			{
				
			}
			//TODO: rescale the image.
			//videoFile.ResizeVideoFrame(next_videoFrame);
		}
		YUV420P_TO_SDLTEXTURE(next_videoFrame, mainWindow_texture, &videoDisplayRect);
		//Can add on UI and other things here.
		DrawTexture(mainWindow_renderer, mainWindow_texture);

		//===For Audio===//


		//===For Input===//
		std::cout << "Current Video " << videoCurrTime << "  " << "Frame pts time " << videoFile.GetCurrentPTSTIME(videoStreamIndex) << "\n";
	}
	


	//Window will stay up.
	SDL_Event e; bool quit = false;
	while (quit == false)
	{
		while (SDL_PollEvent(&e))
		{
			if (e.type == SDL_QUIT) quit = true;
		}
	}
	return 0;
}