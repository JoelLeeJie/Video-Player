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

void* buffer_to_free = nullptr;

/*---------------------------
VideoPlayer class variables*/
std::string VideoPlayer::video_filepath;
VideoFile* VideoPlayer::video_file;
bool VideoPlayer::isRun_Video;
double VideoPlayer::curr_video_time;
int VideoPlayer::audio_stream_index, VideoPlayer::video_stream_index;
AVFrame** VideoPlayer::next_audio_frame, ** VideoPlayer::next_video_frame;
SDL_AudioSpec VideoPlayer::audio_device_specs;

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


	//Initialize output audio device to output in desired format.
	//Needs to be runned once at the start, to enable AudioCallback to start taking in audio input continuously
	InitializeAudioDevice(video_file->GetStreamData(audio_stream_index).codecContext);

	isRun_Video = true;
	return true;
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
		num_retries = 2;
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



	//Every frame, update the new time.
	//Sync up with the slowest stream.
	if (audio_stream_index != -1 && video_stream_index != -1)
	{
		double audio_stream_time = video_file->GetCurrentPTSTIME(CodecType::AUDIOCODEC);
		double video_stream_time = video_file->GetCurrentPTSTIME(CodecType::VIDEOCODEC);
		curr_video_time = (audio_stream_time < video_stream_time) ? audio_stream_time : video_stream_time;
	}
	//This can be changed to make the video run faster or slower.
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
		const VideoFileError* err = video_file->checkIsValid();
		if (err)
		{
			std::cout << err->message;
			video_file->ResetErrorCodes();
		}
	}
	if (!next_audio_frame)
	{
		const VideoFileError* err = video_file->checkIsValid();
		if (err)
		{
			std::cout << err->message;
			video_file->ResetErrorCodes();
		}
	}
}
void VideoPlayer::Free()
{
	//TODO: need to fully free everything, to be able to keep taking new videos.
	if (video_file) delete video_file;
}

/*
	Write data in AVFrame to the buffer.
	If data written does not == buffer_length, then get more data to write.
	If there's excess data in AVFrame, then store it temporarily and wait for the next callback to put it in.
*/
void VideoPlayer::AudioCallback(void* userdata, Uint8* output_buffer, int buffer_length)
{
	//Don't play audio if it's ahead of actual video time.
	double stream_timestamp = video_file->GetCurrentPTSTIME(CodecType::AUDIOCODEC);
	if ((stream_timestamp > curr_video_time) || audio_stream_index == -1) return;

	/*if (next_audio_frame == nullptr || (*next_audio_frame)->data[0] == nullptr || (*next_audio_frame)->data[0][0] == '\0') return;*/

	//Stores excess audio data until next callback. 192000 is the max data size for audio codec in ffmpeg, so just make it 200000 jic.
	static Uint8 audio_buffer[200000]{};
	static int stored_data_size = 0;
	static int stored_data_index = 0;

	while (buffer_length > 0)
	{
		if (stored_data_size > 0)
		{
			//Data stored is more than what's necessary, so just transfer until buffer is full and return.
			if (stored_data_size > buffer_length)
			{
				//Start from [index, index + length - 1]
				std::memcpy(output_buffer, audio_buffer + stored_data_index, buffer_length);
				stored_data_size -= buffer_length;
				//New index is index + length, or rather the endpt of the transfer.
				stored_data_index += buffer_length;
				return;
			}
			//If not, it means data needed is < what's currently available.
			std::memcpy(output_buffer, audio_buffer + stored_data_index, stored_data_size); //Copy over everything.
			buffer_length -= stored_data_size;
			//No data left in buffer, so reset both size and index.
			stored_data_size = 0;
			stored_data_index = 0;
			if (buffer_length == 0) return;
		}
		int retries = 2;
		for (; retries > 0; retries--)
		{
			if (GetAudio(audio_buffer, &stored_data_size)) break;
		}
	}
}

/*
	Stores audio inside audio_buffer provided.
	Returns false if unable to.
	@param audio_buffer
	- EMPTY buffer to store data into.
*/
bool VideoPlayer::GetAudio(Uint8* audio_buffer, int* stored_size)
{
	if (audio_stream_index == -1) return false;
	next_audio_frame = video_file->GetFrame(CodecType::AUDIOCODEC);
	if (next_audio_frame == nullptr) return false;
	AVCodecContext* codec_ctxt = video_file->GetStreamData(audio_stream_index).codecContext;
	if (codec_ctxt == nullptr) return false;

	//Convert audio into format used by audio device.
	//Then put into audio buffer.
	SwrContext* resampler = swr_alloc_set_opts(NULL,
		codec_ctxt->channel_layout,
		AV_SAMPLE_FMT_S16,
		44100,
		codec_ctxt->channel_layout,
		codec_ctxt->sample_fmt,
		codec_ctxt->sample_rate,
		0,
		NULL);
	swr_init(resampler);

	int ret = 0;
	AVFrame* audioframe = av_frame_alloc();
	int dst_samples = (*next_audio_frame)->channels * av_rescale_rnd(
		swr_get_delay(resampler, (*next_audio_frame)->sample_rate)
		+ (*next_audio_frame)->nb_samples,
		44100,
		(*next_audio_frame)->sample_rate,
		AV_ROUND_UP);
	uint8_t* audiobuf = NULL;
	ret = av_samples_alloc(&audiobuf,
		NULL,
		1,
		dst_samples,
		AV_SAMPLE_FMT_S16,
		1);
	dst_samples = (*next_audio_frame)->channels * swr_convert(
		resampler,
		&audiobuf,
		dst_samples,
		(const uint8_t**)(*next_audio_frame)->data,
		(*next_audio_frame)->nb_samples);
	ret = av_samples_fill_arrays(audioframe->data,
		audioframe->linesize,
		audiobuf,
		1,
		dst_samples,
		AV_SAMPLE_FMT_S16,
		1);

	swr_free(&resampler);

	int stored_end_index{};
	////Copy all data from frame to the buffer.
	//for (int channel = 0; channel < audioframe->channels; channel++)
	//{
	//	//Don't check if the channel is null.
	//	if (audioframe->data[channel] == nullptr) continue;
	//	//Iterate over all data within the channel.
	//	for (int i = 0; i < audioframe->linesize[0]; i++)
	//	{
	//		//TODO: maybe? 
	//		//Null terminator indicates end of channel.
	//		//if (audioframe->data[channel][i] == 0) break;
	//		audio_buffer[stored_end_index] = audioframe->data[channel][i];
	//		stored_end_index++;
	//	}
	//}
	for (int i = 0; i < audioframe->linesize[0]; i++)
	{
		//TODO: maybe? 
		//Null terminator indicates end of channel.
		//if (audioframe->data[channel][i] == 0) break;
		audio_buffer[stored_end_index] = audioframe->data[0][i];
		stored_end_index++;
	}
	*stored_size = stored_end_index;
	//TODO: free avframe "audioframe".

}




bool VideoPlayer::InitializeAudioDevice(const AVCodecContext* audio_codec_context)
{
	//TODO: account for when audio device format (have) does not match actual audio from codec_context, and convert audio to match.
	if (!audio_codec_context)
	{
		std::cout << "Accessing non-existent codec context in DisplayWindow::PlayAVFrame()\n";
		return false;
	}
	//Taken from https://stackoverflow.com/questions/55438697/playing-sound-from-a-video-using-ffmpeg-and-sdl-queueaudio-results-in-high-pitch
	SDL_AudioSpec want{};
	SDL_zero(want);
	want.freq = 44100;
	want.channels = audio_codec_context->channels;
	want.format = AUDIO_S16SYS; //Converting this to audio_codec_context->sample_fmt will give a different sound.
	want.silence = 0;
	want.samples = 1024;
	want.userdata = (void*)audio_codec_context;
	want.callback = VideoPlayer::AudioCallback;
	static SDL_AudioDeviceID device = SDL_OpenAudioDevice(NULL, 0, &want, &audio_device_specs, 0);
	if (device == 0)
	{
		std::cout << SDL_GetError();
		return false;
	}

	//TODO: NOTE that VideoPlayer::AudioCallback is the one that is actually passing in the data, this just inits device.

	//SwrContext* resampler = swr_alloc_set_opts(NULL,
	//	audio_codec_context->channel_layout,
	//	AV_SAMPLE_FMT_S16,
	//	44100,
	//	audio_codec_context->channel_layout,
	//	audio_codec_context->sample_fmt,
	//	audio_codec_context->sample_rate,
	//	0,
	//	NULL);
	//swr_init(resampler);

	//int ret = 0;
	//AVFrame* frame = *audio_frame;
	//AVFrame* audioframe = av_frame_alloc();
	//int dst_samples = frame->channels * av_rescale_rnd(
	//	swr_get_delay(resampler, frame->sample_rate)
	//	+ frame->nb_samples,
	//	44100,
	//	frame->sample_rate,
	//	AV_ROUND_UP);
	//uint8_t* audiobuf = NULL;
	//ret = av_samples_alloc(&audiobuf,
	//	NULL,
	//	1,
	//	dst_samples,
	//	AV_SAMPLE_FMT_S16,
	//	1);
	//dst_samples = frame->channels * swr_convert(
	//	resampler,
	//	&audiobuf,
	//	dst_samples,
	//	(const uint8_t**)frame->data,
	//	frame->nb_samples);
	//ret = av_samples_fill_arrays(audioframe->data,
	//	audioframe->linesize,
	//	audiobuf,
	//	1,
	//	dst_samples,
	//	AV_SAMPLE_FMT_S16,
	//	1);
	/*SDL_QueueAudio(device,
		(*audio_frame)->data[0],
		(*audio_frame)->linesize[0]);*/
	SDL_PauseAudioDevice(device, 0);

	//TODO: free avframe "audioframe".
	//swr_free(&resampler);
	//SDL_CloseAudioDevice(device);
}