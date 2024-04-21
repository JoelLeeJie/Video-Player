#ifndef FFMPEG_VIDEOFILEFUNCTIONS_HPP
#define FFMPEG_VIDEOFILEFUNCTIONS_HPP
#include "types.hpp"
#include <string>
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

private:
	AVFormatContext* videoContainer = nullptr;
	AVStream** videoStreamArray = nullptr; //points to the videoContainer's variable, so no need dealloc.

};

AVFormatContext* GetAVFormat(const std::string &fileName);
#endif