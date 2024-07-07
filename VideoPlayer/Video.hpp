/*
	File Name: Video.hpp

	Brief: Declares types and members for the class VideoPlayer, a static class representing 
	the video being currently played.
	Controls initialization, update, drawing and allocation/deallocation of video resources.
	To change video, call free and initialise.

	Only a single video can be played at a time.
		
	Handles reading data from video file through ffmpeg_videoFileFunctions, as well as displaying data through DisplayWindow.
*/

#include <string>
#include "ffmpeg_videoFileFunctions.hpp"
/*
	There'll only be one instance of this class, representing the current video being played.
	Does not only control reading of data from file, but also displaying of data to window. 
*/
static class VideoPlayer
{
	static std::string video_filepath;
	static VideoFile* video_file;

public:
	static bool isRun_Video;

	//Requires DisplayWindow to be initialized.
	static bool Initialize(std::string video_filepath);
	static void Update();
	static void Draw();
	static void Free();
};