#include "ffmpeg_videoFileFunctions.hpp"
#include <iostream>
#include <fstream>
#include <exception>

AVFormatContext* GetAVFormat(const std::string &fileName)
{
	AVFormatContext* returnVal = avformat_alloc_context();
	avformat_open_input(&returnVal, fileName.c_str(), NULL, NULL);
	return returnVal;
}

VideoFile::VideoFile(const std::string &fileName)
{
	//1. Point to the video file
	videoContainer = GetAVFormat(fileName);
	//Don't carry on with opening the rest.
	if (videoContainer == nullptr) {
		return;
	}
	//2. Open video/audio streams. 
	avformat_find_stream_info(videoContainer, NULL);

	//3. Get the codec data for each stream.
	//Iterate over each stream in the array.
	for (unsigned int i = 0; i < videoContainer->nb_streams; i++)
	{
		streamArr.push_back({});
		StreamData& streamData = streamArr.back();
		//For each stream, find the codec param to find the codec id, and use it to find the codec.
		streamData.codecParam = videoContainer->streams[i]->codecpar;
		streamData.codec = avcodec_find_decoder(videoContainer->streams[i]->codecpar->codec_id);
		//Need to alloc memory for codecContext.
		streamData.codecContext = avcodec_alloc_context3(streamData.codec);
		//Assigning data to context.
		avcodec_parameters_to_context(streamData.codecContext, streamData.codecParam);
		avcodec_open2(streamData.codecContext, streamData.codec, NULL);
	}
}

VideoFile::~VideoFile()
{
	for (StreamData& streamData : streamArr)
	{
		//Only need to dealloc codecContext here.
		if (streamData.codecContext) avcodec_free_context(&streamData.codecContext);
	}
	if (videoContainer) avformat_free_context(videoContainer);
}

void VideoFile::PrintDetails(std::ostream& output)
{
	output << "Title: " << videoContainer->iformat->long_name << "\n"
		<< "Duration: " << videoContainer->duration << " secs\n"
		<< "Stream Data\n";
	for (const StreamData& streamData : streamArr)
	{
		switch (streamData.codecParam->codec_type)
		{
		case AVMEDIA_TYPE_VIDEO:
			output << "Video Codec: resolution " << streamData.codecParam->width << " x " << streamData.codecParam->height;
			break;
		case AVMEDIA_TYPE_AUDIO:
			output << "Audio Codec: channels " << streamData.codecParam->channels << ", sample rate " << streamData.codecParam->sample_rate;
			break;
		}
		output << "\nBitRate: " << streamData.codecParam->bit_rate << "\n";
	}
}

bool VideoFile::checkIsValid(std::string& outputMessage)
{
	if (!videoContainer)
	{
		outputMessage = "Invalid Input File\n";
		return false;
	}
	return true;
}

