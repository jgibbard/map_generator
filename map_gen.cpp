#include <iostream>
#include <vector>
#include <string>
#include "polygon.hpp"
#include "image.hpp"
#include "shapefile.hpp"

void print_help() {
    std::cerr << "\nUsage: " << std::endl;
    std::cerr << "\t" << "map_gen <path_to_map_shapefile> [image_width] [image_height]" << std::endl;
    std::cerr << "\t" << "map_gen <path_to_map_shapefile> ";
    std::cerr << "x_min x_max y_min y_max [image_width] [image_height]" << std::endl;
}

template<typename T>
T _read_arg_val(char* str) {
    // Only allow T to be int and double types
    // Will trigger a build error if this is not true
    static_assert(std::is_same<T, int>::value || 
                  std::is_same<T, double>::value);
}

template<>
int _read_arg_val<int>(char* str) { return std::stoi(str); }
template<>
double _read_arg_val<double>(char* str) { return std::stod(str); }

template<typename T>
T read_arg(char* str, int min, int max, const std::string& name) {
    T val = 0;
    try {
        val = _read_arg_val<T>(str);
    } catch (std::invalid_argument& error)  {
        std::cerr << "Error: " << error.what() << std::endl;
        print_help();
        exit(1);
    }
    if (val < min || val > max) {
        std::cerr << "Error: " << name << " must be between ";
        std::cerr << min << " and " << max << std::endl;
        print_help();
        exit(1);
    }
    return val;
}

int main(int argc, char **argv) {

    // Image defaults
    const unsigned int width_default = 3600;
    unsigned int width = width_default;
    unsigned int height = width_default / 2.0;
    bool maintain_aspect_ratio = true;
    double x_min = -180.0;
    double x_max = 180.0;
    double y_min = -90.0;
    double y_max = 90.0;

    if (argc == 2) { // No image size specifed
        width = width_default;
    } else if (argc == 3) {  // Just width specified
        width = read_arg<int>(argv[2], 1, 50000, "image_width");
    } else if (argc == 4) {  // Force size. Aspect ratio might be wrong
        width = read_arg<int>(argv[2], 1, 50000, "image_width");
        height = read_arg<int>(argv[3], 1, 50000, "image_height");
        maintain_aspect_ratio = false;
    } else if (argc >= 6 && argc <= 8) {  // x+y min+max specified 
        x_min = read_arg<double>(argv[2], -180.0, 180.0, "x_min");
        x_max = read_arg<double>(argv[3], -180.0, 180.0, "x_max");
        y_min = read_arg<double>(argv[4], -90.0, 90.0, "y_min");
        y_max = read_arg<double>(argv[5], -90.0, 90.0, "y_max");
        if (argc == 6) {  // No image size specifed
            width = width_default;
        } else if (argc == 7) {  // Just width specified
            width = read_arg<int>(argv[6], 1, 50000, "image_width");
        } else {  // Force size. Aspect ratio might be wrong
            width = read_arg<int>(argv[6], 1, 50000, "image_width");
            height = read_arg<int>(argv[7], 1, 50000, "image_height");
            maintain_aspect_ratio = false;
        }
    } else {
        print_help();
        return 1;
    }

    if ((x_min >= x_max) || (y_min >= y_max)) {
        std::cerr << "Error: x/y_min is greater or equal to x/y_max" << std::endl;
        print_help();
        return 1;
    }

    if (maintain_aspect_ratio) {
        height = std::ceil(width * ((y_max - y_min) / (x_max - x_min)));
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

    double x_scale = static_cast<double>(width - 1) / (x_max - x_min);
    double y_scale = static_cast<double>(height - 1) / (y_max - y_min);

    double x_shift = -x_min;
    double y_shift = -y_min;

    // Shift and scale the lat/lng polygons to match the image size
    for (auto& polygon : polygons) {
        polygon.shift(x_shift, y_shift);
        polygon.scale(x_scale, y_scale);
    }

    // Create image, and set up colour table
    Image<uint8_t, 8> image(width, height);
    image.set_colour(0, 0x8A,0xB4,0xF8);    // Blue
    image.set_colour(1, 0x94,0xD2,0xA5);    // Green
    image.set_colour(2, 0x6A,0x72,0x75);    // Grey
    image.set_colour(3, 0x00,0x00,0x00);    // Black
    image.set_colour(4, 0xFF,0xFF,0xFF);    // White

    // Set background to blue
    image.set_background(0);

    // Draw all the country boundaries
    for (auto& polygon : polygons) {
        image.draw_polygon(polygon, true, 1, true, 2);
    }

    // Output bitmap to stdout
    // Allows piping to a tool like imagemagick for resizing
    // or converting to other file formats
    image.write_bitmap_image(std::cout);
   
    return 0;
}
