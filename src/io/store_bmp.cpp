// Adapted from https://github.com/sol-prog/cpp-bmp-images/blob/master/BMP.h (GNU General Public License v3.0)

#include <fstream>
#include <cmath>
#include <memory>
#include "store_bmp.hpp"

#pragma pack(push, 1)
struct BMPFileHeader {
    uint16_t file_type{ 0x4D42 };          // File type always BM which is 0x4D42 (stored as hex uint16_t in little endian)
    uint32_t file_size{ 0 };               // Size of the file (in bytes)
    uint16_t reserved1{ 0 };               // Reserved, always 0
    uint16_t reserved2{ 0 };               // Reserved, always 0
    uint32_t offset_data{ 0 };             // Start position of pixel data (bytes from the beginning of the file)
};

struct BMPInfoHeader {
    uint32_t size{ 0 };                    // Size of this header (in bytes)
    int32_t width{ 0 };                    // width of bitmap in pixels
    int32_t height{ 0 };                   // width of bitmap in pixels
                                           //       (if positive, bottom-up, with origin in lower left corner)
                                           //       (if negative, top-down, with origin in upper left corner)
    uint16_t planes{ 1 };                  // No. of planes for the target device, this is always 1
    uint16_t bit_count{ 0 };               // No. of bits per pixel
    uint32_t compression{ 0 };             // 0 or 3 - uncompressed. THIS PROGRAM CONSIDERS ONLY UNCOMPRESSED BMP images
    uint32_t size_image{ 0 };              // 0 - for uncompressed images
    int32_t x_pixels_per_meter{ 0 };
    int32_t y_pixels_per_meter{ 0 };
    uint32_t colors_used{ 0 };             // No. color indexes in the color table. Use 0 for the max number of colors allowed by bit_count
    uint32_t colors_important{ 0 };        // No. of colors used for displaying the bitmap. If 0 all colors are required
};
#pragma pack(pop)

bool StoreBMP::Store(const char *fileName, const std::shared_ptr<Viewport>& vp) {

    if (vp->width % 4 != 0)
    {
        throw std::runtime_error("Storing images with width % 4 != 0 is not supported.");
    }

    std::ofstream of(fileName);
    if (of)
    {
        BMPFileHeader file_header;
        BMPInfoHeader bmp_info_header;

        bmp_info_header.size = sizeof(BMPInfoHeader);
        bmp_info_header.width = vp->width;
        bmp_info_header.height = vp->height;
        bmp_info_header.bit_count = 32;

        assert(bmp_info_header.width * bmp_info_header.height * bmp_info_header.bit_count % 8 == 0);

        file_header.offset_data = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader);
        file_header.file_size = file_header.offset_data + (bmp_info_header.width * bmp_info_header.height * bmp_info_header.bit_count / 8);

        of.write((const char *) &file_header, sizeof(file_header));
        of.write((const char *) &bmp_info_header, sizeof(bmp_info_header));

        for (int i = 0; i < vp->width * vp->height * 4; ++i)
        {
            uint8_t value = static_cast<uint8_t>(roundf(vp->pixels[i] * 255.0f));
            of << value;
        }

    }
    else
    {
        throw std::runtime_error("Unable to open the output image file.");
    }

    return true;
}
