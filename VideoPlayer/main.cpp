#include <iostream>
#include <SDL.h>        
#include "ffmpeg_videoFileFunctions.hpp"
#include "types.hpp"
int main(int argc, char** argv)
{
    //AVFormatContext* fmt_ctx = NULL;
    //AVDictionaryEntry* tag = NULL;
    //int ret;

    //if (argc != 2) {
    //    printf("usage: %s <input_file>\n"
    //        "example program to demonstrate the use of the libavformat metadata API.\n"
    //        "\n", argv[0]);
    //    return 1;
    //}
    //const char* filename = "C:/Users/onwin/Videos/Captures/CatHero.mp4";
    //if ((ret = avformat_open_input(&fmt_ctx, filename, NULL, NULL)))
    //    return ret;

    //if ((ret = avformat_find_stream_info(fmt_ctx, NULL)) < 0) {
    //    av_log(NULL, AV_LOG_ERROR, "Cannot find stream information\n");
    //    return ret;
    //}

    //while ((tag = av_dict_get(fmt_ctx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX)))
    //    printf("%s=%s\n", tag->key, tag->value);

    //avformat_close_input(&fmt_ctx);
    //system("PAUSE");

    ////return 0;
    bool quit = false;
    SDL_Event event;

    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window* window = SDL_CreateWindow("SDL2 Displaying Image",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, 0);

    while (!quit)
    {
        SDL_WaitEvent(&event);

        switch (event.type)
        {
        case SDL_QUIT:
            quit = true;
            break;
        }
    }

    SDL_Quit();

    return 0;
}