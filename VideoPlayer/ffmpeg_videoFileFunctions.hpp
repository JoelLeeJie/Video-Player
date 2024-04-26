#ifndef FFMPEG_VIDEOFILEFUNCTIONS_HPP
#define FFMPEG_VIDEOFILEFUNCTIONS_HPP
#include "types.hpp"
#include <string>
#include <vector>
struct StreamData; //forward declaration
class VideoFile
{
public:
	VideoFile(const std::string& fileName);
	~VideoFile();

	//Copy ctor/operator is deleted for now, as unable to control underlying details about ffmpeg.
	//AVFormatContext for example, even if new space is allocated for it and copied from donor, when donor's dtor is called, 
	//it may result in the variables(like ptrs) within AVFormatContext to be deallocated, so the host's version may be destroyed as well.
	//Not sure, but try not to anyways.
	VideoFile(const VideoFile& donor) = delete;
	VideoFile& operator=(const VideoFile& rhs) = delete;

	void PrintDetails(std::ostream& output);
	bool checkIsValid(std::string& outputMessage);
	AVFrame* GetFrame(int index); //Gets frame for that specific stream in the streamArr.
	int64_t GetVideoDuration();
	double GetCurrentPTSTIME(int index);
	
	int GetAudioStreamIndex();
	int GetVideoStreamIndex();
	SDL_Rect GetVideoDimensions();

private:
	AVFormatContext* videoContainer = nullptr;
	std::vector<StreamData> streamArr{}; //Need to dealloc codecContext.
};

//Relevant attributes for each individual stream. 
struct StreamData
{
	AVStream* stream; //Audio/Video stream to read from. Pointer to stream in videoContainer (AVFormatContext).
	AVCodec* codec; 
	AVCodecParameters* codecParam; //Details of codec
	AVCodecContext* codecContext; //Used to decode compressed packets. Need to alloc/dealloc memory for this variable.
	//temporary variables used to store data//
	AVPacket* currPacket; //Need to alloc.
	AVFrame* currFrame; //Need to alloc.
};

AVFormatContext* GetAVFormat(const std::string &fileName);
#endif