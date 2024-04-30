#include <iostream>
#include "Display.hpp"        
#include "ffmpeg_videoFileFunctions.hpp"
#include "types.hpp"
#include <chrono>
#include <ctime> 
#include <exception>
#include <sstream>

//Dimensions of user's computer.
int windowDimensions[2]{};

//Tracks current video run-time.
double videoCurrTime{};

//Temp variables
const VideoFileError* errorCheck{};
std::ostringstream tempMessage{};

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
	//======================INITIALISATION======================//
	//Has to be done for sdl to be used.
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER | SDL_INIT_EVENTS) < 0)
	{
		SDL_Log("Failed to initialise SDL");
		return 1;
	}

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
	
	if (!mainWindow || !mainWindow_renderer)
	{
		SDL_Log("Failed to init main window/renderer");
		return 1;
	}

	//======================INPUT LOOP======================//

	/*
		Choosing Video
	*/
	//TODO: use window's file manager to choose the filepath instead.
	//Init the video file and streams etc.
	VideoFile videoFile{ "C:/Users/onwin/source/repos/VideoPlayer/Assets/Demo/Everlasting Flames - Honkai Impact 3rd.mp4" };
	//Error code is returned.
	//TODO: loop again, and ask user to choose another video. Due to use of constructor, may need to use a ptr and "new/delete" instead.
	if (errorCheck = videoFile.checkIsValid())
	{
		SDL_Log(errorCheck->message.c_str());
		if (!errorCheck->canFind)
		{
			//Display Message box: "Invalid Filepath. Video Not Found"
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Joel's Video Player", "Invalid Filepath, Video Not Found\nChoose another Video\n", mainWindow);
		}
		if (!errorCheck->canRead || !errorCheck->canCodec)
		{
			//Display Message box: "Unable to read video file"
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Joel's Video Player", "Unable to read video file\nChoose another Video\n", mainWindow);
		}
		videoFile.ResetErrorCodes();
	}
	int videoStreamIndex = videoFile.GetVideoStreamIndex();
	int audioStreamIndex = videoFile.GetAudioStreamIndex();
	if (videoStreamIndex == -1)
	{
		SDL_Log("Failed to Read Video from File");
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Joel's Video Player", "No Video Available", mainWindow);
	}
	if (audioStreamIndex == -1)
	{
		SDL_Log("Failed to Read Audio from File");
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Joel's Video Player", "No Audio Available", mainWindow);
	}

	/*
		Video display size
	*/
	//This can be changed to different dimensions when window is resized.
	SDL_Texture* mainWindow_texture = SDL_CreateTexture(mainWindow_renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, windowDimensions[0], windowDimensions[1]);
	//SDL_Rect videoDisplayRect = videoFile.GetVideoDimensions();
	//Fullscreen, but can be changed.
	SDL_Rect videoDisplayRect = { windowDimensions[0]/4, windowDimensions[1]/4, windowDimensions[0]/2, windowDimensions[1]/2};

	
	//TODO: Set it so that video seeking(e.g. going backwards/forwards in the video/audio) is allowed.
	AVFrame* next_videoFrame{};
	/*
		Playing Video
	*/
	for (;videoCurrTime < static_cast<double>(videoFile.GetVideoDuration());)
	{
		double elapsedTime = GetTimeSinceLastCall();
		//e.g. if the video was paused, jic GetTimeSinceLastCall wasn't called for a long time.
		videoCurrTime += (elapsedTime < 0.3f) ? elapsedTime : 0.0;
		//===For video===//
		if (videoStreamIndex >= 0) //Check if video stream is available.
		{
			//Gets the next frame when it's time.
			if (videoFile.GetCurrentPTSTIME(videoStreamIndex) < videoCurrTime)
			{
				//Do until an actual frame is available. May cause an infinite loop but just for testing.
				while ((next_videoFrame = videoFile.GetFrame(videoStreamIndex)) == nullptr)
				{
					//Error occured.
					if ((errorCheck = videoFile.checkIsValid()) == nullptr) continue; //Try to read again if no detectable error.
					SDL_Log(errorCheck->message.c_str()); //For debugging.
					if (errorCheck->reachedEOF) break; //can't read from an empty stream. 
					if (!errorCheck->canRead || !errorCheck->canCodec)
					{
						//TODO: Need better error handling, try to resolve error before requesting new frame else may have infinite loop.
						videoFile.ResetErrorCodes();
						continue; 
					}
				}
				//Resize(and convert) image frame to correct dimensions and YUV420P format.
				//Note: dimensions based on videoDisplayRect.
				if(next_videoFrame) videoFile.ResizeVideoFrame(next_videoFrame, videoDisplayRect.w, videoDisplayRect.h);
				if (errorCheck = videoFile.checkIsValid())
				{
					//Resize error.
					SDL_Log(errorCheck->message.c_str());
					videoFile.ResetErrorCodes();
				}
			}
			try
			{
				if (next_videoFrame) YUV420P_TO_SDLTEXTURE(next_videoFrame, mainWindow_texture, &videoDisplayRect);
			}
			catch (std::exception& e)
			{
				SDL_Log(e.what());
			}
		}

		//===For Audio===//
		if (audioStreamIndex >= 0) //Check if audio stream is available.
		{




		}

		//===For Input===//
		tempMessage.clear();
		//tempMessage << "Current Video " << videoCurrTime << "  " << "Frame pts time " << videoFile.GetCurrentPTSTIME(videoStreamIndex) << "\n";
		//SDL_Log(tempMessage.str().c_str());
		
		//TODO: Allow video to be paused.
		//TODO: Add UI elements into mainWindow_texture.

		//Finally, draw texture
		//Can add on UI and other things before drawing.
		DrawTexture(mainWindow_renderer, mainWindow_texture);
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
	if(mainWindow_renderer) SDL_DestroyRenderer(mainWindow_renderer);
	if(mainWindow) SDL_DestroyWindow(mainWindow);
	if (mainWindow_texture) SDL_DestroyTexture(mainWindow_texture);
	return 0;
}