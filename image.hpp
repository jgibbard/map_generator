#pragma once

#include <fstream>
#include <vector>
#include <array>
#include <cstring>  // memcpy
#include <cmath>    // pow

template <class T, size_t BITS_PER_PIXEL>
class Image {
private:
    uint32_t width;
    uint32_t height;
    std::vector<T> image_data;
    std::vector<uint32_t> colour_table;

public:
    Image(unsigned int x_size, unsigned int y_size) : 
        width(x_size),
        height(y_size),
        image_data(x_size * y_size) {
        
        if (BITS_PER_PIXEL != 8 && BITS_PER_PIXEL != 16 && BITS_PER_PIXEL != 24 && BITS_PER_PIXEL != 32) {
            throw std::runtime_error("Bits per pixel must be 8, 16, 24, or 32");
        } else if (BITS_PER_PIXEL > (sizeof(T)*8)) {
            throw std::runtime_error("Image pixel type is not big enough to hold " +\
                                     std::to_string(BITS_PER_PIXEL) + " bits per pixel");
        }

        if (BITS_PER_PIXEL == 8) {
            colour_table.resize(std::pow(2,BITS_PER_PIXEL));
        }
    }   

    void set_colour(unsigned int index, uint8_t r, uint8_t g, uint8_t b) {
        if (index >= colour_table.size()) throw std::runtime_error("Colour table index out of range");
        colour_table[index] = ((uint32_t)r)<<16 | ((uint32_t)g)<<8 | (uint8_t)b;
    }

    T get_pixel(unsigned int x, unsigned int y) {
        if (x >= width || y >= height) throw std::runtime_error("Pixel index out of range");
        return image_data[(y * width) + x];
    }
    void set_pixel(unsigned int x, unsigned int y, T val) {
        if (x >= width || y >= height) throw std::runtime_error("Pixel index out of range");
        image_data[(y * width) + x] = val;
    }

    void set_background(T val) {
        std::fill(image_data.begin(), image_data.end(), val);
    }

    uint32_t get_height() {return height;}
    uint32_t get_width() {return width;}

    void save_bitmap_image(const std::string& filename) {
        // Initialise default headers
        std::array<uint8_t, 14> bmp_header = { \
            0x42, 0x4D,                 // File ID: "BM"
            0x00, 0x00, 0x00, 0x00,     // Size of BMP file
            0x00, 0x00,                 // Unused
            0x00, 0x00,                 // Unused
            0x00, 0x00, 0x00, 0x00};    // Offset where pixel array starts
        std::array<uint8_t, 40> dib_header = { \
            0x28, 0x00, 0x00, 0x00,     // Size of DIB header
            0x00, 0x00, 0x00, 0x00,     // Width of bitmap in pixels
            0x00, 0x00, 0x00, 0x00,     // Height of bitmap in pixels
            0x01, 0x00,                 // Number of colour planes
            0x00, 0x00,                 // Number of bits per pixel
            0x00, 0x00, 0x00, 0x00,     // BI_RGB (no compression)
            0x00, 0x00, 0x00, 0x00,     // Size of raw bitmap data (inc padding)
            0x13, 0x0B, 0x00, 0x00,     // Horizontal resolution in pixels per meter (72dpi)
            0x13, 0x0B, 0x00, 0x00,     // Vertical resolution in pixels per meter (72dpi)
            0x00, 0x00, 0x00, 0x00,     // Number of colours in colour table
            0x00, 0x00, 0x00, 0x00};    // Number of important colours

        // Configure BMP header
        // Get BMP size
        const uint32_t size_of_row = (BITS_PER_PIXEL * width) / 8;
        // All rows must be padded to be a multiple of 4 bytes long
        const uint32_t padding_bytes = (size_of_row % 4) == 0 ? 0 : (4 - (size_of_row % 4));
        const uint32_t size_of_row_with_padding = size_of_row + padding_bytes;
        const uint32_t size_of_pixel_array = size_of_row_with_padding * height;
        // Pixel array starts after BMP header, DIB header, and colour table (when present)
        uint32_t pixel_array_offset = bmp_header.size() + dib_header.size();
        pixel_array_offset += (colour_table.size() * sizeof(uint32_t));
        uint32_t size_of_bmp = size_of_pixel_array + pixel_array_offset;
        // Set size of BMP file
        std::memcpy(bmp_header.data()+2, &size_of_bmp, sizeof(size_of_bmp));
        // Set pixel array offset        
        std::memcpy(bmp_header.data()+10, &pixel_array_offset, sizeof(pixel_array_offset));

        // Configure DIB header
        // Set width
        std::memcpy(dib_header.data()+4, &width, sizeof(width));
        // Set height
        std::memcpy(dib_header.data()+8, &height, sizeof(height));
        // Set number of bits per pixel
        uint16_t num_bits_per_pixel = BITS_PER_PIXEL;
        std::memcpy(dib_header.data()+14, &num_bits_per_pixel, sizeof(num_bits_per_pixel));
        // Set size of raw bitmap data (inc padding)
        std::memcpy(dib_header.data()+20, &size_of_pixel_array, sizeof(size_of_pixel_array));
        // Set number of colours in colour table
        uint16_t colour_table_size = colour_table.size();
        std::memcpy(dib_header.data()+32, &colour_table_size, sizeof(colour_table_size));
        
        std::vector<uint8_t> padded_image_data(size_of_pixel_array);
        if (padding_bytes == 0 && BITS_PER_PIXEL == (sizeof(T)*8)) {  // No padding and datatype matches width
            std::memcpy(padded_image_data.data(), image_data.data(), image_data.size()*sizeof(T));
        } else if (BITS_PER_PIXEL == (sizeof(T)*8)) {   // Padding required, but datatype matches width
            unsigned int image_data_index = 0;
            unsigned int padded_image_data_index = 0;
            while (padded_image_data_index < size_of_pixel_array) {
                std::memcpy(padded_image_data.data() + padded_image_data_index,
                            image_data.data() + image_data_index,
                            size_of_row);
                image_data_index += size_of_row;
                padded_image_data_index += size_of_row_with_padding;
            }
        } else {
            throw std::runtime_error("Not currently supported!");
        }

        std::ofstream bmp_file(filename, std::ios::binary);
        if (!bmp_file.good()) {
            throw std::runtime_error("Failed to open file: \"" + filename + "\"");
        }

        bmp_file.write(reinterpret_cast<char*>(bmp_header.data()), bmp_header.size());
        bmp_file.write(reinterpret_cast<char*>(dib_header.data()), dib_header.size());
        bmp_file.write(reinterpret_cast<char*>(colour_table.data()), colour_table.size()*(sizeof(uint32_t)));
        bmp_file.write(reinterpret_cast<char*>(padded_image_data.data()), padded_image_data.size());
        bmp_file.close();
    }

};