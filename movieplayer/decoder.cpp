//
//  video_decoder.cpp
//  curve
//
//  Created by Joshua Fisher on 1/10/13.
//  Copyright (c) 2013 Joshua Fisher. All rights reserved.
//

#include "decoder.h"

namespace jf {
	
	AVPacket PacketQueue::FlushPacket;
	
	PacketQueue::PacketQueue() {
		std::once_flag once;
		std::call_once(once, [&]() {
			av_init_packet(&FlushPacket);
			FlushPacket.data = (uint8_t*)"FLUSH";
		});
	}
	
	bool PacketQueue::isEmpty() {
		return packets.empty();
	}
	
	void PacketQueue::push(AVPacket pkt) {
		av_dup_packet(&pkt);
		packets.push_back(pkt);
	}
	
	int PacketQueue::pop(AVPacket* pkt) {
		if(!packets.empty()) {
			*pkt = packets.front();
			packets.pop_front();
			return 1;
		}
		return 0;
	}
	
	void PacketQueue::flush() {
		for(AVPacket& p : packets) {
			if(!PacketQueue::isFlushPacket(p))
				av_free_packet(&p);
		}
		packets.clear();
		packets.push_back(FlushPacket);
	}

	bool PacketQueue::isFlushPacket(AVPacket pkt) {
		return pkt.data == FlushPacket.data;
	}

	Demuxer* Demuxer::open(const char* path) {
		// make sure everything is ready to go
		static std::once_flag ffmpeg_initialized;
		std::call_once(ffmpeg_initialized, []() {
			avcodec_register_all();
			av_register_all();
			avformat_network_init();
		});
		
		AVFormatContext* format = NULL;
		
		try {
			// open the file
			if(avformat_open_input(&format, path, NULL, NULL) < 0)
				throw -1;
			// read thru the header
			if(avformat_find_stream_info(format, NULL) < 0) {
				throw -1;
			}
			
			// make a demuxer, return it
			Demuxer* de = new Demuxer();
			de->format = format;
			return de;
		}
		catch(...) {
			if(format) {
				// something went wrong, clean up
				avformat_close_input(&format);
				format = NULL;
			}
		}
		
		return NULL;
	}
	
	Demuxer::Demuxer()
	:	format(NULL)
	{}
	
	Demuxer::~Demuxer() {
		if(format) {
			avformat_close_input(&format);
			
			for(auto st : packetQueues)
				delete st.second;
			packetQueues.clear();
		}
	}
	
	AVFormatContext* Demuxer::getFormat() {
		return format;
	}

	AVStream* Demuxer::getStream(int idx) {
		if(idx < 0 || idx >= format->nb_streams)
			return NULL;
		return format->streams[idx];
	}
	
	int Demuxer::getStreamIndex(AVMediaType type) {
		// look up the best-guess stream index for media type
		return av_find_best_stream(format, type, -1, -1, NULL, 0);
	}
	
	PacketQueue* Demuxer::getPacketQueue(int idx) {
		if(idx < 0 || idx >= format->nb_streams)
			return NULL;
		
		// check to see if we already made one
		if(packetQueues.count(idx))
			return packetQueues[idx];
		
		// guess not, get the stream
		AVStream* st = format->streams[idx];
		// and the codec context
		AVCodecContext* ctx = st->codec;
		// find the decoder
		AVCodec* codec = avcodec_find_decoder(ctx->codec_id);
		// initialize the decoder
		if(avcodec_open2(ctx, codec, NULL) < 0)
			return NULL;
		
		// go ahead and allocate some storage
		ctx->opaque = (uint64_t*)av_malloc(sizeof(uint64_t));
		
		// make a new packet queue, store it, return it
		PacketQueue* queue = new PacketQueue;
		packetQueues[idx] = queue;
		return queue;
	}

	void Demuxer::demux(int idx) {
		if(idx < 0 || idx >= format->nb_streams)
			return;
		
		// make sure we event care about this stream
		if(packetQueues.count(idx)) {
			AVPacket packet;
			// pull packets until we find the stream we want
			while(true) {
				// get the next packet
				if(av_read_frame(format, &packet) < 0)
					return;
				
				// check if its a stream we care about
				if(packetQueues.count(packet.stream_index)) {
					// store the packet
					packetQueues[packet.stream_index]->push(packet);
					// and return if it was the one we wanted
					if(packet.stream_index == idx)
						return;
				}
				else {
					// otherwise clean up and move on
					av_free_packet(&packet);
				}
			}
		}
	}
	
	void Demuxer::seekToTime(double time) {
		int idx = getStreamIndex(AVMEDIA_TYPE_VIDEO);
		
		for(auto p : packetQueues)
			p.second->flush();
	}

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

	uint64_t avcontext_getOpaque(AVCodecContext* context) {
		return *(uint64_t*)context->opaque;
	}
	
	void avcontext_setOpaque(AVCodecContext* context, uint64_t i) {
		*(uint64_t*)context->opaque = i;
	}
	
	uint64_t avframe_getOpaque(AVFrame* frame) {
		return *(uint64_t*)frame->opaque;
	}
	
	void avframe_setOpaque(AVFrame* frame, uint64_t i) {
		*(uint64_t*)frame->opaque = i;
	}

	int mp_create_video_buffer(AVCodecContext* c, AVFrame* pic) {
		int ret = avcodec_default_get_buffer(c, pic);
		pic->opaque = av_malloc(sizeof(uint64_t));
		avframe_setOpaque(pic, avcontext_getOpaque(c));
		return ret;
	}
	
	void mp_release_video_buffer(AVCodecContext* c, AVFrame* pic) {
		if(pic) av_freep(&pic->opaque);
		avcodec_default_release_buffer(c, pic);
	}

	VideoDecoder* VideoDecoder::open(Demuxer* de) {
		if(!de)
			return NULL;
		
		int st = de->getStreamIndex(AVMEDIA_TYPE_VIDEO);
		if(st < 0)
			return NULL;
		
		VideoDecoder* dec = new VideoDecoder();
		dec->demuxer = de;
		dec->packets = de->getPacketQueue(st);
		dec->streamIdx = st;
		dec->context = de->getStream(st)->codec;
		dec->frame = avcodec_alloc_frame();
		dec->frameRGB = avcodec_alloc_frame();

		// look at link for why we need to override the frame creation and release functions
		// http://dranger.com/ffmpeg/tutorial05.html
		dec->context->get_buffer = mp_create_video_buffer;
		dec->context->release_buffer = mp_release_video_buffer;
		
		dec->width = dec->context->width;
		dec->height = dec->context->height;
		dec->bytesPerFrame = avpicture_get_size(PIX_FMT_RGB24, dec->width, dec->height);
		
		dec->sws = sws_getCachedContext(dec->sws,
										dec->width,
										dec->height,
										dec->context->pix_fmt,
										dec->width,
										dec->height,
										PIX_FMT_RGB24,
										SWS_BILINEAR,
										NULL,
										NULL,
										NULL);

		return dec;
	}
	
	VideoDecoder::VideoDecoder()
	:	streamIdx(-1)
	,	context(NULL)
	,	frame(NULL)
	,	frameRGB(NULL)
	,	sws(NULL)
	,	width(0)
	,	height(0)
	,	bytesPerFrame(0)
	,	clock(0.0)
	,	frameNumber(0L)
	{}
	
	VideoDecoder::~VideoDecoder() {
		av_free(frame);
		av_free(frameRGB);
		sws_freeContext(sws);
	}

	int VideoDecoder::getWidth() { return width; }
	int VideoDecoder::getHeight() { return height; }
	int VideoDecoder::getBytesPerFrame() { return bytesPerFrame; }

	VideoFrame::Ptr VideoDecoder::previousFrame() {
		seekToFrame(context->frame_number - 1);
		return nextFrame();
	}
	
	VideoFrame::Ptr VideoDecoder::nextFrame() {
		AVPacket packet;
		
		// keep decoding until we have a whole frame
		while(true) {
			if(packets->isEmpty()) {
				// fetch more packets
				demuxer->demux(streamIdx);
			}
			
			if(!packets->pop(&packet))
				// i guess we're out of packets
				break;
			
			if(PacketQueue::isFlushPacket(packet)) {
				avcodec_flush_buffers(context);
				continue;
			}

			// look at link for a discussion of why the pts has to be handled this way
			// http://dranger.com/ffmpeg/tutorial05.html
			avcontext_setOpaque(context, packet.pts);
			
			// decode next packet of video
			int complete = 0;
			avcodec_decode_video2(context, frame, &complete, &packet);
			// free allocated packet resources
			av_free_packet(&packet);
			
			// figure out the actual pts of this frame
			double pts = 0.0;
			if(packet.dts == AV_NOPTS_VALUE && avframe_getOpaque(frame) != AV_NOPTS_VALUE)
				pts = avframe_getOpaque(frame);
			else if(packet.dts != AV_NOPTS_VALUE)
				pts = packet.dts;
			else
				pts = 0;
			
			// put pts into seconds
			frameNumber = pts;
			pts *= av_q2d(context->time_base);
			
			printf("%lld\n", frameNumber);
			
			// we decoded a whole frame
			if(complete) {
				double delay;
				
				if(pts != 0)
					clock = pts;
				else
					pts = clock;
				
				delay = av_q2d(context->time_base);
				delay += frame->repeat_pict * (delay * 0.5);
				clock += delay;
				
				VideoFrame::Ptr rez = VideoFrame::create(pts, width, height, bytesPerFrame, NULL);
				avpicture_fill((AVPicture*)frameRGB, rez->bytes, PIX_FMT_RGB24, width, height);
				sws_scale(sws, (uint8_t const* const*)frame->data, frame->linesize, 0, height, frameRGB->data, frameRGB->linesize);
				return rez;
			}
		}
		
		return VideoFrame::Ptr();
	}
	
	void VideoDecoder::seekToFrame(int frame) {
		demuxer->seekToFrame(frame);
	}
	
	void VideoDecoder::seekToTime(double time) {
	}
	
}