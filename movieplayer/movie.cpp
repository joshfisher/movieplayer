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
	
	bool uploadFrame(Buffer pbo, Texture tex, VideoFrame::Ptr frame) {
		if(!frame)
			return false;
		
		tex.bind();
		
		pbo.bind();
		pbo.upload(frame->numBytes, frame->bytes);
		
		tex.upload(frame->width, frame->height, GL_RGB, 0);
		pbo.unbind();
		
		tex.generateMipMaps();
		tex.unbind();
		
		return true;
	}
	
	MoviePlayer::MoviePlayer()
	:	demuxer(NULL)
	,	videoDecoder(NULL)
	,	audioDecoder(NULL)
	,	state(Stopped)
	,	playStartTime(0)
	,	pauseStartTime(0)
	,	pauseElapsedTime(0)
	{}
	
	MoviePlayer::~MoviePlayer() {
		close();
	}
	
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
		quad.configure(layout);
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
		if(audioDecoder) {
			delete audioDecoder;
			audioDecoder = NULL;
		}
		if(demuxer) {
			delete demuxer;
			demuxer = NULL;
		}
	}
	
	void MoviePlayer::play() {
		if(state != Playing) {
			switch(state) {
				case Stopped:
					videoDecoder->seekToFrame(0);
					playStartTime = SDL_GetTicks();
					pauseElapsedTime = 0;
					break;
					
				case Paused:
					pauseElapsedTime += (SDL_GetTicks() - pauseStartTime);
					break;
					
				default:
					break;
			}
			
			state = Playing;
		}
	}
	
	void MoviePlayer::pause() {
		if(state != Paused) {
			switch(state) {
				case Playing:
					pauseStartTime = SDL_GetTicks();
					break;
					
				default:
					break;
			}
			
			state = Paused;
		}
	}
	
	void MoviePlayer::stop() {
		if(state != Stopped) {
			state = Stopped;
		}
	}
	
	void MoviePlayer::seek(float time) {
		videoDecoder->seekToTime(time);
	}
	
	void MoviePlayer::previousFrame() {
		if(videoDecoder) {
			pause();
			uploadFrame(pixelBuffer, texture, videoDecoder->previousFrame());
		}
	}
	
	void MoviePlayer::nextFrame() {
		if(videoDecoder && state != Complete) {
			pause();
			if(!uploadFrame(pixelBuffer, texture, videoDecoder->nextFrame())) {
				if(videoDecoder->isLastFrame()) {
					state = Complete;
				}
			}
		}
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
	
	bool MoviePlayer::isFinished() const {
		return state == Complete;
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
			double elapsed = (SDL_GetTicks() - playStartTime - pauseElapsedTime) / 1000.0;
			double target = videoDecoder->getNextTime();
			
			if(state == Playing && elapsed >= target) {
				uploadFrame(pixelBuffer, texture, videoDecoder->nextFrame());
			}
			
			texture.bind();
			vao.bind();
			glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
			vao.unbind();
			texture.unbind();
		}
	}
}


