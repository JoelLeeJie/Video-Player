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
	while (true)
	{
		//Placeholder filename. Next time use window's filemanager to add in files.
		VideoPlayer::Initialize("C:/Users/onwin/source/repos/VideoPlayer/Assets/Demo/Everlasting Flames - Honkai Impact 3rd.mp4");
		//While video is running
		while (true)
		{
			Update();
			Draw();
		}
		VideoPlayer::Free();
	}

	FreeSystem();
}

bool InitializeSystem()
{
	//==========Initialize SDL window system.
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER | SDL_INIT_EVENTS) < 0)
	{
		SDL_Log("Failed to initialise SDL");
		return false;
	}

	//Get computer's dimensions.
	if (SDL_GetDesktopDisplayMode(0, ) != 0)
	{
		SDL_Log("Failed to get computer dimensions");
		return 1;
	}
}

void Update()
{
	Utility::UpdateDeltaTime();
}

void Draw()
{

}

void FreeSystem()
{

}