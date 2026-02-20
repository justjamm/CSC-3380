# RTSP Stream Viewer

A simple C++ program that captures an RTSP stream and displays it using OpenCV.

## Prerequisites

none really besides docker and docker compose, takes care of the dependencies



## Compilation

docker-compose up --build

## Dev Environment
"reopen in container" in vscode

## Usage
docker compose and then navigate to browser and do localhost:8081. should work on local networks as well, just make sure to configure firewall to allow access.


uhhhhhhhhh when this isn't vibecoded ill have better documentation on usage, just a poc for now and will expand later. 

mediamtx is pretty much only there because Windows blocks accessing devices on the network through docker by regular means. I think using it for now is fine, things can be hardcoded and fixed later. actually running it on the server should pretty much circumvent the issue entirely. 


## Notes

- We are abandoning FFmpeg for Gstreamer for latency reasons. Gstreamer is practically built for this, while ffmpeg is moreso a general tool so the overhead kinda sucks


- More notes in the future obviously