#include "ffmpeg_videoFileFunctions.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <exception>

AVFormatContext* GetAVFormat(const std::string &fileName)
{
	AVFormatContext* returnVal = avformat_alloc_context();
	int err;
	if ((err = avformat_open_input(&returnVal, fileName.c_str(), NULL, NULL)) != 0)
	{
		std::string msg = "Error opening video file: AVERROR ";
		msg += err;
		throw std::invalid_argument(msg); //TODO: catch and display ("Video not found") message box to user instead.
	}
	return returnVal;
}

VideoFile::VideoFile(const std::string &fileName)
{
	//1. Point to the video file
	try
	{
		videoContainer = GetAVFormat(fileName);
	}
	catch (std::invalid_argument& e)
	{
		throw e; //Unable to open file.
	}

	//2. Open video/audio streams. 
	if (avformat_find_stream_info(videoContainer, NULL) < 0)
	{
		throw std::exception("Unable to open video streams");
	}

	//3. Get the codec data/context for each stream.
	//Iterate over each stream in the array and fill in streamData for streamArr.
	for (unsigned int i = 0; i < videoContainer->nb_streams; i++)
	{
		streamArr.push_back({});
		StreamData& streamData = streamArr.back();
		streamData.stream = videoContainer->streams[i];
		//For each stream, find the codec param to find the codec id, and use it to find the codec.
		streamData.codecParam = videoContainer->streams[i]->codecpar;
		streamData.codec = avcodec_find_decoder(videoContainer->streams[i]->codecpar->codec_id);
		//Need to alloc memory for codecContext.
		streamData.codecContext = avcodec_alloc_context3(streamData.codec);
		//Assigning data to context.
		avcodec_parameters_to_context(streamData.codecContext, streamData.codecParam);
		if(avcodec_open2(streamData.codecContext, streamData.codec, NULL)!=0) throw std::exception("Unable to set codecContext");
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
	if (video_resizeconvert_sws_ctxt) sws_freeContext(video_resizeconvert_sws_ctxt);
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

int64_t VideoFile::GetVideoDuration()
{
	return videoContainer->duration / AV_TIME_BASE;
}

double VideoFile::GetCurrentPTSTIME(int index)
{
	return static_cast<double>(streamArr[index].stream->time_base.num/static_cast<double>(streamArr[index].stream->time_base.den) * static_cast<double>(streamArr[index].currFrame->pts));
}


int VideoFile::GetAudioStreamIndex()
{
	int returnVal{};
	for (const StreamData &streamData : streamArr)
	{
		if (streamData.codecParam->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			return returnVal;
		}
		returnVal++;
	}
	return -1; //No audio stream found.
}
int VideoFile::GetVideoStreamIndex()
{
	int returnVal{};
	for (const StreamData& streamData : streamArr)
	{
		if (streamData.codecParam->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			return returnVal;
		}
		returnVal++;
	}
	return -1; //No video stream found.
}

SDL_Rect VideoFile::GetVideoDimensions()
{
	SDL_Rect returnVal{};
	for (const StreamData& streamData : streamArr)
	{
		switch (streamData.codecParam->codec_type)
		{
		case AVMEDIA_TYPE_VIDEO:
			returnVal.w = streamData.codecParam->width;
			returnVal.h = streamData.codecParam->height;
		}
	}
	return returnVal;
}

void VideoFile::ResizeVideoFrame(AVFrame*& originalFrame, int width, int height)
{
	if (!originalFrame) return;
	//Checks if a new context needs to be allocated.
	static int prevWidth{}, prevHeight{};

	int videoStreamIndex = GetVideoStreamIndex();
	if (videoStreamIndex < 0) //No video stream found
	{
		throw std::exception("No video stream to resize");
	}
	StreamData& videoStreamData = streamArr[videoStreamIndex];

	//Need to reallocate context, either dimensions changed or it's a new video file(i.e. none allocated yet).
	if (prevWidth != width || prevHeight != height || video_resizeconvert_sws_ctxt == nullptr)
	{
		if (video_resizeconvert_sws_ctxt) sws_freeContext(video_resizeconvert_sws_ctxt);
		video_resizeconvert_sws_ctxt = sws_getContext(
			videoStreamData.codecContext->width, videoStreamData.codecContext->height, videoStreamData.codecContext->pix_fmt, 
			width, height, AV_PIX_FMT_YUV420P, //Change to new height and YUV420P format(standardised to follow sdl display)
			SWS_BICUBIC, //better quality than billinear
			NULL,
			NULL,
			NULL);
		prevWidth = width;
		prevHeight = height;
		if (video_resizeconvert_sws_ctxt) std::exception("Unable to get resize sws_context");
	}

	//Allocates a new frame with new format and dimensions.
	//Used as return value.
	AVFrame* tempFrame = av_frame_alloc();
	int num_bytes = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, width, height, 1);
	uint8_t* frame2_buffer = (uint8_t*)av_malloc(num_bytes);
	//"Fills" the temp frame with rest of the required memory(av_frame_alloc only does the basic memory alloc).
	av_image_fill_arrays(tempFrame->data, tempFrame->linesize, (uint8_t*)frame2_buffer, AV_PIX_FMT_YUV420P, width, height, 1);

	//Converts frame into correct format and dimensions, then puts it into tempFrame. 
	sws_scale(video_resizeconvert_sws_ctxt, originalFrame->data, originalFrame->linesize, 0, videoStreamData.codecContext->height, tempFrame->data, tempFrame->linesize);
	
	//Swaps tempframe and current frame.
	//New frame replaces old frame.
	if (originalFrame) av_frame_free(&originalFrame);
	originalFrame = tempFrame;
}