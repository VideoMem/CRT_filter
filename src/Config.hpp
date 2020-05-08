//
// Created by sebastian on 18/2/20.
//

#ifndef SDL_CRT_FILTER_CONFIG_HPP
#define SDL_CRT_FILTER_CONFIG_HPP
#include <ResourceRoller.hpp>

class Config {
public:
    static constexpr auto sampleBitmapsFolder = "resources/images";
    static const nint samples = 6;
    static std::string magick_default_format() { return "BMP"; };
    const std::string sampleBitmapsNames[6] = {
            "testCardRGB.bmp",
            "RCA_Indian_Head_Test_Pattern.bmp"
  //          "Mantis.png",
    //        "standby.png",
      //      "marcosvtar.bmp",
        //    "alf.bmp"
    };

    static const int NKERNEL_WIDTH  = 640;
    static const int NKERNEL_HEIGHT = 480;
    static const int SCREEN_WIDTH  = 800 * 4/3;
    static const int SCREEN_HEIGHT = 600 * 4/3;
    static const int TARGET_WIDTH  = 1024 * 4/3;
    static const int TARGET_HEIGHT = 768  * 4/3;

    nint initResources(ResourceRoller& channels) {
        const std::string path  = sampleBitmapsFolder;
        const std::string type = "image";
        auto str = sampleBitmapsNames;
        nint count = samples;
        for(nint i = 0; i < count; ++i)
            channels.Add(str[i],  path + "/" + str[i], type);
        return count;
    }


};

#endif //SDL_CRT_FILTER_CONFIG_HPP
