#include <iostream>
#include <SDL.h>        
#include "ffmpeg_videoFileFunctions.hpp"
#include "types.hpp"
int main(int argc, char** argv)
{
    VideoFile videoFile{ "C:/Users/onwin/source/repos/VideoPlayer/Assets/Demo/Everlasting Flames - Honkai Impact 3rd.mp4" };
    std::string errMsg{};
    if (!videoFile.checkIsValid(errMsg))
    {
        std::cout << errMsg;
    }
    videoFile.PrintDetails(std::cout);

    return 0;
}