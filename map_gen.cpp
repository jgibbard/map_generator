#include <iostream>
#include <vector>
#include "polygon.hpp"
#include "image.hpp"
#include "shapefile.hpp"

int main(int argc, char **argv) {

    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <path_to_map_shapefile>" << std::endl;
        return 1;
    }

    // Open and parse the shapefile
    Shapefile shapefile(argv[1]);
    try {
        shapefile.read();
    } catch (std::runtime_error& error) {
        std::cerr << "Error: " << error.what() << std::endl;
        return 1;
    }

    // Extract all the polygons from the shapefile
    std::vector<Polygon> polygons;
    shapefile.get_polygons(polygons);

    unsigned int width = 3600;
    unsigned int height = width / 2;
    double x_scale = static_cast<double>(width - 1) / 360.0;
    double y_scale = static_cast<double>(height - 1) / 180.0;

    // Shift and scale the lat/lng polygons to match the image size
    for (auto& polygon : polygons) {
        // X shift: -180 to 180 -> 0 to 360
        // Y shift:  -90 to  90 -> 0 to 180
        polygon.shift(180.0, 90.0);
        polygon.scale(x_scale, y_scale);
    }

    // Create image, and set up colour table
    Image<uint8_t, 8> image(width, height);
    image.set_colour(0, 0x30,0x99,0xBF);    // Blue
    image.set_colour(1, 0x61,0xD4,0x95);    // Green
    image.set_colour(2, 0x00,0x00,0x00);    // Black
    image.set_colour(3, 0xFF,0xFF,0xFF);    // White

    // Set background to blue
    image.set_background(0);

    // Draw all the country boundaries
    for (auto& polygon : polygons) {
        image.draw_polygon(polygon, true, 1, true, 2);
    }

    // Store image as bitmap file
    image.save_bitmap_image("map.bmp");
   
    return 0;
}
