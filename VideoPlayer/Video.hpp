/*
	File Name: Video.hpp

	Brief: Declares types and members for the class VideoPlayer, a static class representing 
	the video being currently played.
	Controls initialization, update, drawing and allocation/deallocation of video resources.
	To change video, call free and initialise.

	Only a single video can be played at a time.
*/

#include <string>

//There'll only be one instance of this class, representing the current video being played.
static class VideoPlayer
{
	static std::string video_filepath;


	public:
	static void Initialize(std::string video_filepath);
	static void Update();
	static void Draw();
	static void Free();
};