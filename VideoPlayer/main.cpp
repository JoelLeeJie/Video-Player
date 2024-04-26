#include <iostream>
#include "Display.hpp"        
#include "ffmpeg_videoFileFunctions.hpp"
#include "types.hpp"

int windowDimensions[2] = { 500, 500 };

int main(int argc, char** argv)
{
    //Has to be done for sdl to be used.
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER | SDL_INIT_EVENTS);
    SDL_Window* mainWindow = SDL_CreateWindow("Joel's Video Player", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, windowDimensions[0], windowDimensions[1], SDL_WINDOW_RESIZABLE | SDL_WINDOW_INPUT_GRABBED);
    

    VideoFile videoFile{ "C:/Users/onwin/source/repos/VideoPlayer/Assets/Demo/Everlasting Flames - Honkai Impact 3rd.mp4" };
    std::string errMsg{};
    if (!videoFile.checkIsValid(errMsg))
    {
        std::cout << errMsg;
    }
    videoFile.PrintDetails(std::cout);

    //TODO: Remove, for testing purposes.
    //Get a frame from a video stream.
    AVFrame* videoFrame;
    int videoStreamIndex{};
    for (StreamData& stream : videoFile.streamArr)
    {
        if (stream.codecParam->codec_type != AVMEDIA_TYPE_VIDEO)
        {
            videoStreamIndex++;
            continue;
        }
        //Do until an actual frame is available. May cause an infinite loop but just for testing.
        while ((videoFrame = videoFile.GetFrame(videoStreamIndex)) == nullptr);
        break;
    }
    //One frame from the video is achieved.

    return 0;
}