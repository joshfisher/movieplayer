//
//  GLView.m
//  curve
//
//  Created by Joshua Fisher on 12/31/12.
//  Copyright (c) 2012 Joshua Fisher. All rights reserved.
//

#import "GLView.h"
#import "render.h"
#import "movie.h"
#import <fstream>
#import <glm/gtc/matrix_transform.hpp>


class Movie;

void initFFMpeg();
void decoderThread(Movie*);

struct VideoFrame {
	std::vector<uint8_t*> buffer;
	float displayTime;
};


std::string readFile(const std::string& path, bool* success=NULL) {
	if(success)
		*success = false;
	
	std::string contents;
	
	std::ifstream file(path, std::ios::in|std::ios::binary);
	if(file) {
		file.seekg(0, std::ios::end);
		contents.resize(file.tellg());
		file.seekg(0, std::ios::beg);
		file.read(&contents[0], contents.size());
		file.close();
		
		if(success)
			*success = true;
	}
	
	return contents;
}


using namespace jf;
using namespace glm;

Program program;
Buffer buffer;
VertexArray vao;
MoviePlayer movie;
Texture video;

uint32_t frameCount;
double frameRate;
double frameMark;

@implementation GLView

+ (NSOpenGLPixelFormat*)defaultPixelFormat {
	NSOpenGLPixelFormatAttribute attribs[] = {
		NSOpenGLPFAAccelerated,
		NSOpenGLPFANoRecovery,
		NSOpenGLPFADepthSize, 24,
		NSOpenGLPFAStencilSize, 8,
		NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion3_2Core,
		NSOpenGLPFADoubleBuffer,
		0
	};
	return [[NSOpenGLPixelFormat alloc] initWithAttributes:attribs];
}

- (id)init {
	self = [super initWithFrame:NSMakeRect(0,0,100,100) pixelFormat:[GLView defaultPixelFormat]];
	
	GLint yes = 1;
	[self.openGLContext setValues:&yes forParameter:NSOpenGLCPSwapInterval];
	
	return self;
}

-(void)open:(NSString*)path {
	movie.open([path cStringUsingEncoding:NSUTF8StringEncoding]);
}

-(void)play {
	movie.play();
}

-(void)pause {
	movie.pause();
}

- (void)drawRect:(NSRect)dirtyRect {
	double now = CACurrentMediaTime();
	if(now - frameMark >= 1.0) {
		frameMark = now;
		frameRate = 1.0 / frameCount;
		frameCount = 0;
	}
	frameCount += 1;
	
	movie.getTexture();
	glClear(GL_COLOR_BUFFER_BIT);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	[self.openGLContext flushBuffer];
}

- (void)prepareOpenGL {
	glClearColor(1,1,1,1);
	
	program.addSource(GL_VERTEX_SHADER, readFile("basic.vert"));
	program.addSource(GL_FRAGMENT_SHADER, readFile("basic.frag"));
	
	program.create();
	program.compile();
	program.link({{"color", 0}}, {{"position",0}, {"texCoords",1}});
	program.bind();
	program.setUniform(program.getUniformLocation("texUnit"), 0);
	program.setUniform(program.getUniformLocation("viewMatrix"), glm::mat4(1.0));
	
	vao.create();
	
	buffer.create(GL_ARRAY_BUFFER, GL_STATIC_DRAW);
	vec2 pts[] = {
		vec2(10,10), vec2(0,1),
		vec2(490,10), vec2(1,1),
		vec2(490,280), vec2(1,0),
		vec2(10,280), vec2(0,0)
	};
	buffer.upload(sizeof(pts), &pts[0].x);
	
	VertexLayout layout;
	layout.addAttribute(0, 2, GL_FLOAT);
	layout.addAttribute(1, 2, GL_FLOAT);
	layout.fitStrideToAttributes();
	layout.configure();
}

- (void)reshape {
	NSRect r = [self bounds];
	glViewport(0.0, 0.0, r.size.width, r.size.height);
	program.setUniform(program.getUniformLocation("projectionMatrix"),
					   glm::ortho<float>(0.0, r.size.width, 0.0, r.size.height, -1.0, 1.0));
	
	vec2 pts[] = {
		vec2(10,10), vec2(0,1),
		vec2(r.size.width-10,10), vec2(1,1),
		vec2(r.size.width-10,r.size.height-10), vec2(1,0),
		vec2(10,r.size.height-10), vec2(0,0)
	};
	buffer.upload(sizeof(pts), &pts[0].x);
}

@end
