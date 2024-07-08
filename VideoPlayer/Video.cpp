/*
	File Name: Video.cpp

	Brief: Defines types and members for the class VideoPlayer, a static class representing
	the video being currently played.
	Controls initialization, update, drawing and allocation/deallocation of video resources.
	To change video, call free and initialise.

	Only a single video can be played at a time.

	Handles reading data from video file through ffmpeg_videoFileFunctions, as well as displaying data through DisplayWindow.
*/

#include "Video.hpp"
#include "Display.hpp"
#include "Utility.hpp"
#include <iostream>

/*---------------------------
VideoPlayer class variables*/
std::string VideoPlayer::video_filepath;
VideoFile* VideoPlayer::video_file;
bool VideoPlayer::isRun_Video;
double VideoPlayer::curr_video_time;
int VideoPlayer::audio_stream_index, VideoPlayer::video_stream_index;
AVFrame **VideoPlayer::next_audio_frame, **VideoPlayer::next_video_frame;

bool VideoPlayer::Initialize(std::string video_filepath)
{
	//If initialization is unsuccessful, it indicates to not run VideoPlayer::Update and Draw loop.
	//If successful, set it to true at the end.
	isRun_Video = false;
	//=======Initialize video file.
	VideoPlayer::video_filepath = video_filepath;
	video_file = new VideoFile{ video_filepath };
	curr_video_time = 0.0;
	//=======Check if video_file is successfully created.
	const VideoFileError* errorChecker;
	if (errorChecker = video_file->checkIsValid())
	{
		SDL_Log(errorChecker->message.c_str());
		if (!errorChecker->canFind)
		{
			//Display Message box: "Invalid Filepath. Video Not Found"
			DisplayWindow::DisplayMessageBox("Invalid Filepath, Video Not Found\nChoose another Video\n");
		}
		if (!errorChecker->canRead || !errorChecker->canCodec)
		{
			//Display Message box: "Unable to read video file"
			DisplayWindow::DisplayMessageBox("Unable to read video file\nChoose another Video\n");
		}
		video_file->ResetErrorCodes();
		//Unable to initialize video, so deallocate resources and return false.
		delete video_file;
		video_file = nullptr;
		return false;
	}

	//====Check which audio/video streams are available. It's fine to continue running even if one of them is missing.
	video_stream_index = video_file->GetVideoStreamIndex();
	audio_stream_index = video_file->GetAudioStreamIndex();
	if (video_stream_index == -1)
	{
		SDL_Log("Failed to Read Video from File");
		DisplayWindow::DisplayMessageBox("No Video Available");
	}
	if (audio_stream_index == -1)
	{
		SDL_Log("Failed to Read Audio from File");
		DisplayWindow::DisplayMessageBox("No Audio Available");
	}

	//=========Initializing other aspects
	//Constraints video display dimensions to aspect ratio of the video.
	DisplayWindow::LimitVideoDisplayRect_to_Video(video_file);



	isRun_Video = true; 
}
void VideoPlayer::Update()
{
	double stream_timestamp; 
	int num_retries = 2;
	//Used to resize the video to fit within program window.
	SDL_Rect video_dimensions = DisplayWindow::GetVideoDimensions();
	
	//Playing video stream, check if it has a video stream first.
	if (video_stream_index != -1)
	{
		//Get the stream timestamp to compare to the actual video timestamp.
		stream_timestamp = video_file->GetCurrentPTSTIME(CodecType::VIDEOCODEC);
		//Get the next frame when it's time.
		if (stream_timestamp < curr_video_time)
		{
			next_video_frame = video_file->GetFrame(CodecType::VIDEOCODEC);
			//It may fail sometimes, retry for X num of times before giving up.
			while (num_retries-- && next_video_frame == nullptr)
			{
				next_video_frame = video_file->GetFrame(CodecType::VIDEOCODEC);
			}
			if (next_video_frame)
			{
				video_file->ResizeVideoFrame(*next_video_frame, video_dimensions.w, video_dimensions.h);
			}
		}
	}

	//TODO: Audio


	curr_video_time += Utility::deltaTime;
}
void VideoPlayer::Draw()
{
	if (next_video_frame)
	{
		DisplayWindow::DrawAVFrame(next_video_frame);
	}
	else
	{
		std::cout << video_file->checkIsValid()->message;
		video_file->ResetErrorCodes();
	}
}
void VideoPlayer::Free()
{
	if(video_file) delete video_file;
}