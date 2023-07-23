TARGET = map_gen
BUILD_DIR = build
HEADERFILES = point.hpp polygon.hpp shapefile.hpp

CXX = g++
CXXFLAGS = -g -Wall -std=c++11 -O3

all: $(BUILD_DIR)/$(TARGET)

$(BUILD_DIR)/$(TARGET): $(TARGET).cpp $(HEADERFILES)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $< -o $(BUILD_DIR)/$(TARGET)

maps : maps/ne_50m_admin_0_countries.shp maps/ne_10m_admin_0_countries.shp

maps/ne_50m_admin_0_countries.shp :
	@echo "Downloading 50m resolutions maps"
	@mkdir -p maps/temp
	@wget -q https://www.naturalearthdata.com/http//www.naturalearthdata.com/download/50m/cultural/ne_50m_admin_0_countries.zip -P maps/temp
	@unzip -qq maps/temp/ne_50m_admin_0_countries.zip -d maps/temp
	@mv maps/temp/ne_50m_admin_0_countries.shp maps/
	@rm -rf maps/temp

maps/ne_10m_admin_0_countries.shp :
	@echo "Downloading 10m resolutions maps"
	@mkdir -p maps/temp
	@wget -q https://www.naturalearthdata.com/http//www.naturalearthdata.com/download/10m/cultural/ne_10m_admin_0_countries.zip -P maps/temp
	@unzip -qq maps/temp/ne_10m_admin_0_countries.zip -d maps/temp
	@mv maps/temp/ne_10m_admin_0_countries.shp maps/
	@rm -rf maps/temp

clean:
	$(RM) $(BUILD_DIR)/$(TARGET)

clean_all:
	$(RM) -r $(BUILD_DIR)
	$(RM) -r maps

.PHONY: clean all maps