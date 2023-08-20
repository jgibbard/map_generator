#pragma once

#include <fstream>
#include <string>
#include <vector>
#include <cstring>  //memcpy
#include "point.hpp"
#include "polygon.hpp"

class Shapefile {

private:

    // Main header constants
    const unsigned int kFileCode = 9994;
    const unsigned int kFileVersion = 1000;
    const unsigned int kMainHeaderSize = 100;

    const unsigned int kFileCodeOffset = 0;
    const unsigned int kFileLengthOffset = 24;
    const unsigned int kFileVersionOffset = 28;

    // Record header constants
    const unsigned int kRecordHeaderSize = 8;
    const unsigned int kRecordNumberOffset = 0;
    const unsigned int kRecordLengthOffset = 4;
    // All valid record have at the very minimum a
    // shape type field which is 4 bytes
    const unsigned int kMinRecordLength = 4;
    const unsigned int kShapeTypeOffset = 0;

    // Polygon constants
    const unsigned int kPolygonShapeType = 5;
    const unsigned int kPolygonNumPartsOffset = 36;
    const unsigned int kPolygonNumPointsOffset = 40;
    const unsigned int kPolygonPartsOffset = 44;

    std::vector<uint8_t> raw_data;
    std::vector<std::pair<unsigned int,unsigned int>> record_index;
    std::string filename;
    bool good;

    inline unsigned int get_unsigned_int_big_endian(unsigned int index) {
        return (raw_data[index+3]<<0) | (raw_data[index+2]<<8) | (raw_data[index+1]<<16) | ((unsigned)raw_data[index]<<24);
    }

    inline unsigned int get_unsigned_int_little_endian(unsigned int index) {
        return (raw_data[index]<<0) | (raw_data[index+1]<<8) | (raw_data[index+2]<<16) | ((unsigned)raw_data[index+3]<<24);
    }

    bool _load_records() {
        record_index.clear();
        // Check all record headers and create record index
        const unsigned int length = raw_data.size();
        unsigned int index = kMainHeaderSize;
        unsigned int record_num = 1;
        while (index < length) {
            unsigned int start = index + kRecordHeaderSize;
            // Check data is big enough for the record header
            if (length < (index + kRecordHeaderSize)) return false;
            // Check for sequential record numbers
            if (record_num != get_unsigned_int_big_endian(index + kRecordNumberOffset)) return false;
            record_num++;
            // Record length in shape file is number of 16 bits word
            // To x2 to get bytes
            unsigned int record_length = (get_unsigned_int_big_endian(index + kRecordLengthOffset) * 2);
            // Check the record is has enough room for at least the shape type
            if (record_length < kMinRecordLength) return false;
            // Store the start index and length of each record
            record_index.push_back({start, record_length});

            index += (kRecordHeaderSize + record_length);
        }
        // Check last record is the correct length
        if (index != length) return false;
        // Success
        return true;
    }

    bool _is_valid() {
        // Check data is long enough to contain the header
        const unsigned int length = raw_data.size();
        if (length < kMainHeaderSize) return false;
        // Check the file code matches the expected value
        unsigned int file_code = get_unsigned_int_big_endian(kFileCodeOffset);        
        if (file_code != kFileCode) return false;
        // Check the length of the file matches the header
        unsigned int file_length = get_unsigned_int_big_endian(kFileLengthOffset);
        if (file_length != length / 2) return false;
        // Check that the shapefile version number matched the expected value
        unsigned int file_version = get_unsigned_int_little_endian(kFileVersionOffset);
        if (file_version != kFileVersion) return false;
        // Success
        return true;
    }

    bool _is_polygon(std::pair<unsigned int, unsigned int>& record) {
        unsigned int index = record.first;
        return (get_unsigned_int_little_endian(index + kShapeTypeOffset) == kPolygonShapeType);
    }

    std::vector<Polygon> _get_polygons_from_record(std::pair<unsigned int, unsigned int>& record) {
        unsigned int index = record.first;
        // Check that this is a polygon record type
        if (get_unsigned_int_little_endian(index + kShapeTypeOffset) != kPolygonShapeType) {
            throw std::runtime_error("Shape type is not Polygon");
        }
        // Check there is enough data to store the polygon record header
        if (raw_data.size() < (index + kPolygonPartsOffset)) {
            throw std::runtime_error("Polygon is corrupted");
        }

        // Get the number of parts in the polygon as well as the total number of points
        // which are split across all the parts
        const unsigned int number_parts = get_unsigned_int_little_endian(index + kPolygonNumPartsOffset);
        const unsigned int number_points = get_unsigned_int_little_endian(index + kPolygonNumPointsOffset);

        // Polygons must have at least one part, and at least 4 points
        if (number_parts == 0 || number_points < 4) {
            throw std::runtime_error("Polygon is corrupted");
        }
        // Check there is enough data to store all the index of each part
        if (raw_data.size() < (index + kPolygonPartsOffset + (sizeof(uint32_t) * number_parts))) {
            throw std::runtime_error("Polygon is corrupted");
        }

        // Get the starting index of each part and store in a vector
        std::vector<uint32_t> part_indexes;
        part_indexes.resize(number_parts);
        std::memcpy(part_indexes.data(), &raw_data[index + kPolygonPartsOffset], sizeof(uint32_t) * number_parts);
        // Check all indexes are in bounds
        for (unsigned int part_index : part_indexes) {
            if (part_index >= number_points) throw std::runtime_error("Polygon is corrupted");
        }

        // The array of data points starts after the array of part indexes
        const unsigned int kPolygonPointsOffset = kPolygonPartsOffset + (number_parts * sizeof(uint32_t));
        // Check there is enough data in the shapefile to fit all data points
        if (raw_data.size() < (index + kPolygonPointsOffset + (sizeof(uint32_t) * number_points))) {
            throw std::runtime_error("Polygon is corrupted");
        }
        // Read points for all parts into a single vector
        std::vector<Point> points;
        points.resize(number_points);
        std::memcpy(points.data(), &raw_data[index + kPolygonPointsOffset], sizeof(Point) * number_points);

        // Store each part
        std::vector<Polygon> polygons;
        // Worst case size if all parts are outer parts
        polygons.reserve(number_parts);
        // For now, hold the inner parts in a temp vector
        // This is because we need to read in all the outer boundaries
        // first, and then see which of these boundaries contains the inner boundary
        std::vector<std::vector<Point>> inner_parts;

        for (unsigned int index = 0; index < part_indexes.size(); index++) {
            // Get an iterator to the start and end of each part
            auto start_it = points.begin() + part_indexes[index];
            // The end index of the last part is equal to the total number of points
            auto end_it = points.begin() + ((index == (part_indexes.size() - 1)) ? number_points : part_indexes[index+1]);
            // Parts must have at least 4 points
            if ((end_it - start_it) < 4) throw std::runtime_error("Polygon is corrupted");
            // Direction points listed in determines if the part is an outer (boundary) or
            // inner (hole in boundary) polygon part.
            if (Polygon::is_clockwise(start_it, end_it)) {
                polygons.push_back({std::vector<Point>(start_it, end_it),           // Outer
                                    std::vector<std::vector<Point>>{},              // Blank vector for inner
                                    Polygon::get_bounding_box(start_it, end_it)});    // Bounding box of outer
            } else {
                inner_parts.push_back(std::vector<Point>(start_it, end_it));
            }
        }

        // For each inner part, check which outer part(s) it fits within
        for (auto& inner_part : inner_parts) {
            for (auto& outer_part : polygons) {
                for (auto& point : inner_part) {
                    // If at least on point of the inner part is within
                    // the outer part, then add it to the part as 
                    if (Polygon::contains(outer_part.outer, point)) {                        
                        outer_part.inner.push_back(inner_part);
                        break;
                    }
                }
            }
        }
        return polygons;
    }

public:
    Shapefile() : filename(""), good(false) {  }
    Shapefile(const std::string& shapefile_filename) : filename(shapefile_filename), good(false) {  }
    
    void read(const std::string& shapefile_filename) {
        filename  = shapefile_filename;
        read();
    }

    void read() {
        good = false;
        // Check the endian of the processor the code is running on.
        int endian_test = 1;
        if (! *((char *)&endian_test)) {
            throw std::runtime_error("Program will only run on little endian processor");
        }

        std::ifstream map_shapefile(filename, std::ios::binary);
        if (!map_shapefile.good()) {
            throw std::runtime_error("Failed to open file: \"" + filename + "\"");
        }
        // Read entire shapefile    
        raw_data = std::vector<uint8_t>((std::istreambuf_iterator<char>(map_shapefile)),
                                         std::istreambuf_iterator<char>());
        // Check no errors occurred while reading the file
        if (!map_shapefile.good()) {
            throw std::runtime_error("Failed to read: " + filename);
        } else {
            map_shapefile.close();
        }
        // Check file looks like a valid shapefile
        if (!_is_valid()) {
            throw std::runtime_error("File is not a shapefile: " + filename);
        }
        // Check every record and create an index of each record
        if (!_load_records()) {
            throw std::runtime_error("File is not a shapefile: " + filename);
        }

        good = true;
    }

    void get_polygons(std::vector<Polygon>& polygons) {
        polygons.clear();
        if (!good) throw std::runtime_error("Shapefile::read() must called successfully first");

        for (auto& record : record_index) {
            if (_is_polygon(record)) {
                // This should be a relatively efficient way of building the vector
                // of polygons. With C++11 function returns are handles with std::move,
                // and then using make_move_iterator to append to original vector
                std::vector<Polygon> polygons_temp = _get_polygons_from_record(record);
                polygons.insert(polygons.end(),
                                std::make_move_iterator(polygons_temp.begin()), 
                                std::make_move_iterator(polygons_temp.end()));
            }
        }
    }
};
