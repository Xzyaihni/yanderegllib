#ifndef YANCONV_H
#define YANCONV_H

#include <vector>
#include <string>

//everything written by 57qr53r3dn4y to reinvent the wheel (and very poorly)

std::vector<std::string> stringSplit(std::string, std::string);

template <typename T>
inline void cAToNumber(char* val, T* returnVal);

template <typename T>
inline std::vector<T> lengthsToPrefix(std::vector<T> lengthsArr, T maxVal, uint8_t* highestLength);

class YandereImage
{
public:
    enum class ResizeType {NearestNeighbor, AreaSample};

    YandereImage();
    YandereImage(std::string imagePath);

    void bppResize(uint8_t desiredBpp, uint8_t extraChannel=255);

    void resize(unsigned desiredWidth, unsigned desiredHeight, ResizeType resizeType);
    void flip();

    void grayscale();

	bool read(std::string loadPath);
    bool saveToFile(std::string savePath);
    
    unsigned getPixelColorPos(unsigned x, unsigned y, uint8_t color);
    uint8_t getPixelColor(unsigned x, unsigned y, uint8_t color);

    static bool canParse(std::string extension);

    unsigned width;
    unsigned height;
    uint8_t bpp;

    std::vector<uint8_t> image;

private:
    bool pngRead(bool checkChecksums = false);
    std::vector<uint8_t> yanDeflate(std::vector<uint8_t>& inputData);

    bool pgmRead();

    void pgmSave(std::string savePath);
    void ppmSave(std::string savePath);
    
    std::vector<uint8_t> yanInflate(std::vector<uint8_t>& inputData);
    std::vector<uint32_t> crcTableGen();
    void pngSave(std::string savePath);

    uint8_t subPositive(int lVar, int rVar);

    std::string _imagePath;
};

#endif
