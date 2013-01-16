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
	
	struct VideoFrame {
		typedef std::shared_ptr<VideoFrame> Ptr;
		
		int width;
		int height;
		int numBytes;
		uint8_t* bytes;
		
		double outTime;
		
		~VideoFrame() {
			if(bytes)
				delete [] bytes;
		}
		
		static Ptr create(double o, int w, int h, int sz, uint8_t* ptr) {
			VideoFrame* fr = new VideoFrame;
			fr->outTime = o;
			fr->width = w;
			fr->height = h;
			fr->numBytes = sz;
			fr->bytes = new uint8_t[sz];
			if(ptr) {
				memcpy(fr->bytes, ptr, sz);
			}
			return Ptr(fr);
		}
		
	private:
		VideoFrame()
		:	width(0)
		,	height(0)
		,	numBytes(0)
		,	bytes(NULL)
		{}
		
		VideoFrame(const VideoFrame&) =delete;
		VideoFrame& operator=(const VideoFrame&) =delete;
	};
	
	class Decoder {
	public:
		Decoder();
		~Decoder();
		
		bool open(const char* file);
		void close();
		void start();
		void seek(double time);

		bool isOpen() const;
		bool hasAudio() const;
		bool hasVideo() const;
		
		void videoFrameSize(int* width, int* height, int* numBytes) const;
		VideoFrame::Ptr consumeVideoFrame(double time);
		
	private:
		AVFormatContext* format;
		
		struct {
			AVStream* stream;
			AVCodecContext* context;
			
			int bytesPerFrame;
			int width, height;

			int streamIdx;
			
			double clock;
		} video;
		
		bool openVideo();
		void closeVideo();

		struct {
			AVStream* stream;
			AVCodecContext* context;
			
			int streamIdx;
		} audio;

		bool openAudio();
		void closeAudio();

		struct PacketQueue {
			typedef std::list<AVPacket> PacketList;
			
			int count;
			PacketList packets;
			std::mutex mutex;
			
			PacketQueue();
			~PacketQueue();
			
			void push(AVPacket*);
			int pop(AVPacket*);
			void flush();
		};
		
		struct FrameQueue {
			typedef std::list<VideoFrame::Ptr> FrameList;
			
			int count;
			FrameList frames;
			std::mutex mutex;
			
			FrameQueue();
			void push(VideoFrame::Ptr);
			VideoFrame::Ptr pop();
			VideoFrame::Ptr front();
			void flush();
		};
		
		PacketQueue videoQueue;
		PacketQueue audioQueue;
		FrameQueue frameQueue;
		
		bool shutdown;
		std::thread demuxThread;
		std::thread videoThread;
		std::thread audioThread;
				
		void demux();
		void decodeVideo();
		void decodeAudio();
	};

}