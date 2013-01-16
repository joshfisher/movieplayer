//
//  Movie.h
//  curve
//
//  Created by Joshua Fisher on 1/8/13.
//  Copyright (c) 2013 Joshua Fisher. All rights reserved.
//

#pragma once

#include "render.h"

namespace jf {
	
	class Decoder;
	
	class MoviePlayer {
	public:
		class MovieDecodeContext;
		
		MoviePlayer();

		bool open(const char* path);
		void close();
		void play();
		void pause();
		void stop();
		void seek(double sec);
		
		bool isPlaying() const;
		bool isPaused() const;
		bool isStopped() const;
		
		void setRect(float x, float y, float w, float h);
		void draw();
		
	private:
		Decoder* decoder;
		
		enum {
			Playing,
			Paused,
			Stopped
		} state;
		
		double playStart;
		
		Texture texture;
		Buffer pixelBuffer;

		VertexArray vao;
		Buffer quad;
	};

}