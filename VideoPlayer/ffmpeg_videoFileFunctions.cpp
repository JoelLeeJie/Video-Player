#include "ffmpeg_videoFileFunctions.hpp"
#include <iostream>
#include <fstream>
#include <string>

//Returns nullptr if unable to open video file.
AVFormatContext* GetAVFormat(const std::string &fileName)
{
	AVFormatContext* returnVal = avformat_alloc_context();
	int err;
	//Unable to open videofile, so set to null.
	if ((err = avformat_open_input(&returnVal, fileName.c_str(), NULL, NULL)) != 0)
	{
		avformat_close_input(&returnVal); //will also free the context.
		returnVal = nullptr; 
	}
	return returnVal;
}

VideoFile::VideoFile(const std::string &fileName)
{
	//1. Point to the video file
	videoContainer = GetAVFormat(fileName);
	if (!videoContainer)
	{
		//Don't try to init an empty container, just set error and return.
		errorCodes.message += "Invalid FilePath. Unable to open video file\n";
		errorCodes.canFind = false;
		return;
	}

	//2. Open video/audio streams. 
	if (avformat_find_stream_info(videoContainer, NULL) < 0)
	{
		//Don't init if cannot find streams to play.
		errorCodes.message += "Unable to read video/audio av streams\n";
		errorCodes.canRead = false;
		return;
	}

	//3. Get the codec data/context for each stream.
	//Iterate over each stream in the array and fill in streamData for streamArr.
	for (unsigned int i = 0; i < videoContainer->nb_streams; i++)
	{
		streamArr.push_back({});
		StreamData& streamData = streamArr.back();
		streamData.stream = videoContainer->streams[i];
		//For each stream, find the codec param to find the codec id, and use it to find the codec, and use that to find codec context.
		streamData.codecParam = videoContainer->streams[i]->codecpar;
		streamData.codec = avcodec_find_decoder(videoContainer->streams[i]->codecpar->codec_id);
		//Need to alloc memory for codecContext.
		streamData.codecContext = avcodec_alloc_context3(streamData.codec);
		//Assigning data to context.
		avcodec_parameters_to_context(streamData.codecContext, streamData.codecParam);
		if (avcodec_open2(streamData.codecContext, streamData.codec, NULL) != 0)
		{
			errorCodes.message += "Unable to open codec context for stream\n";
			errorCodes.canCodec = false;
		}
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
		if (streamData.currPacket)
		{
			//Dereference buffer.
			av_packet_unref(streamData.currPacket);
			av_packet_free(&streamData.currPacket);
		}
		if (streamData.currFrame)
		{
			//Dereference buffer.
			av_frame_unref(streamData.currFrame);
			av_freep(&streamData.currFrame->data[0]);
		}
	}
	if (video_resizeconvert_sws_ctxt) sws_freeContext(video_resizeconvert_sws_ctxt);
	if (videoContainer) avformat_close_input(&videoContainer);
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

const VideoFileError *VideoFile::checkIsValid()
{
	//There's an error.
	if (!errorCodes.canFind || !errorCodes.canRead || !errorCodes.canCodec || errorCodes.reachedEOF || errorCodes.resizeError)
	{
		return &errorCodes;
	}
	//No error
	return nullptr;
}


void VideoFile::ResetErrorCodes()
{
	errorCodes = VideoFileError{};
}

//Returns nullptr if no frame can be read, check error codes.
AVFrame** VideoFile::GetFrame(int index)
{
	StreamData& stream = streamArr[index];
	int errVal{};
	while ((errVal = avcodec_receive_frame(stream.codecContext, stream.currFrame)) != 0)
	{
		//Not successful,try to resolve.
		switch (errVal)
		{
		//Reached end of file, need to indicate.
		case AVERROR_EOF:
			errorCodes.reachedEOF = true;
			errorCodes.message += std::to_string(index) += " stream has reached EOF\n";
			return nullptr;
		//Send a new packet, since incomplete frame.
		case AVERROR(EAGAIN): 
			//Read a packet and send it.
			if (av_read_frame(videoContainer, stream.currPacket) < 0)
			{
				//error reading packet. Not sure if issue will persist, so don't set error.
				return nullptr;
			}
			if ((errVal = avcodec_send_packet(stream.codecContext, stream.currPacket)) == 0 || errVal == AVERROR(EAGAIN))   //TODO: If exception "Access write violation" thrown here, it means the issue is with resizeVideo function when freeing originalFrame.
			{
					continue; //try to read a frame again.
			}
			else if (errVal == AVERROR_EOF)
			{
				//Should be caught before, but jic just set.
				errorCodes.reachedEOF = true;
				errorCodes.message += std::to_string(index) += " stream has reached EOF\n";
				return nullptr;
			}
			else if (errVal == AVERROR_INVALIDDATA)
			{
				errorCodes.canRead = errorCodes.canCodec = false;
				errorCodes.message += "Invalid Data reading frame from ";
				errorCodes.message += std::to_string(index) += " stream\n";
				return nullptr;
			}
			//unable to send packet, and no frames can be read either, unable to resolve.
			//Read error.
			errorCodes.canRead = errorCodes.canCodec = false;
			errorCodes.message += "Unknown error reading frame from ";
			errorCodes.message += std::to_string(index) += " stream\n";
			return nullptr;
		//Unable to resolve.
		//Read error.
		default:
			errorCodes.canRead = errorCodes.canCodec = false;
			errorCodes.message += "Unknown error reading frame from ";
			errorCodes.message += std::to_string(index) += " stream\n";
			return nullptr;
		}
	}
	//read successful.
	return &stream.currFrame;
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

/*
	1. Allocates an sws_context(ffmpeg) if none found/a new one is needed.
	2. Allocates a temp frame and fills it with required memory.
	3. 
*/
void VideoFile::ResizeVideoFrame(AVFrame*& originalFrame, int width, int height)
{
	if (!originalFrame) return;
	//Store original presentation time, so that it can be changed when reassigning frames.
	int64_t originalPts = originalFrame->pts;
	int64_t originalDts = originalFrame->pkt_dts;
	//Checks if a new context needs to be allocated.
	static int prevWidth{}, prevHeight{};

	int videoStreamIndex = GetVideoStreamIndex();
	if (videoStreamIndex < 0) //No video stream found
	{
		errorCodes.resizeError = true;
		errorCodes.message += "No video stream found\n";
		return;
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
		if (!video_resizeconvert_sws_ctxt)
		{
			errorCodes.resizeError = true;
			errorCodes.message += "Unable to set sws_context for resizing+converting video\n";
			return;
		}
	}

	//Allocates a new frame with new format and dimensions.
	//Used as return value.
	AVFrame* tempFrame = av_frame_alloc();
	if (!tempFrame)
	{
		errorCodes.resizeError = true;
		errorCodes.message += "Unable to allocate temporary frame for resize\n";
		return;
	}
	int num_bytes = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, width, height, 1);
	uint8_t* frame2_buffer = (uint8_t*)av_malloc(num_bytes);
	//"Fills" the temp frame with rest of the required memory(av_frame_alloc only does the basic memory alloc).
	if (num_bytes < 0 || !frame2_buffer || av_image_fill_arrays(tempFrame->data, tempFrame->linesize, (uint8_t*)frame2_buffer, AV_PIX_FMT_YUV420P, width, height, 1) < 0)
	{
		//Error occured. Note that the rest won't run if prior conditions fulfilled, due to short-circuiting.
		errorCodes.resizeError = true;
		errorCodes.message += "Error allocating frame\n";
		av_frame_unref(tempFrame);
		av_freep(&tempFrame->data[0]);
		return;
	}



	//Converts frame into correct format and dimensions, then puts it into tempFrame. 
	sws_scale(video_resizeconvert_sws_ctxt, originalFrame->data, originalFrame->linesize, 0, originalFrame->height, tempFrame->data, tempFrame->linesize);

	//Throw away the old frame and get the new frame.
	//Dereference buffers before freeing, necessary otherwise there'll be heap corruption.
	av_frame_unref(originalFrame);
	av_freep(&originalFrame->data[0]); //Commenting both out will run the video player no issues.

	originalFrame = tempFrame;
	originalFrame->pts = originalPts; //So that the new frame will have the old frame's time.
	originalFrame->pkt_dts = originalDts;

}
