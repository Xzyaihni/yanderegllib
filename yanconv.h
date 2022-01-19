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
    enum class ResizeType {nearest_neighbor, area_sample};

    YandereImage();
    YandereImage(std::string imagePath);

    void bpp_resize(uint8_t desiredBpp, uint8_t extraChannel=255);

    void resize(unsigned desiredWidth, unsigned desiredHeight, ResizeType resizeType);
    void flip();

    void grayscale();

	bool read(std::string loadPath);
    bool save_to_file(std::string savePath);
    
    unsigned pixel_color_pos(unsigned x, unsigned y, uint8_t color);
    uint8_t pixel_color(unsigned x, unsigned y, uint8_t color);

    static bool can_parse(std::string extension);

    unsigned width;
    unsigned height;
    uint8_t bpp;

    std::vector<uint8_t> image;

private:
    bool png_read(bool checkChecksums = false);
    std::vector<uint8_t> yan_deflate(std::vector<uint8_t>& inputData);

    bool pgm_read();

    void pgm_save(std::string savePath);
    void ppm_save(std::string savePath);
    
    std::vector<uint8_t> yan_inflate(std::vector<uint8_t>& inputData);
    std::vector<uint32_t> crc_table_gen();
    void png_save(std::string savePath);

    uint8_t sub_positive(int lVar, int rVar);

    std::string _imagePath;
};

#endif
