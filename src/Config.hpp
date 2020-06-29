//
// Created by sebastian on 18/2/20.
//

#ifndef SDL_CRT_FILTER_CONFIG_HPP
#define SDL_CRT_FILTER_CONFIG_HPP
#include <ResourceRoller.hpp>

class Config {
public:
    static constexpr auto sampleBitmapsFolder = "resources/images";
    static std::string magick_default_format() { return "BMP"; };

    static const int NKERNEL_WIDTH  = 320;
    static const int NKERNEL_HEIGHT = 240;
    static const int VIDEOFRAME_WIDTH  = 640;
    static const int VIDEOFRAME_HEIGHT = 480;
    static const int SCREEN_WIDTH  = 640;
    static const int SCREEN_HEIGHT = 480;
    static const int TARGET_WIDTH  = 640;
    static const int TARGET_HEIGHT = 480;

    static const nint samples = 6;
    const std::string sampleBitmapsNames[6] = {
            "testCardRGB.bmp",
            "RCA_Indian_Head_Test_Pattern.bmp"
            //          "Mantis.png",
            //        "standby.png",
            //      "marcosvtar.bmp",
            //    "alf.bmp"
    };

    nint initResources(ResourceRoller& channels) {

        const std::string path  = sampleBitmapsFolder;
        const std::string type = "image";
        auto str = sampleBitmapsNames;
        nint count = samples;
        for(nint i = 0; i < count; ++i)
            channels.Add(str[i],  path + "/" + str[i], type);
        return count;
    }

    struct transport {
        static const std::string int_frame_puller;
        static const std::string int_frame_pusher;
        static const std::string grc_sink;
        static const std::string grc_source;
    };

};

const std::string Config::transport::int_frame_puller = "tcp://0.0.0.0:5133";
const std::string Config::transport::int_frame_pusher = "tcp://localhost:5133";
const std::string Config::transport::grc_sink = "tcp://0.0.0.0:5555";
const std::string Config::transport::grc_source = "tcp://localhost:5656";

#endif //SDL_CRT_FILTER_CONFIG_HPP
