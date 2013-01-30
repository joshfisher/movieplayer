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
	
	class Demuxer;
	class VideoDecoder;
	class AudioDecoder;
	
	class MoviePlayer {
	public:
		MoviePlayer();
		~MoviePlayer();

		bool open(const char* path);
		void close();
		void play();
		void pause();
		void stop();
		void seek(float sec);
		void previousFrame();
		void nextFrame();
		
		bool isPlaying() const;
		bool isPaused() const;
		bool isStopped() const;
		bool isFinished() const;
		
		void setRect(float x, float y, float w, float h);
		void draw();
		
	private:
		Demuxer* demuxer;
		VideoDecoder* videoDecoder;
		AudioDecoder* audioDecoder;
		
		enum {
			Playing,
			Paused,
			Stopped,
			Complete
		} state;
		
		uint32_t playStartTime;
		uint32_t pauseStartTime, pauseElapsedTime;
		
		Texture texture;
		Buffer pixelBuffer;

		VertexArray vao;
		Buffer quad;
	};

}