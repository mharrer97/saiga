#pragma once


#include "saiga/opengl/opengl.h"


#include <stdint.h>
#include <vector>

class fipImage;

namespace PNG{
class Image;
}

class SAIGA_GLOBAL Image{
public:

    Image();
    ~Image();
	
    typedef unsigned char byte_t;

    //raw image data
    std::vector<byte_t> data;

    //image dimensions
    int size = 0; //size of data in bytes
    int width = 0;
    int height = 0;

    int bitDepth;
    int channels;
    bool srgb = false;
    bool shouldDelete = false;
    //Alignment of the beginning of each row. Allowed values: 1,2,4,8
    int rowAlignment = 4;
    int bytesPerRow;
public:
    byte_t* getRawData();

    int bytesPerChannel();
    int bytesPerPixel();
    int bitsPerPixel();
    size_t getSize();
    void setPixel(int x, int y, void* data);
    void setPixel(int x, int y, uint8_t data);
    void setPixel(int x, int y, uint16_t data);
    void setPixel(int x, int y, uint32_t data);

    void setPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b);

    int position(int x, int y);
    uint8_t* positionPtr(int x, int y);

    template<typename T>
    T getPixel(int x, int y){
        return *(T*)positionPtr(x,y);
    }


    void makeZero(); //zeros out all bytes
    void create(byte_t* initialData=nullptr);//allocates memory

    void resize(int w, int h);
    void setSubImage(int x, int y, Image &src);
    void setSubImage(int x, int y , int w , int h , uint8_t* data);
    void getSubImage(int x, int y, int w, int h, Image &out);

    //adds a zero initialized channel
    void addChannel();

    void flipRB();

    //======================================================

    int getBitDepth() const;
    void setBitDepth(int value);
    int getChannels() const;
    void setChannels(int value);



};
