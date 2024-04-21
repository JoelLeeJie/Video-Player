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
	//Should be caught.
	if (videoContainer == nullptr) {
		throw std::invalid_argument(fileName);
	}
	//2. Open video/audio streams. 
	avformat_find_stream_info(videoContainer, NULL);

	//3. Get the codec for each stream.
	//Iterate over each stream in the array.
	for (int i = 0; i < videoContainer->nb_streams; i++)
	{
		//For each stream, find the codec param to find the codec id, and use it to find the codec.
		AVCodec* codec = avcodec_find_decoder(videoContainer->streams[i]->codecpar->codec_id);
	}
}

VideoFile::~VideoFile()
{
	if (videoContainer) avformat_free_context(videoContainer);
}

void VideoFile::PrintDetails(std::ostream& output)
{
	output << "Title: " << videoContainer->iformat->long_name << "\n"
		<< "Duration: " << videoContainer->duration << " secs\n";
}


