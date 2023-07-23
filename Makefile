TARGET = map_gen
BUILD_DIR = build
HEADERFILES = point.hpp polygon.hpp shapefile.hpp

CXX = g++
CXXFLAGS = -g -Wall -std=c++11 -O3

all: $(BUILD_DIR)/$(TARGET)

$(BUILD_DIR)/$(TARGET): $(TARGET).cpp $(HEADERFILES)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $< -o $(BUILD_DIR)/$(TARGET)

clean:
	$(RM) $(BUILD_DIR)/$(TARGET)

.PHONY: clean all