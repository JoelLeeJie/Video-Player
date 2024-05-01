#ifndef FFMPEG_VIDEOFILEFUNCTIONS_HPP
#define FFMPEG_VIDEOFILEFUNCTIONS_HPP
#include "types.hpp"
#include <string>
#include <vector>
//Relevant attributes for each individual stream. 
struct StreamData
{
	AVStream* stream{}; //Audio/Video/other stream to read from. Pointer to the stream in videoContainer (AVFormatContext).
	AVCodec* codec{};
	AVCodecParameters* codecParam{}; //Details of codec
	AVCodecContext* codecContext{}; //Used to decode compressed packets. Need to alloc/dealloc memory for this variable.
	//temporary variables used to store data//
	AVPacket* currPacket{}; //Need to alloc.
	AVFrame* currFrame{}; //Need to alloc.
};

struct VideoFileError
{
	bool canFind = true;
	bool canRead = true;
	bool canCodec = true;
	bool reachedEOF = false;
	bool resizeError = false;
	std::string message{};
};

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

	//Returns pointer to video file's error code if there's an error, else returns nullptr.
	const VideoFileError *checkIsValid();
	
	//Call once errors are handled, to reset errors.
	void ResetErrorCodes();

	/*
	Gets an AvFrame for that particular stream.
	Returns null if none read, check error codes for info.
	*/
	AVFrame** GetFrame(int index);
	int64_t GetVideoDuration();

	/*
	* Gets the current frame's ptsTime for that stream. Use for synchronisation.
	*/
	double GetCurrentPTSTIME(int index);
	
	/*Returns -1 if none found*/
	int GetAudioStreamIndex();

	/*Returns -1 if none found*/
	int GetVideoStreamIndex();

	SDL_Rect GetVideoDimensions();

	/*
	Will destroy the old frame, leaving the new frame in its place.
	*/
	void ResizeVideoFrame(AVFrame*& originalFrame, int width, int height);
	
private:
	AVFormatContext* videoContainer = nullptr;
	std::vector<StreamData> streamArr{}; //Need to dealloc codecContext.
	
	//Used to resize and convert video using sws_scale.
	//Allocated and deallocated when used.
	struct SwsContext* video_resizeconvert_sws_ctxt = nullptr; 

	//Error code. Used instead of std::exceptions(which can crash the program if not caught).
	VideoFileError errorCodes{};
};



//Returns nullptr if unable to open video file.
AVFormatContext* GetAVFormat(const std::string &fileName);
#endif