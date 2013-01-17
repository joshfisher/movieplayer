//
//  video_decoder.cpp
//  curve
//
//  Created by Joshua Fisher on 1/10/13.
//  Copyright (c) 2013 Joshua Fisher. All rights reserved.
//

#include "decoder.h"

int mp_create_video_buffer(AVCodecContext* c, AVFrame* pic);
void mp_release_video_buffer(AVCodecContext* c, AVFrame* pic);

namespace jf {
	
	std::once_flag ffmpeg_initialized;
	
	VideoFrame::VideoFrame()
	:	width(0)
	,	height(0)
	,	numBytes(0)
	,	bytes(NULL)
	{}

	VideoFrame::~VideoFrame() {
		if(bytes)
			delete [] bytes;
	}
	
	VideoFrame::Ptr VideoFrame::create(double o, int w, int h, int sz, uint8_t* ptr) {
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

	int mp_create_video_buffer(AVCodecContext* c, AVFrame* pic) {
		int ret = avcodec_default_get_buffer(c, pic);
		uint64_t* pts = (uint64_t*)av_malloc(sizeof(uint64_t));
		*pts = *(uint64_t*)c->opaque;
		pic->opaque = pts;
		return ret;
	}
	
	void mp_release_video_buffer(AVCodecContext* c, AVFrame* pic) {
		if(pic) av_freep(&pic->opaque);
		avcodec_default_release_buffer(c, pic);
	}

	Demuxer* Demuxer::open(const char* path) {
		std::call_once(ffmpeg_initialized, []() {
			avcodec_register_all();
			av_register_all();
			avformat_network_init();
		});
		
		AVFormatContext* format = NULL;
		
		try {
			if(avformat_open_input(&format, path, NULL, NULL) < 0)
				throw -1;
			if(avformat_find_stream_info(format, NULL) < 0) {
				throw -1;
			}
			
			Demuxer* de = new Demuxer();
			de->format = format;
			return de;
		}
		catch(...) {
			if(format)
				avformat_close_input(&format);
		}
		
		return NULL;
	}
	
	Demuxer::Demuxer()
	:	format(NULL)
	{}
	
	Demuxer::~Demuxer() {
		if(format) {
			avformat_close_input(&format);
			format = NULL;
		}
	}
	
	VideoDecoder* VideoDecoder::open(Demuxer* de) {
		AVCodec* codec = NULL;
		int streamIdx = av_find_best_stream(de->format, AVMEDIA_TYPE_VIDEO, -1, -1, &codec, 0);
		if(streamIdx < 0 || !codec)
			return NULL;
		
		AVStream* stream = de->format->streams[streamIdx];
		AVCodecContext* context = stream->codec;
		if(avcodec_open2(context, codec, NULL) < 0)
			return NULL;
		
		VideoDecoder* dec = new VideoDecoder();
		dec->
	}
	
	VideoDecoder::VideoDecoder()
	:	demuxer(NULL)
	,	streamIdx(-1)
	,	stream(NULL)
	,	context(NULL)
	,	frame(NULL)
	,	frameRGB(NULL)
	,	sws(NULL)
	{}
	
	VideoDecoder::~VideoDecoder() {
		if(context) {
			if(context->opaque)
				av_free(context->opaque);
			avcodec_close(context);
		}
	}

	bool Decoder::openVideo() {
		AVCodec* codec = NULL;
		int stream = av_find_best_stream(format, AVMEDIA_TYPE_VIDEO, -1, -1, &codec, 0);
		if(stream < 0 || !codec) {
			return false;
		}
		
		video.streamIdx = stream;
		video.stream = format->streams[video.streamIdx];
		video.context = video.stream->codec;
		if(avcodec_open2(video.context, codec, NULL) < 0)
			return false;
		
		video.width = video.context->width;
		video.height = video.context->height;
		video.bytesPerFrame = avpicture_get_size(PIX_FMT_RGB24, video.width, video.height);
		
		video.context->opaque = (uint64_t*)av_malloc(sizeof(uint64_t));
		video.context->get_buffer = mp_create_video_buffer;
		video.context->release_buffer = mp_release_video_buffer;
		
		video.frame = avcodec_alloc_frame();
		video.frameRGB = avcodec_alloc_frame();
		video.sws = sws_getCachedContext(video.sws,
										 video.width,
										 video.height,
										 video.context->pix_fmt,
										 video.width,
										 video.height,
										 PIX_FMT_RGB24,
										 SWS_BILINEAR,
										 NULL,
										 NULL,
										 NULL);
		video.clock = 0.0;

		return true;
	}
	
	void Decoder::closeVideo() {
		if(video.context) {
			av_free(video.context->opaque);
			avcodec_close(video.context);
			
			av_free(video.frame);
			av_free(video.frameRGB);
			sws_freeContext(video.sws);
		}

		video.stream = NULL;
		video.context = NULL;
		video.frame = NULL;
		video.frameRGB = NULL;
		video.sws = NULL;
		video.width = 0;
		video.height = 0;
		video.bytesPerFrame = 0;
		video.streamIdx = -1;
	}
	
	Decoder::Decoder()
	:	format(NULL)
	,	shutdown(false)
	{

		video.context = NULL;
		video.streamIdx = -1;
		video.bytesPerFrame = 0;
		video.width = 0;
		video.height = 0;
	}
	
	void Decoder::close() {
		closeVideo();
	}
	
	bool Decoder::isOpen() const {
		return (hasAudio() || hasVideo());
	}
	
	bool Decoder::hasVideo() const {
		return format && video.streamIdx >= 0 && video.context != NULL;
	}
	
	int Decoder::getVideoWidth() const {
		return video.width;
	}
	
	int Decoder::getVideoHeight() const {
		return video.height;
	}
	
	int Decoder::getBytesPerVideoFrame() const {
		return video.bytesPerFrame;
	}

	void Decoder::seek(double ms) {
		if(isOpen()) {
			uint64_t num = video.stream->time_base.den;
			uint64_t den = video.stream->time_base.num;
			uint64_t pts = av_rescale(ms, num, den);
			avformat_seek_file(format, video.streamIdx, INT64_MIN, pts, pts, 0);
			avcodec_flush_buffers(video.context);
		}
	}
	
	void Decoder::stepForward() {
		
	}
	
	void Decoder::stepBackward() {
		
	}

	VideoFrame::Ptr Decoder::grabVideoForTime(double time) {
		seek(time);

		while(true) {
			av_read_frame(format, &video.packet);
			if(video.packet.stream_index == video.streamIdx)
				break;
		}
			
		int complete = 0;
		while(!complete)
			avcodec_decode_video2(video.context, video.frame, &complete, &video.packet);

		VideoFrame::Ptr rez = VideoFrame::create(time, video.width, video.height, video.bytesPerFrame, NULL);
		avpicture_fill((AVPicture*)video.frameRGB, rez->bytes, PIX_FMT_RGB24, video.width, video.height);
		sws_scale(video.sws, (uint8_t const* const*)video.frame->data, video.frame->linesize, 0, video.height, video.frameRGB->data, video.frameRGB->linesize);
		return rez;
	}

	VideoFrame::Ptr Decoder::previousVideoFrame() {
		
	}

	VideoFrame::Ptr Decoder::nextVideoFrame() {
		VideoFrame::Ptr frame = frameQueue.front();
		if(frame)
			return frame;
		
		return VideoFrame::Ptr();
	}
	
	void Decoder::demux() {
		if(av_read_frame(format, &packet) < 0) {
			break;
		}
		
		if(packet.stream_index == video.streamIdx)
			videoQueue.push(&packet);
		else if(packet.stream_index == audio.streamIdx)
			audioQueue.push(&packet);
		else
			av_free_packet(&packet);
	}
	
	void Decoder::decodeVideo() {
		while(!shutdown) {
			if(frameQueue.count > 15) {
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
				continue;
			}
			
			// i'm guessing we're done
			if(videoQueue.pop(&video.packet) == 0) {
				break;
			}
			
			*(uint64_t*)video.context->opaque = video.packet.pts;
			
			int complete = 0;
			avcodec_decode_video2(video.context, video.frame, &complete, &video.packet);
			
			double pts = 0.0;
			if(video.packet.dts == AV_NOPTS_VALUE && video.frame->opaque && *(uint64_t*)video.frame->opaque != AV_NOPTS_VALUE)
				pts = *(uint64_t*)video.frame->opaque;
			else if(video.packet.dts != AV_NOPTS_VALUE)
				pts = video.packet.dts;
			else
				pts = 0;
			
			pts *= av_q2d(video.stream->time_base);
			
			if(complete) {
				double delay;
				
				if(pts != 0)
					video.clock = pts;
				else
					pts = video.clock;
				
				delay = av_q2d(video.context->time_base);
				delay += video.frame->repeat_pict * (delay * 0.5);
				video.clock += delay;
				
				VideoFrame::Ptr rez = VideoFrame::create(pts, video.width, video.height, video.bytesPerFrame, NULL);
				avpicture_fill((AVPicture*)video.frameRGB, rez->bytes, PIX_FMT_RGB24, video.width, video.height);
				sws_scale(video.sws, (uint8_t const* const*)video.frame->data, video.frame->linesize, 0, video.height, video.frameRGB->data, video.frameRGB->linesize);
				
				frameQueue.push(rez);
			}
			
			av_free_packet(&video.packet);
		}
	}
	
	void Decoder::decodeAudio() {
		while(!shutdown) {
			std::this_thread::sleep_for(std::chrono::milliseconds(5));
		}
	}

}

int mp_create_video_buffer(AVCodecContext* c, AVFrame* pic) {
	int ret = avcodec_default_get_buffer(c, pic);
	uint64_t* pts = (uint64_t*)av_malloc(sizeof(uint64_t));
	*pts = *(uint64_t*)c->opaque;
	pic->opaque = pts;
	return ret;
}

void mp_release_video_buffer(AVCodecContext* c, AVFrame* pic) {
	if(pic) av_freep(&pic->opaque);
	avcodec_default_release_buffer(c, pic);
}
