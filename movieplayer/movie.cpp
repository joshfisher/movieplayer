//
//  Movie.cpp
//  curve
//
//  Created by Joshua Fisher on 1/8/13.
//  Copyright (c) 2013 Joshua Fisher. All rights reserved.
//

#include "movie.h"
#include "decoder.h"

#include <list>
#include <mutex>
#include <thread>
#include <atomic>
#include <vector>
#include <memory>

#include <SDL.h>

namespace jf {
	
	void uploadFrame(Buffer pbo, Texture tex, VideoFrame::Ptr frame) {
		tex.bind();
		
		pbo.bind();
		pbo.upload(frame->numBytes, frame->bytes);
		
		tex.upload(frame->width, frame->height, GL_RGB, 0);
		pbo.unbind();
		
		tex.generateMipMaps();
		tex.unbind();
	}
	
	MoviePlayer::MoviePlayer()
	:	demuxer(NULL)
	,	videoDecoder(NULL)
	,	state(Stopped)
	,	playStart(0)
	{}
	
	bool MoviePlayer::open(const char* path) {
		close();
	
		if(!(demuxer = Demuxer::open(path)))
			return false;
		
		if(!(videoDecoder = VideoDecoder::open(demuxer)))
		   return false;

		texture.create(GL_TEXTURE_2D, GL_RGB);
		texture.configure(TextureParameters()
						  .setFilters(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR)
						  .setLevels(0, 10));
		
		pixelBuffer.create(GL_PIXEL_UNPACK_BUFFER, GL_STREAM_DRAW);
		pixelBuffer.upload(videoDecoder->getBytesPerFrame(), NULL);
		
		float verts[] = {
			0,0, 0,1,
			1,0, 1,1,
			1,1, 1,0,
			0,1, 0,0
		};
		
		quad.create(GL_ARRAY_BUFFER, GL_STATIC_DRAW);
		quad.upload(sizeof(verts), verts);
		
		vao.create();
		VertexLayout layout;
		layout.addAttribute(0, 2, GL_FLOAT);
		layout.addAttribute(1, 2, GL_FLOAT);
		layout.fitStrideToAttributes();
		layout.configure();
		vao.unbind();
		
		quad.unbind();

		uploadFrame(pixelBuffer, texture, videoDecoder->nextFrame());
		
		return true;
	}
	
	void MoviePlayer::close() {
		state = Stopped;
		texture.destroy();
		pixelBuffer.destroy();
		vao.destroy();
		quad.destroy();
		if(videoDecoder) {
			delete videoDecoder;
			videoDecoder = NULL;
		}
		if(demuxer) {
			delete demuxer;
			demuxer = NULL;
		}
	}
	
	void MoviePlayer::play() {
	}
	
	void MoviePlayer::pause() {
	}
	
	void MoviePlayer::stop() {
	}
	
	void MoviePlayer::seek(double time) {
	}
	
	void MoviePlayer::previousFrame() {
		if(videoDecoder)
			uploadFrame(pixelBuffer, texture, videoDecoder->previousFrame());
	}
	
	void MoviePlayer::nextFrame() {
		if(videoDecoder)
			uploadFrame(pixelBuffer, texture, videoDecoder->nextFrame());
	}
	
	bool MoviePlayer::isPlaying() const {
		return state == Playing;
	}
	
	bool MoviePlayer::isPaused() const {
		return state == Paused;
	}
	
	bool MoviePlayer::isStopped() const {
		return state == Stopped;
	}

	void MoviePlayer::setRect(float x, float y, float w, float h) {
		if(videoDecoder) {
			int vW = videoDecoder->getWidth();
			int vH = videoDecoder->getHeight();
			
			float vR = vH / (float)vW;
			float hNew = w * vR;
			float wNew = w;

			if(hNew > h) {
				float scale = h / hNew;
				wNew *= scale;
				hNew *= scale;
			}
			
			x += (w - wNew) / 2.f;
			y += (h - hNew) / 2.f;
			
			float verts[] = {
				x, y,
				0, 1,
				
				x+wNew, y,
				1, 1,
				
				x+wNew, y+hNew,
				1, 0,
				
				x, y+hNew,
				0, 0
			};
			
			quad.bind();
			quad.upload(sizeof(verts), verts);
			quad.unbind();
		}
	}

	void MoviePlayer::draw() {
		if(videoDecoder) {
			texture.bind();
			vao.bind();
			glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
			vao.unbind();
			texture.unbind();
		}
	}
}