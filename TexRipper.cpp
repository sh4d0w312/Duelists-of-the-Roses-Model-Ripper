#include "TexRipper.h"
#include<iostream>
#include<fstream>
#include<string>

//texture primary header struct
struct primary_header {

    //must equal 0x00324852
    unsigned int identifier;

    int blank0;

    //length of entire texture block
    int texture_size;

    int blank1;

    //relative offset to secondary header
    int secondary_header_offset;

    //length of secondary header
    int secondary_header_size;

    //relative offset to palette
    int palette_offset;

    //length of palette
    int palette_size;

    //relative offset to pixel data
    int pixel_offset;

    //length of pixel data
    int pixel_size;

    //relative offset to end of texture
    int end_offset;

    int blank2;
    int unk0;
    int unk1;
    int unk2;
    int blank3;
    int unk3;
    int blank4;
    int unk4;
    int blank5;
    int unk5;

    //width of texture in pixels
    short int tex_width;

    //height of texture in pixels;
    short int tex_height;

    //number of subtextures, including palette
    int n_subtextures;

    int blank6;
};

//repeated block format used by secondary header
struct secondary_header_block {

    int unk0;

    //offset to subtexture from primary header
    int subtex_offset;

    int unk1;
    int unk2;
    int unk3;
    int unk4;
    int unk5;
    int blank0;
    int blank1;
    int unk6;
    int unk7;
    int blank3;
};

//template class for palette assignment
template <class T>
void get_palette(char *buffer, int offset, T *palette) {

    T *palette_data = reinterpret_cast<T *>(buffer + offset + 96);

    //assign values from data to correct location in palette array
    for (int i = 0; i < 256; i++) {

        if (i%32 < 8) {

            palette[i] = palette_data[i];
        
        } else if (i%32 < 16) {

            palette[i] = palette_data[i+8];
        
        } else if (i%32 < 24) {

            palette[i] = palette_data[i-8];

        } else {

            palette[i] = palette_data[i];
        }
    }
}

//define static variables

//false if pixel_map has not been initialised
bool TexRipper::map_init = false;

//maps pixels from location in data to location in image
int TexRipper::pixel_map[8192];

int TexRipper::rip(char *buffer, int offset, std::string out_path) {

    //check if pixel_map has been initialised
    //if not, call generate_map()
    if (!map_init) {

        generate_map();
    }

    //header containing texture info
    primary_header header_1 = reinterpret_cast<primary_header *>(buffer + offset)[0];

    //check if identifier matches
    //return -1 if not
    if (header_1.identifier != 0x00324852) {

        return -1;
    }

    //variable size array of 48 byte blocks in header
    //contains offsets to palette and each subtexture
    secondary_header_block *header_2 = new secondary_header_block[header_1.n_subtextures];

    //obtain header blocks from buffer
    for (int i = 0; i < header_1.n_subtextures; i++) {

        header_2[i] = reinterpret_cast<secondary_header_block *>(buffer + offset + header_1.secondary_header_offset)[i];
    }

    //palette array
    unsigned short int palette[256];

    //palette array if 32 bit colours are used
    unsigned int palette_32[256];

    unsigned int curr_colour;

    //true if 32 bit palette is used
    bool rgba32_palette = false;

    //check if 32 bit palette is used
    if (reinterpret_cast<unsigned short int *>(buffer + offset + header_2[0].subtex_offset)[0] != 0x0025) {

        rgba32_palette = true;

        //obtain 32 bit palette
        get_palette(buffer, offset + header_2[0].subtex_offset, palette_32);
    
    } else {

        //obtain 16 bit palette
        get_palette(buffer, offset + header_2[0].subtex_offset, palette);
    }

    //convert 16 bit palette to 32 bit palette
    if (!rgba32_palette) {

        for (int i = 0; i < 256; i++) {

            //red
            curr_colour = palette[i]&0x001F;
            palette_32[i] = ((curr_colour << 3) | (curr_colour >> 2)) << 16;

            //green
            curr_colour = (palette[i]&0x03E0) >> 5;
            palette_32[i] += ((curr_colour << 3) | (curr_colour >> 2)) << 8;

            //blue
            curr_colour = (palette[i]&0x7C00) >> 10;
            palette_32[i] += (curr_colour << 3) | (curr_colour >> 2);

            //alpha
            if (palette[i]&0x8000) {

                palette_32[i] += 0x80000000;
            }
        }
    
    //swap red and blue
    } else {

        for (int i = 0; i < 256; i++) {

            palette_32[i] = (palette_32[i]&0xFF00FF00) + ((palette_32[i]&0xFF0000) >> 16) + ((palette_32[i]&0xFF) << 16);
        }
    }

    rgba32_palette = true;

    //array of subtexture arrays
    //all subtextures are 128x64 at most
    unsigned short int (*subtextures)[8192] = new unsigned short int[header_1.n_subtextures - 1][8192];

    //array of subtexture arrays when using 32 bit palette
    unsigned int (*subtextures_32)[8192] = new unsigned int[header_1.n_subtextures - 1][8192];

    //pointer to pixel data in buffer
    unsigned char *pixel_data;

    //true if subtextures have non-standard (128x64) dimensions
    bool special_subtex = false;

    //size of subtex in pixels along x-direction
    int subtex_x = 128;

    //size of subtex in pixels along y-direction
    int subtex_y = 64;

    //first two bytes of subtexture header correspond to 0x0205 only on standard size textures
    if (reinterpret_cast<unsigned short int *>(buffer + offset + header_2[1].subtex_offset)[0] != 517) {

        //update subtex x and y to correct values
        subtex_x = reinterpret_cast<int *>(buffer + offset + header_2[1].subtex_offset + 48)[0];
        subtex_y = reinterpret_cast<int *>(buffer + offset + header_2[1].subtex_offset + 52)[0];

        special_subtex = true;
    }

    //obtain subtextures from buffer
    for (int i = 0; i < header_1.n_subtextures-1; i++) {

        pixel_data = (unsigned char *)(buffer + offset + header_2[i+1].subtex_offset + 96);

        for (int j = 0; j < 8192; j++) {

            //subtextures with standard dimensions are jumbled, and the pixel map must be used to decode them
            if (!special_subtex) {

                subtextures_32[i][pixel_map[j]] = palette_32[pixel_data[j]];
            
            //subtextures with nonstandard dimensions are stored in the correct order
            } else {

                subtextures_32[i][j] = palette_32[pixel_data[j]];
            }
        }
    }

    //number of subtexture columns in combined texture
    int tex_cols = header_1.tex_width/subtex_x;

    //combined texture array
    unsigned short int *combined_tex = new unsigned short int[header_1.tex_height*header_1.tex_width];

    //combined texture array if using 32 bit palette
    unsigned int *combined_tex_32 = new unsigned int[header_1.tex_height*header_1.tex_width];

    //combine subtextures
    //pixels are added upside down as .bmp stores images upside down
    for (int i = 0; i < header_1.tex_height; i++) {

        for (int j = 0; j < header_1.tex_width; j++) {

            combined_tex_32[i*header_1.tex_width + j] = subtextures_32[tex_cols*(i/subtex_y) + j/subtex_x][subtex_x*(i%subtex_y) + (j%subtex_x)];
        }
    }

    //bmp file
    std::ofstream OutFile(out_path, std::ios::binary|std::ios::trunc);

    //char array for bmp header
    char bmp_header[0x8A] = {0};

    //identifier
    bmp_header[0] = 0x42;
    bmp_header[1] = 0x4D;

    //file size
    int bmp_size;

    bmp_size = 0x8A + 4*header_1.tex_width*header_1.tex_height;

    bmp_header[2] = bmp_size & 0xFF;
    bmp_header[3] = (bmp_size >> 8) & 0xFF;
    bmp_header[4] = (bmp_size >> 16) & 0xFF;
    bmp_header[5] = (bmp_size >> 24) & 0xFF;

    //pixel data offset
    bmp_header[10] = 0x8A;

    //header size
    bmp_header[14] = 0x7C;

    //image width
    bmp_header[18] = header_1.tex_width & 0xFF;
    bmp_header[19] = (header_1.tex_width >> 8) & 0xFF;

    //image height
    bmp_header[22] = header_1.tex_height & 0xFF;
    bmp_header[23] = (header_1.tex_height >> 8) & 0xFF;

    //planes
    bmp_header[26] = 0x01;

    bmp_header[28] = 0x20;

    //compression
    bmp_header[30] = 0x03;

    //mask
    bmp_header[56] = 0xFF;
    bmp_header[59] = 0xFF;
    bmp_header[62] = 0xFF;
    bmp_header[69] = 0xFF;

    //colour space type
    bmp_header[70] = 0x42;
    bmp_header[71] = 0x47;
    bmp_header[72] = 0x52;
    bmp_header[73] = 0x73;

    //intent
    bmp_header[122] = 0x02;

    //write header
    OutFile.write(bmp_header, 0x8A);

    //write pixel data
    OutFile.write(reinterpret_cast<char *>(combined_tex_32), 4*header_1.tex_width*header_1.tex_height);

    //free resources
    OutFile.close();
    delete[] header_2;
    delete[] subtextures;
    delete[] subtextures_32;
    delete[] combined_tex;
    delete[] combined_tex_32;

    //return offset to end of texture block
    return header_1.texture_size;
}

void TexRipper::generate_map() {

    //offset of top-leftmost pixel in current 32 byte block on the final image
    int base = 0;

    //counter tracks current byte in block
    int counter = 0;

    //odd bytes are placed 2 rows below even bytes
    int skip_line = 0;

    //offset in x direction relative to location in memory
    int offset = 12;

    //some pixels have an extra offset of -8
    int bonus_offset = 0;

    for (int i = 0; i < 8192; i++) {

        //set to 256 on odd bytes
        skip_line = 256*(counter%2);

        //update offset
        offset += 3;

        //decrease offset every 4 pixels
        if (counter%4 == 0) {

            offset -= 15;
        }

        //check for bonus offset
        if ((counter > 16) && (base%1024 < 512)) {

            bonus_offset = -8*(counter%2);
        
        }

        if ((!((counter < 16) && (counter%2 == 0))) && (base%1024 >= 512)) {
            
            bonus_offset = -8;
        }

        //calculate pixel location
        pixel_map[i] = base + counter + skip_line + offset + bonus_offset;

        //update counter
        counter += 1;

        //reset bonus offset
        bonus_offset = 0;

        //end of 32 byte block re-initialisation
        if (counter == 32) {

            //reset variables
            counter = 0;
            offset = 12;
            bonus_offset = 0;

            //increase base offset
            base += 16;

            //skip 2 lines after completing 2 lines
            if (base%256 == 0) {

                base += 256;
            }

            //adjust offset on alternating line skips
            if (base%1024 >= 512) {

                offset = 16;
            }
        }
    }
}