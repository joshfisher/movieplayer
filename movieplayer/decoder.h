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
	
	class PacketQueue {
		std::list<AVPacket> packets;
		
	public:
		PacketQueue();
		bool isEmpty();
		void push(AVPacket pkt);
		int pop(AVPacket* pkt);
		void flush();
		
		static AVPacket FlushPacket;
		static bool isFlushPacket(AVPacket pct);
	};
	
	class Demuxer {
	public:
		static Demuxer* open(const char*);
		~Demuxer();
		
		AVFormatContext* getFormat();
		int getStreamIndex(AVMediaType);
		AVStream* getStream(int streamIdx);
		PacketQueue* getPacketQueue(int streamIdx);
		
		// trigger loading packets into all registered queues
		void demux(int streamIndx);

		void seekToTime(double time);
		
	private:
		Demuxer();
		
		AVFormatContext* format;
		std::map<int,PacketQueue*> packetQueues;
	};
	
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

	class VideoDecoder  {
	public:
		static VideoDecoder* open(Demuxer*);
		~VideoDecoder();
		
		int getWidth();
		int getHeight();
		int getBytesPerFrame();
		
		VideoFrame::Ptr previousFrame();
		VideoFrame::Ptr nextFrame();
		
		void seekToTime(double time);
		
	private:
		VideoDecoder();
		
		Demuxer* demuxer;
		PacketQueue* packets;
		
		int streamIdx;
		AVCodecContext* context;
		AVFrame *frame, *frameRGB;
		SwsContext* sws;
		
		double clock;
		uint64_t frameNumber;
		int width, height, bytesPerFrame;
	};

}