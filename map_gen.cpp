#include <iostream>
#include <vector>
#include "polygon.hpp"
#include "image.hpp"
#include "shapefile.hpp"

int main(int argc, char **argv) {

    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <path_to_map_shape_file>" << std::endl;
        return 1;
    }

    Shapefile shapefile(argv[1]);
    try {
        shapefile.read();
    } catch (std::runtime_error& error) {
        std::cerr << "Error: " << error.what() << std::endl;
        return 1;
    }

    std::vector<Polygon> polygons;
    shapefile.get_polygons(polygons);

    Image<uint8_t, 8> test(23, 3);
    test.set_colour(0, 0x30,0x99,0xBF);
    test.set_colour(1, 0x61,0xD4,0x95);

    test.set_background(0);
    test.set_pixel(10,0,1);
    test.save_bitmap_image("test.bmp");
   
    return 0;
}
