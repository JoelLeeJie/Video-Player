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
class VideoPlayer
{
	static std::string video_filepath;
	//The file to read data from.
	static VideoFile* video_file;
	//Representing the current video timestamp. Used for sync.
	static double curr_video_time;

	static int audio_stream_index, video_stream_index;
	//Storage for the frames read from the file.
	static AVFrame **next_video_frame, **next_audio_frame;

	static SDL_AudioSpec audio_device_specs;

public:
	static bool isRun_Video;
	static SDL_AudioDeviceID audio_device;
	/*Requires DisplayWindow to be initialized.
	Called once every time a new video file is to be played.*/
	static bool Initialize(std::string video_filepath);
	static void Update();
	static void Draw();
	static void Free();
	static void AudioCallback(void* userdata, Uint8* buffer, int buffer_length);
	static bool InitializeAudioDevice(const AVCodecContext* audio_codec_context);
	static bool GetAudio(Uint8* audio_buffer, int* stored_size);

	static void SeekVideo(double offset);
};

