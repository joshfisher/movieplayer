//
//  main.m
//  curve
//
//  Created by Joshua Fisher on 12/31/12.
//  Copyright (c) 2012 Joshua Fisher. All rights reserved.
//

#include "movie.h"

#include <string>
#include <fstream>
#include <unistd.h>

#include <SDL.h>
#include <glm/gtc/matrix_transform.hpp>

std::string readFile(const std::string& path) {
	using namespace std;
	std::string response;
	std::ifstream in(path, ios::in | ios::binary);
	if(in) {
		in.seekg(0, ios::end);
		response.resize(in.tellg());
		in.seekg(0, ios::beg);
		in.read(&response[0], response.size());
		in.close();
	}
	return response;
}

int main(int argc, char *argv[]) {
	chdir("../../../");
	
	SDL_Init(SDL_INIT_VIDEO);
	
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	
	int width = 600;
	int height = 400;
	float aspect = height / (float)width;
	SDL_Window* win = SDL_CreateWindow("window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);
	SDL_GLContext ctx = SDL_GL_CreateContext((SDL_Window*)win);
	SDL_GL_MakeCurrent((SDL_Window*)win, (SDL_GLContext)ctx);
	SDL_GL_SetSwapInterval(1);
	
	SDL_RaiseWindow(win);
	
	using namespace jf;
	Program prog;
	prog.addSource(GL_VERTEX_SHADER, readFile("resources/basic.vert"));
	prog.addSource(GL_FRAGMENT_SHADER, readFile("resources/basic.frag"));
	prog.create();
	prog.compile();
	prog.link({{"color",0}}, {{"position",0},{"texCoords",1}});
	prog.bind();
	prog.setUniform(prog.getUniformLocation("texUnit"), 0);
	prog.setUniform(prog.getUniformLocation("viewMatrix"), glm::mat4(1.f));
	
	int projLoc = prog.getUniformLocation("projectionMatrix");
	prog.setUniform(projLoc, glm::ortho(0.f,1.f,0.f,aspect,-1.f,1.f));
	
	jf::MoviePlayer movie;
	if(movie.open("resources/real.mov")) {
		movie.setRect(0,0,1,aspect);
	}
	
	bool done = 0;
	SDL_Event event;
	while(!done) {
		while(SDL_PollEvent(&event)) {
			switch(event.type) {
				case SDL_WINDOWEVENT:
					switch(event.window.event) {
						case SDL_WINDOWEVENT_RESIZED:
							width = event.window.data1;
							height = event.window.data2;
							aspect = height / (float)width;
							glViewport(0, 0, width, height);
							prog.setUniform(projLoc, glm::ortho(0.f,1.f,0.f,aspect,-1.f,1.f));
							movie.setRect(0,0,1,aspect);
							break;
					}
					break;
					
				case SDL_KEYDOWN:
					switch(event.key.keysym.sym) {
						case SDLK_RIGHT:
							movie.nextFrame();
							break;
							
						case SDLK_LEFT:
							movie.previousFrame();
							break;
					}
					break;
					
				case SDL_QUIT:
					done = true;
					break;
			}
		}
		
		glClear(GL_COLOR_BUFFER_BIT);
		
		movie.draw();
		
		SDL_GL_SwapWindow(win);
	}
	
	prog.destroy();
	movie.close();
	
	SDL_GL_DeleteContext(ctx);
	SDL_DestroyWindow(win);
}
