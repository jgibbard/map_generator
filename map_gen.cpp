#include <iostream>
#include <vector>
#include "polygon.hpp"
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

    return 0;
}
