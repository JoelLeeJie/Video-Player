/*
	File Name: ffmpeg_videoFileFunctions.cpp

	Brief: Defines various types and functions used to interact with the video through ffmpeg.
*/

#include "ffmpeg_videoFileFunctions.hpp"
#include <iostream>
#include <fstream>
#include <string>


PacketData::PacketData()
{
	packet = av_packet_alloc();
	//TODO: remove
	//this->packet = nullptr;
}

PacketData::~PacketData()
{
	if (packet)
	{
		AVPacket* packetData = packet;
		//Dereference buffer.
		av_packet_unref(packetData);
		av_packet_free(&packetData);
		packet = nullptr;
	}
}


AVPacket** VideoFile::GetPacket(CodecType codecType, bool isClearPackets)
{

	//TODO: using a list may be more efficient when removing front element.
	//Update packet queue and remove packets which were read by all codecs.

	std::list<PacketData>::iterator iter;

	//Only set to true, when avcodec_receive_frame has been done. 
	//This'll prevent packets from being cleared prematurely(even if all codecs have read it) if it's still being used by avcodec_receive_frame.
	if (isClearPackets)
	{
		//Emergency clear if packet list gets too big, 1000 is an arbitrary large number.
		if (packetArr.size() >= 500)
		{
			int counter = 0;
			//Clear the first 500 of the list.
			for (iter = packetArr.begin(); iter != packetArr.end() && counter < 250; counter++)
			{
				//Erase the current element and move to the next.
				iter = packetArr.erase(iter);
			}
		}
		bool breakLoop = false;
		for (iter = packetArr.begin(); iter != packetArr.end();) //Note the lack of iter++, as the loop won't increment until a packet is removed.
		{
			//Loop through codecReadArr and check if any codec hasn't read it yet.
			for (int i = 0; i < static_cast<int>(CodecType::END); i++)
			{
				//If this packet hasn't been read by a codec, then don't remove it.
				if (iter->codecReadArr[i] == false)
				{
					breakLoop = true;
					break;
				}
			}
			if (breakLoop) break; //This packet(and thus all following packets) has not been read by at least a codec.
			//Packet has been read by all codecs, so remove it.
			iter = packetArr.erase(iter);
		}
		return nullptr; //Not trying to get a packet.
	}
	//Check if any packet in the queue hasn't been read by the codec yet.
	for (iter = packetArr.begin(); iter != packetArr.end(); iter++)
	{
		//Check if this packet has been read by this type of codec.
		if (iter->codecReadArr[static_cast<int>(codecType)]) continue;
		iter->codecReadArr[static_cast<int>(codecType)] = true;
		return &iter->packet;
	}
	//Add new packet here, as all packets in queue have already been read by this codec.
	packetArr.emplace_back(); //don't use push_back, as it creates a temp copy that'll call the destructor pre-maturely.
	//Storing data in the newly added packet.

	//TODO: remove
	//TODO: packet may sometimes be 0xcdcdcdcd instead of nullptr, so check for that too.
	if (packetArr.back().packet == nullptr)
	{
		//Unable to allocate packet.
		packetArr.pop_back(); //destroy the newly created packet data.
		return nullptr;
	}
	if (av_read_frame(videoContainer, packetArr.back().packet) < 0)
	{
		//Unknown error.
		packetArr.pop_back(); //destroy the newly created packet data.
		return nullptr;
	}
	packetArr.back().codecReadArr[static_cast<int>(codecType)] = true;
	//If any stream is invalid, then no need to check if that codec read the packet.
	if (audioStreamIndex == -1) packetArr.back().codecReadArr[static_cast<int>(CodecType::AUDIOCODEC)] = true;
	if (videoStreamIndex == -1) packetArr.back().codecReadArr[static_cast<int>(CodecType::VIDEOCODEC)] = true;
	return &packetArr.back().packet;
}

//Returns nullptr if unable to open video file.
AVFormatContext* GetAVFormat(const std::string& fileName)
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

VideoFile::VideoFile(const std::string& fileName)
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
		if (avcodec_parameters_to_context(streamData.codecContext, streamData.codecParam) < 0)
		{
			errorCodes.message += "Unable to open codec context for stream\n";
			errorCodes.canCodec = false;
		}
		if (avcodec_open2(streamData.codecContext, streamData.codec, NULL) != 0)
		{
			errorCodes.message += "Unable to open codec context for stream\n";
			errorCodes.canCodec = false;
		}
		//Just alloc memory for this, no need to update.
		streamData.currFrame = av_frame_alloc();
	}

	int index = 0;
	for (const StreamData& streamData : streamArr)
	{
		if (streamData.codecParam->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			audioStreamIndex = index;
		}
		if (streamData.codecParam->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			videoStreamIndex = index;
		}
		index++;
	}
}

VideoFile::~VideoFile()
{
	for (StreamData& streamData : streamArr)
	{
		//Only dealloc these items, the rest is done in free_context.
		if (streamData.codecContext) avcodec_free_context(&streamData.codecContext);
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
		<< "Duration: " << videoContainer->duration / AV_TIME_BASE << " secs\n"
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

const VideoFileError* VideoFile::checkIsValid()
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
AVFrame** VideoFile::GetFrame(CodecType codecType)
{
	int index = 0;
	AVPacket** pPacket = nullptr;
	switch (codecType)
	{
	case CodecType::AUDIOCODEC:
		index = audioStreamIndex;
		break;
	case CodecType::VIDEOCODEC:
		index = videoStreamIndex;
		break;
	}
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
			pPacket = GetPacket(codecType);
			//Unknown error when getting packet, so return but don't set error codes.
			if (!pPacket) return nullptr;
			if ((errVal = avcodec_send_packet(stream.codecContext, *pPacket)) == 0 || errVal == AVERROR(EAGAIN))   //TODO: If exception "Access write violation" thrown here, it means the issue is with resizeVideo function when freeing originalFrame.
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
	//Clear all used packets(that have been read by all codecs) after avcodec_receive_frame is finished.
	//Does not get a packet.
	GetPacket(codecType, true);
	return &stream.currFrame;
}

int64_t VideoFile::GetVideoDuration()
{
	return videoContainer->duration / AV_TIME_BASE;
}

double VideoFile::GetCurrentPTSTIME(CodecType codecType)
{
	int index = 0;
	switch (codecType)
	{
	case CodecType::AUDIOCODEC:
		index = audioStreamIndex;
		break;
	case CodecType::VIDEOCODEC:
		index = videoStreamIndex;
		break;
	}
	return static_cast<double>(streamArr[index].stream->time_base.num / static_cast<double>(streamArr[index].stream->time_base.den) * static_cast<double>(streamArr[index].currFrame->pts));
}


int VideoFile::GetAudioStreamIndex()
{
	return audioStreamIndex;
}
int VideoFile::GetVideoStreamIndex()
{
	return videoStreamIndex;
}

SDL_Rect VideoFile::GetVideoDimensions() const
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
		void* buffer_ptr = &originalFrame->data[0];
		av_frame_unref(tempFrame);
		//if(buffer_ptr) av_free(buffer_ptr);
		av_frame_free(&tempFrame);
		return;
	}



	//Converts frame into correct format and dimensions, then puts it into tempFrame. 
	sws_scale(video_resizeconvert_sws_ctxt, originalFrame->data, originalFrame->linesize, 0, originalFrame->height, tempFrame->data, tempFrame->linesize);

	//Throw away the old frame and get the new frame.
	//Dereference buffers before freeing, necessary otherwise there'll be heap corruption.
	void* buffer_ptr = &originalFrame->data[0];
	av_frame_unref(originalFrame);
	//if(buffer_ptr) av_free(buffer_ptr); //Commenting both out will run the video player no issues.
	av_frame_free(&originalFrame);

	//Saves the buffer that the previous frame was using(i.e. originalFrame's buffer) and frees it here.
	static void* prev_buffer = nullptr;
	if (prev_buffer) av_freep(&prev_buffer);
	prev_buffer = frame2_buffer;

	originalFrame = tempFrame;
	originalFrame->pts = originalPts; //So that the new frame will have the old frame's time.
	originalFrame->pkt_dts = originalDts;

}

StreamData VideoFile::GetStreamData(int stream_index)
{
	if (stream_index == -1)
	{
		std::cout << "Accessing non-existent stream, VideoFile::GetStreamData()\n";
		return StreamData{};
	}
	return streamArr[stream_index];
}

