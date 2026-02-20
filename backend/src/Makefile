# Makefile for RTSP Viewer

CXX = g++
CXXFLAGS = -std=c++11 -Wall
OPENCV_FLAGS = `pkg-config --cflags --libs opencv4`

# Fallback to opencv if opencv4 is not found
ifeq ($(shell pkg-config --exists opencv4 && echo yes),)
	OPENCV_FLAGS = `pkg-config --cflags --libs opencv`
endif

TARGET = rtsp_viewer
SOURCE = rtsp_viewer.cpp

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CXX) $(CXXFLAGS) $(SOURCE) -o $(TARGET) $(OPENCV_FLAGS)

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run