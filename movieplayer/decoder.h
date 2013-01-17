//
//  MovieDecoder.h
//  curve
//
//  Created by Joshua Fisher on 1/10/13.
//  Copyright (c) 2013 Joshua Fisher. All rights reserved.
//

#pragma once

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

#include <thread>
#include <mutex>
#include <memory>
#include <string>
#include <vector>
#include <list>

#include "render.h"

namespace jf {
	
	// single frame of RGB video
	struct VideoFrame {
		typedef std::shared_ptr<VideoFrame> Ptr;
		
		int width;
		int height;
		int numBytes;
		uint8_t* bytes;
		
		double outTime;
		
		~VideoFrame();
		static Ptr create(double o, int w, int h, int sz, uint8_t* ptr);
		
	private:
		VideoFrame();
		VideoFrame(const VideoFrame&) =delete;
		VideoFrame& operator=(const VideoFrame&) =delete;
	};
	
	class Demuxer {
	public:
		AVFormatContext* format;
		
		static Demuxer* open(const char*);
		~Demuxer();
		
		void seek(double time);
		
	private:
		Demuxer();
		Demuxer(const Demuxer&) =delete;
		Demuxer& operator=(const Demuxer&) =delete;
	};
	
	class VideoDecoder {
	public:
		Demuxer* demuxer;
		
		int streamIdx;
		AVStream* stream;
		AVCodecContext* context;
		AVFrame *frame, *frameRGB;
		SwsContext* sws;
		
		static VideoDecoder* open(Demuxer*);
		~VideoDecoder();
		
	private:
		VideoDecoder();
	};

}