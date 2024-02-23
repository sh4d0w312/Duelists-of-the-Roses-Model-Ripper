#include<string>

#ifndef TEXRIPPER_H
#define TEXRIPPER_H

class TexRipper {

public:

    //rips texture from buffer starting at offset
    //returns offset to end of texture block
    //if buffer does not contain a texture block at offset, then it returns -1 instead
    static int rip(char *buffer, int offset, std::string output_path);

private:

    //generates pixel_map array
    static void generate_map();

    //map from pixel locations in data to pixel locations in image
    static int pixel_map[8192];

    //false if pixel_map has not been initialised
    static bool map_init;
};

#endif