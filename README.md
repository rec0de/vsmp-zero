# vsmp-zero
A very slow movie player designed for the Raspberry Pi Zero

## Context

This is my take on the concept of a very slow movie player, as previously done by [Tom Whitwell](https://debugger.medium.com/how-to-build-a-very-slow-movie-player-in-2020-c5745052e4e4)
and, originally, [Bryan Boyer](https://medium.com/s/story/very-slow-movie-player-499f76c48b62).  
As opposed to their implementations, this one is written in C to be close to the hardware, using libav and the [IT8951 C library](https://github.com/waveshare/IT8951) by waveshare to display frames from a video file on an ePaper display.

The code is designed for and tested with the [1872x1404 E-Ink panel](https://www.waveshare.com/product/raspberry-pi/displays/e-paper/7.8inch-e-paper-hat.htm) from waveshare. Other panels using the same controller should work out of the box, for other panels you might have to customize the `processFrame` method.

## Pre-processing

vsmp-zero does not perform scaling, grayscale or framerate conversion of the video at runtime to save on computation and implementation complexity. You should pre-process your input accordingly on a faster machine using ffmpeg.

*details / examples to come*

## Dependencies

This repo comes with a modified version of the IT8951 library, so all you need is the [bcm2835](http://www.airspayce.com/mikem/bcm2835/) library as well as libavformat, libavcodec and libavutil.  
To use hardware acceleration for video decoding, you'll have to use custom-built ffmpeg libraries (see [https://maniaclander.blogspot.com/2017/08/ffmpeg-with-pi-hardware-acceleration.html](https://maniaclander.blogspot.com/2017/08/ffmpeg-with-pi-hardware-acceleration.html)).
