#pragma once

#include <fstream>
#include <vector>
#include <array>
#include <cstring>  // memcpy
#include <cmath>    // pow
#include "polygon.hpp"

template <class T, size_t BITS_PER_PIXEL>
class Image {
private:
    uint32_t m_width;
    uint32_t m_height;
    uint32_t m_max_x;
    uint32_t m_max_y;
    std::vector<T> m_image_data;
    std::vector<uint32_t> m_colour_table;

public:
    Image(unsigned int x_size, unsigned int y_size) : 
        m_width(x_size),
        m_height(y_size),
        m_max_x(x_size - 1),
        m_max_y(y_size - 1),
        m_image_data(x_size * y_size) {
        
        if (BITS_PER_PIXEL != 8 && BITS_PER_PIXEL != 16 && BITS_PER_PIXEL != 24 && BITS_PER_PIXEL != 32) {
            throw std::runtime_error("Bits per pixel must be 8, 16, 24, or 32");
        } else if (BITS_PER_PIXEL > (sizeof(T)*8)) {
            throw std::runtime_error("Image pixel type is not big enough to hold " +\
                                     std::to_string(BITS_PER_PIXEL) + " bits per pixel");
        }

        if (BITS_PER_PIXEL == 8) {
            m_colour_table.resize(std::pow(2,BITS_PER_PIXEL));
        }
    }   

    void set_colour(unsigned int index, uint8_t r, uint8_t g, uint8_t b) {
        if (index >= m_colour_table.size()) throw std::runtime_error("Colour table index out of range");
        m_colour_table[index] = ((uint32_t)r)<<16 | ((uint32_t)g)<<8 | (uint8_t)b;
    }

    T get_pixel(unsigned int x, unsigned int y) {
        if (x >= m_width || y >= m_height) throw std::runtime_error("Pixel index out of range");
        return m_image_data[(y * m_width) + x];
    }
    void set_pixel(unsigned int x, unsigned int y, T val) {
        if (x >= m_width || y >= m_height) throw std::runtime_error("Pixel index out of range " + std::to_string(x) + "," + std::to_string(y));
        m_image_data[(y * m_width) + x] = val;
    }

    void set_background(T val) {
        std::fill(m_image_data.begin(), m_image_data.end(), val);
    }

    void draw_square(const Point& bl, const Point& tr, T val) {

        for (unsigned int x = static_cast<unsigned int>(std::round(bl.x)); x <= static_cast<unsigned int>(std::round(tr.x)); x++) {
            set_pixel(x, static_cast<unsigned int>(std::round(bl.y)), val);
            set_pixel(x, static_cast<unsigned int>(std::round(tr.y)), val);
        }

        for (unsigned int y = static_cast<unsigned int>(bl.y); y <= static_cast<unsigned int>(tr.y); y++) {
            set_pixel(static_cast<unsigned int>(std::round(bl.x)), y, val);
            set_pixel(static_cast<unsigned int>(std::round(tr.x)), y, val);
        }
    }

    void draw_line(const Point& p0, const Point& p1, T val) {
        // Bresenham's Line Drawing Algorithm

        // Round line start and end points to nearest whole pixel value
        double x1 = std::round(p0.x);
        double y1 = std::round(p0.y); 
        double x2 = std::round(p1.x);
        double y2 = std::round(p1.y);

        // No need to draw the line if it is outside the viewport
        if (std::max(x1, x2) < 0.0 || std::max(y1, y2) < 0.0 || std::min(x1, x2) > m_width || std::min(y1, y2) > m_height) {
            return;
        }
        // For large angles, step through y axis rather than x axis
        const bool large_angle = (std::fabs(y2 - y1) > std::fabs(x2 - x1));
        if (large_angle) {
            std::swap(x1, y1);
            std::swap(x2, y2);
        }
        // Algorithm expects x2 >= x1, so swap them if that isn't true
        if (x1 > x2) {
            std::swap(x1, x2);
            std::swap(y1, y2);
        }
        // Get the delta in the x and y coordinates
        const double dx = x2 - x1;
        const double dy = std::fabs(y2 - y1);
        // Initialise the error to mid way between x1 and x2
        double error = dx / 2.0;
        const int y_step = (y1 < y2) ? 1 : -1;
        int y = static_cast<int>(y1);

        // Step along x axis and inc y axis based when the error term goes below 0
        // Uses _set_pixel to ignore out of bounds points without flagging an error.
        // In many (but not all) cases this will be faster than working out where the 
        // line intersects the edge of the viewport and truncating it
        if (large_angle) {
            for (int x = static_cast<int>(x1); x <= static_cast<int>(x2); x++) {
                _set_pixel(y, x, val);
                error -= dy;
                if (error < 0) {
                    y += y_step;
                    error += dx;
                }
            }
        } else {
            for (int x = static_cast<int>(x1); x <= static_cast<int>(x2); x++) {
                _set_pixel(x, y, val);
                error -= dy;
                if (error < 0) {
                    y += y_step;
                    error += dx;
                }
            }
        }
    }

    void draw_polygon(const Polygon& polygon, bool fill, T fill_colour, bool border, T border_colour) {
        // No need to draw anything if the polygon bounding box 
        // is outside the image area
        if (polygon.max_x() < 0.0 || polygon.max_y() < 0.0 || polygon.min_x() > m_width || polygon.min_y() > m_height) {
            return;
        // Skip drawing anything less than 1 px wide
        } else if (((polygon.max_x() - polygon.min_x()) < 1.0) || ((polygon.max_y() - polygon.min_y()) < 1.0)) {
            return;
        }

        if (fill) {
            _polygon_fill(polygon, fill_colour);
        }
        if (border) {
            _polygon_border(polygon, border_colour);
        }
       
    }

    uint32_t get_height() {return m_height;}
    uint32_t get_width() {return m_width;}

    void save_bitmap_image_to_file(const std::string& filename) {
        std::ofstream bmp_file(filename, std::ios::binary);
        if (!bmp_file.good()) {
            throw std::runtime_error("Failed to open file: \"" + filename + "\"");
        }
        write_bitmap_image(bmp_file);
        bmp_file.close();
    }

    void write_bitmap_image(std::ostream& bmp_file) {
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
        const uint32_t size_of_row = (BITS_PER_PIXEL * m_width) / 8;
        // All rows must be padded to be a multiple of 4 bytes long
        const uint32_t padding_bytes = (size_of_row % 4) == 0 ? 0 : (4 - (size_of_row % 4));
        const uint32_t size_of_row_with_padding = size_of_row + padding_bytes;
        const uint32_t size_of_pixel_array = size_of_row_with_padding * m_height;
        // Pixel array starts after BMP header, DIB header, and colour table (when present)
        uint32_t pixel_array_offset = bmp_header.size() + dib_header.size();
        pixel_array_offset += (m_colour_table.size() * sizeof(uint32_t));
        uint32_t size_of_bmp = size_of_pixel_array + pixel_array_offset;
        // Set size of BMP file
        std::memcpy(bmp_header.data()+2, &size_of_bmp, sizeof(size_of_bmp));
        // Set pixel array offset        
        std::memcpy(bmp_header.data()+10, &pixel_array_offset, sizeof(pixel_array_offset));

        // Configure DIB header
        // Set width
        std::memcpy(dib_header.data()+4, &m_width, sizeof(m_width));
        // Set height
        std::memcpy(dib_header.data()+8, &m_height, sizeof(m_height));
        // Set number of bits per pixel
        uint16_t num_bits_per_pixel = BITS_PER_PIXEL;
        std::memcpy(dib_header.data()+14, &num_bits_per_pixel, sizeof(num_bits_per_pixel));
        // Set size of raw bitmap data (inc padding)
        std::memcpy(dib_header.data()+20, &size_of_pixel_array, sizeof(size_of_pixel_array));
        // Set number of colours in colour table
        uint16_t colour_table_size = m_colour_table.size();
        std::memcpy(dib_header.data()+32, &colour_table_size, sizeof(colour_table_size));
        
        std::vector<uint8_t> padded_image_data(size_of_pixel_array);
        if (padding_bytes == 0 && BITS_PER_PIXEL == (sizeof(T)*8)) {  // No padding and datatype matches width
            std::memcpy(padded_image_data.data(), m_image_data.data(), m_image_data.size()*sizeof(T));
        } else if (BITS_PER_PIXEL == (sizeof(T)*8)) {   // Padding required, but datatype matches width
            unsigned int image_data_index = 0;
            unsigned int padded_image_data_index = 0;
            while (padded_image_data_index < size_of_pixel_array) {
                std::memcpy(padded_image_data.data() + padded_image_data_index,
                            m_image_data.data() + image_data_index,
                            size_of_row);
                image_data_index += size_of_row;
                padded_image_data_index += size_of_row_with_padding;
            }
        } else {
            throw std::runtime_error("Not currently supported!");
        }

        if (!bmp_file.good()) {
            throw std::runtime_error("Error with output file stream");
        }

        bmp_file.write(reinterpret_cast<char*>(bmp_header.data()), bmp_header.size());
        bmp_file.write(reinterpret_cast<char*>(dib_header.data()), dib_header.size());
        bmp_file.write(reinterpret_cast<char*>(m_colour_table.data()), m_colour_table.size()*(sizeof(uint32_t)));
        bmp_file.write(reinterpret_cast<char*>(padded_image_data.data()), padded_image_data.size());
        
    }

private:
    void _set_pixel(unsigned int x, unsigned int y, T val) {
        if (x >= m_width || y >= m_height) return;
        m_image_data[(y * m_width) + x] = val;
    }

    void _polygon_fill(const Polygon& polygon, T val) {
               
        // Only need to fill over the bounding box area that is visible within the viewport
        int x_start = (polygon.min_x() < 0.0) ? 0 : std::floor(polygon.min_x());
        int y_start = (polygon.min_y() < 0.0) ? 0 : std::floor(polygon.min_y());
        int x_stop = (polygon.max_x() > m_max_x) ? m_max_x : std::ceil(polygon.max_x());
        int y_stop = (polygon.max_y() > m_max_y) ? m_max_y : std::ceil(polygon.max_y());

        // Step through each row within the bounding box
        for (int y_index = y_start; y_index <= y_stop; y_index++) {
            std::vector<int> x_crossings;
            _get_x_crossings(polygon.outer, y_index, x_crossings);
            for (const auto& inner :  polygon.inner) {
                _get_x_crossings(inner, y_index, x_crossings);
            }
            // If no crossings on this row, then continue to the next row
            if (x_crossings.size() < 2) continue;            
            // Sort the crossing points from smallest to biggest pixel
            std::sort(x_crossings.begin(), x_crossings.end());
            // Step through each pair or crossings and fill all the pixels in between
            for (unsigned int node_index = 0; node_index < (x_crossings.size()-1); node_index+=2) {
                // If the x crossing coordinate starts outside of the viewport, then stop filling this row.
                // x_crossings is sorted by size, so the rest will be out of the viewport too
                if (x_crossings[node_index] >= x_stop) break;
                // If the x crossing ends before the viewport starts, skip it
                if (x_crossings[node_index + 1] < x_start) continue;
                // Limit fill range to the viewport
                unsigned int x_fill_start = (x_crossings[node_index] < 0) ?
                                            0 : static_cast<unsigned int>(x_crossings[node_index]);
                unsigned int x_fill_stop = (x_crossings[node_index + 1] > static_cast<int>(m_max_x)) ?
                                            m_max_x : static_cast<unsigned int>(x_crossings[node_index + 1]);
                // Fill the pixels between the pair of x coordinates
                for (unsigned int x_index = x_fill_start; x_index <= x_fill_stop; x_index++) {
                    set_pixel(x_index, y_index, val);
                }
            }
        }
    }

    void _polygon_border(const Polygon& polygon, T val) {
        for  (unsigned int node = 0; node < (polygon.outer.size() - 1); node++) {
            draw_line(polygon.outer[node], polygon.outer[node + 1], val);
        }
        for (const auto& inner_poly : polygon.inner) {
            for  (unsigned int node = 0; node < (inner_poly.size() - 1); node++) {
                draw_line(inner_poly[node], inner_poly[node + 1], val);
            }
        }
    }

    void _get_x_crossings(const std::vector<Point>& polygon, unsigned int row_index, std::vector<int>& x_crossings) {

        unsigned int i = 1;
        unsigned int j = 0;
        double y_index_dbl = static_cast<double>(row_index);
        // Step through each adjacent pair of nodes in the polygon
        while(i < polygon.size()) {
            // If one node is above the current row, and one node is below
            // the current row (or one node is directly on it)
            if (((polygon[i].y < y_index_dbl) && (polygon[j].y >= y_index_dbl)) ||
                ((polygon[j].y < y_index_dbl) && (polygon[i].y >= y_index_dbl))) {
                // Interpolate the coordinate of the point that the polygon crosses the x axis on this row
                x_crossings.push_back(
                    round(
                        polygon[i].x + (
                            ((y_index_dbl - polygon[i].y) / (polygon[j].y - polygon[i].y)) * 
                            (polygon[j].x - polygon[i].x)
                )));
            }
            i++;
            j++;
        }
    }
};