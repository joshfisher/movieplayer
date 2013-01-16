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
	
	MoviePlayer::MoviePlayer()
	:	decoder(new Decoder)
	,	state(Stopped)
	,	playStart(0)
	{}
	
	bool MoviePlayer::open(const char* path) {
		close();
		
		if(decoder->open(path)) {
			int w, h, b;
			decoder->videoFrameSize(&w, &h, &b);

			texture.create(GL_TEXTURE_2D, GL_RGB);
			texture.configure(TextureParameters()
							  .setFilters(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR)
							  .setLevels(0, 10));
			
			pixelBuffer.create(GL_PIXEL_UNPACK_BUFFER, GL_STREAM_DRAW);
			pixelBuffer.upload(b, NULL);
			
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
		}
		
		return true;
	}
	
	void MoviePlayer::close() {
		decoder->close();
		texture.destroy();
		pixelBuffer.destroy();
		vao.destroy();
		quad.destroy();
	}
	
	void MoviePlayer::play() {
		if(decoder) {
			switch(state) {
				case Stopped:
					playStart = SDL_GetTicks() / 1000.0;
					decoder->start();
					break;
					
				case Paused:
					break;
					
				default:
					break;
			}
			
			state = Playing;
		}
	}
	
	void MoviePlayer::seek(double time) {
		if(decoder)
			decoder->seek(time);
	}
	
	void MoviePlayer::setRect(float x, float y, float w, float h) {
		if(decoder->hasVideo()) {
			int vW, vH, numBytes;
			decoder->videoFrameSize(&vW, &vH, &numBytes);
			
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
		if(decoder && decoder->hasVideo()) {
			texture.bind();
			
			double now = SDL_GetTicks() / 1000.0;
			VideoFrame::Ptr frame = decoder->consumeVideoFrame(now - playStart);
			
			if(frame) {
				// update the texture
				pixelBuffer.bind();
				pixelBuffer.upload(frame->numBytes, frame->bytes);
				
				texture.upload(frame->width, frame->height, GL_RGB, 0);
				pixelBuffer.unbind();
				
				texture.generateMipMaps();
			}
			
			vao.bind();
			glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
			vao.unbind();
			texture.unbind();
		}
	}
}