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
		//Just alloc memory for these two, no need to update.
		streamData.currPacket = av_packet_alloc();
		streamData.currFrame = av_frame_alloc();
	}
}

VideoFile::~VideoFile()
{
	for (StreamData& streamData : streamArr)
	{
		//Only dealloc these items, the rest is done in free_context.
		if (streamData.codecContext) avcodec_free_context(&streamData.codecContext);
		if (streamData.currPacket) av_packet_free(&streamData.currPacket);
		if (streamData.currFrame) av_frame_free(&streamData.currFrame);
	}
	if (videoContainer) avformat_free_context(videoContainer);
}

void VideoFile::PrintDetails(std::ostream& output)
{
	output << "Title: " << videoContainer->iformat->long_name << "\n"
		<< "Duration: " << videoContainer->duration/AV_TIME_BASE << " secs\n"
		<< "\n<<Stream Data>>\n";
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


//Returns nullptr if no frame can be read.
AVFrame* VideoFile::GetFrame(int index)
{
	StreamData& stream = streamArr[index];
	int errVal{};
	while ((errVal = avcodec_receive_frame(stream.codecContext, stream.currFrame)) != 0)
	{
		//Not successful,try to resolve.
		switch (errVal)
		{
		//Send a new packet, since incomplete frame.
		case AVERROR(EAGAIN):
			//error reading packet, maybe reached eof?
			if (av_read_frame(videoContainer, stream.currPacket) < 0)
			{
				return nullptr;
			}
			if ((errVal = avcodec_send_packet(stream.codecContext, stream.currPacket)) == 0)
			{
				continue; //try to read again.
			}
			//unable to send packet, and no frames can be read either, unable to resolve.
			return nullptr;
		//Unable to resolve.
		default:
			return nullptr;
		}
	}
	//read successful.
	return stream.currFrame;
}