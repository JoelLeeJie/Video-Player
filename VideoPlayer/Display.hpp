/*
	File Name: Display.hpp

	Brief: Declares functions and class to handle the program's window display.

	TODO: Add capability for window resizing.
*/

#ifndef DISPLAY_HPP
#define DISPLAY_HPP

#include "types.hpp"
#include "ffmpeg_videoFileFunctions.hpp"
namespace DisplayUtility
{
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
}

/*
	Only one instance of this in the program. Controls the program's window.
*/
class DisplayWindow
{
	//The device dimensions and various parameters.
	static SDL_DisplayMode device_dimensions;
	//Dimensions for the program's window.
	static int window_dimensions[2];
	/*The dimensions in which the video can be displayed in.
	For example, additional UI may make this smaller than window dimensions.
	
	NEEDS TO BE LIMITED TO THE VIDEO DIMENSIONS.
	*/
	static SDL_Rect videoDisplayRect;

	//The window the program will run on.
	static SDL_Window* mainWindow;
	//Used to render the window
	static SDL_Renderer* mainWindow_renderer;
	//Used to draw on, before copying to the renderer.
	static SDL_Texture* videoDisplayTexture;

public:


	//=======Functions

	//Called once at the start of the program to initialise the program's window.
	static bool Initialize();
	//Called once at the end of the program to free resources related to the program's window.
	static void Free();

	static void DisplayMessageBox(std::string message);
	static bool DrawAVFrame(AVFrame** video_frame);

	//=======Setters and Getters
	static SDL_DisplayMode GetDeviceDimensions() { return device_dimensions; }
	static Vec2 GetWindowDimensions() {
		return Vec2{ static_cast<double>(window_dimensions[0]), static_cast<double>(window_dimensions[1]) };
	}
	static SDL_Rect GetVideoDimensions() {
		return videoDisplayRect;
	}
	//Needs to be run everytime a new video is used.
	static void LimitVideoDisplayRect_to_Video(const VideoFile* video_file){
		videoDisplayRect = DisplayUtility::AdjustRectangle(video_file->GetVideoDimensions(), videoDisplayRect, false);
	}
};


#endif