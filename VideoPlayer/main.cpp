/*
	File Name: main.cpp

	Brief: Entry point and main controller of program execution.
*/

#include "Video.hpp"
#include "Utility.hpp"
#include "types.hpp"
#include "Display.hpp"



/*-----------------------
Functions*/
bool InitializeSystem();
void Update();
void Draw();
void FreeSystem();

/*
	Entry point of the program.
	Controls program flow.
*/
int main(int argc, char **argv)
{
	//Temp error code to indicate unable to initialize system.
	if(!InitializeSystem()) return 10;
	//While program is running
	SDL_Event sdl_event; bool quit = false;
	while (!quit)
	{
		//TODO: Placeholder filename. Next time use window's filemanager to add in files. 
		//If unsuccessful Initialization, VideoPlayer::Free will still run, but not the inner loop.
		if (!VideoPlayer::Initialize("C:/Users/onwin/source/repos/VideoPlayer/Assets/Demo/Everlasting Flames-Honkai Impact 3rd.mp4"))
		{
			//Unable to initialize video file, so choose another one.
			DisplayWindow::DisplayMessageBox("Select another video");
		}
		//While video is running
		while (VideoPlayer::isRun_Video && !quit)
		{
			Update();
			Draw();
			//SDL event handling.
			while (SDL_PollEvent(&sdl_event))
			{
				if (sdl_event.type == SDL_QUIT) quit = true;
			}
		}
		VideoPlayer::Free();
	}

	FreeSystem();
}

bool InitializeSystem()
{
	//Initialize the program's SDL window.
	if(!DisplayWindow::Initialize()) return false;
}

void Update()
{
	Utility::UpdateDeltaTime();
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