/*
	File Name: main.cpp

	Brief: Entry point and main controller of program execution.
*/

#include "Video.hpp"
#include "Utility.hpp"
#include "types.hpp"
#include "Display.hpp"
#include "shobjidl_core.h"
#include "Windows.hpp"
#include <iostream>

/*-----------------------
Functions*/
bool InitializeSystem();
void Update();
void Draw();
void FreeSystem();
void Input();

/*----------------------
* Global varables*/
bool isPaused = false;

/*
	Entry point of the program.
	Controls program flow.
*/
int main(int argc, char** argv)
{
	//Temp error code to indicate unable to initialize system.
	if (!InitializeSystem()) return 10;
	//While program is running
	SDL_Event sdl_event; bool quit = false;
	//Runs every video chosen.
	while (!quit)
	{
		//Prevents user from being unable to close window.
		while (SDL_PollEvent(&sdl_event))
		{
			if (sdl_event.type == SDL_QUIT) quit = true;
		}

		std::string video_filepath = BasicFileOpen();
		if (video_filepath.empty()) continue;
		//TODO: Placeholder filename. Next time use window's filemanager to add in files. 
		//If unsuccessful Initialization, VideoPlayer::Free will still run, but not the inner loop.
		if (!VideoPlayer::Initialize(video_filepath)) //"C:/Users/onwin/source/repos/VideoPlayer/Assets/Demo/Everlasting Flames-Honkai Impact 3rd.mp4"
		{
			//Unable to initialize video file, so choose another one.
			DisplayWindow::DisplayMessageBox("Select another video");
			continue;
		}
		//While video is running
		while (VideoPlayer::isRun_Video && !quit)
		{
			Utility::UpdateDeltaTime();
			Input();
			//Closing window will cause both inner and outer loops to break after appropriate freeing of resources.
			while (SDL_PollEvent(&sdl_event))
			{
				if (sdl_event.type == SDL_QUIT) quit = true;
			}
			if (isPaused) continue; //Don't continue updating or drawing.
			Update();
			Draw();
		}
		VideoPlayer::Free();
	}

	FreeSystem();
	return 0;
}

bool InitializeSystem()
{
	//Initialize the program's SDL window.
	if (!DisplayWindow::Initialize()) return false;
	return true;
}

void Update()
{
	VideoPlayer::Update();
}

void Draw()
{
	VideoPlayer::Draw();
}

void FreeSystem()
{
	DisplayWindow::Free();
}

void Input()
{
	static float input_delay = 0;
	//SDL event handling.
	SDL_PumpEvents(); //check for new events.
	const Uint8* keyboard = SDL_GetKeyboardState(nullptr);
	if (input_delay <= 0)
	{
		if (keyboard[SDL_SCANCODE_SPACE])
		{
			input_delay = 0.2f;
			isPaused = !isPaused;
			std::cout << "paused/unpaused\n";
			if (isPaused)
			{
				if (VideoPlayer::audio_device != 0) SDL_PauseAudioDevice(VideoPlayer::audio_device, 1);
			}
			else
			{
				if (VideoPlayer::audio_device != 0) SDL_PauseAudioDevice(VideoPlayer::audio_device, 0);
			}
		}

		//Seek right by 10s.
		if (keyboard[SDL_SCANCODE_RIGHT])
		{
			input_delay = 0.2f;
			VideoPlayer::SeekVideo(10.0);
		}
		//Seek left by 10s.
		if (keyboard[SDL_SCANCODE_LEFT])
		{
			input_delay = 0.2f;
			VideoPlayer::SeekVideo(-10.0);
		}
	}
	input_delay -= static_cast<float>(Utility::deltaTime);
}