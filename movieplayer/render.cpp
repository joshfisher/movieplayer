//
//  render.cpp
//  curve
//
//  Created by Joshua Fisher on 12/31/12.
//  Copyright (c) 2012 Joshua Fisher. All rights reserved.
//

#include "render.h"

namespace jf {
	
	using std::map;
	using glm::vec2;
	using glm::vec3;
	using glm::vec4;
	using glm::mat4;
	
	#define BUFFER_OFFSET(i) (char*)NULL + i
	
	static const map<GLenum,int> glTypeSizeLookup = {
		{GL_BYTE, sizeof(GLbyte)},
		{GL_UNSIGNED_BYTE, sizeof(GLubyte)},
		{GL_SHORT, sizeof(GLshort)},
		{GL_UNSIGNED_SHORT, sizeof(GLushort)},
		{GL_INT, sizeof(GLint)},
		{GL_UNSIGNED_INT, sizeof(GLuint)},
		{GL_HALF_FLOAT, sizeof(GLhalf)},
		{GL_FLOAT, sizeof(GLfloat)},
		{GL_DOUBLE, sizeof(GLdouble)}};
	
	static const map<GLenum,const char*> glShaderTypenameLookup = {
		{GL_VERTEX_SHADER, "vertex"},
		{GL_GEOMETRY_SHADER, "geometry"},
		{GL_FRAGMENT_SHADER, "fragment"}
	};

	Buffer::Buffer()
	:	uid(0)
	,	target(0)
	,	usage(0)
	,	size(0)
	{}
	
	void Buffer::create(GLenum target, GLenum usage) {
		this->target = target;
		this->usage = usage;
		glGenBuffers(1, &uid);
		bind();
	}
	
	void Buffer::destroy() {
		glDeleteBuffers(1, &uid);
		uid = 0;
	}
	
	void Buffer::upload(GLsizeiptr size, const void* data) {
		this->size = size;
		glBufferData(target, size, data, usage);
	}
	
	void Buffer::bind() {
		glBindBuffer(target, uid);
	}
	
	void Buffer::unbind() {
		glBindBuffer(target, 0);
	}
	
	Buffer::operator bool() {
		return uid != 0;
	}
	
	void* Buffer::map(GLenum usage) {
		return glMapBuffer(target, usage);
	}
	
	void Buffer::unmap() {
		glUnmapBuffer(target);
	}
	
	VertexLayout::VertexLayout()
	:	stride(0)
	{}
	
	void VertexLayout::addAttribute(GLint loc, GLint size, GLenum type) {
		attributes.push_back({loc,size,type});
	}
	
	void VertexLayout::fitStrideToAttributes() {
		stride = 0;
		for(Attribute& attrib : attributes) {
			stride += attrib.size * glTypeSizeLookup.at(attrib.type);
		}
	}
	
	void VertexLayout::configure() {
		int offset = 0;
		for(Attribute& attrib : attributes) {
			glEnableVertexAttribArray(attrib.location);
			glVertexAttribPointer(attrib.location, attrib.size, attrib.type, GL_FALSE, stride, BUFFER_OFFSET(offset));
			offset += attrib.size * glTypeSizeLookup.at(attrib.type);
		}
	}
	
	VertexArray::VertexArray()
	:	uid(0)
	{}

	void VertexArray::create() {
		glGenVertexArrays(1, &uid);
		bind();
	}
	
	void VertexArray::destroy() {
		glDeleteVertexArrays(1, &uid);
		uid = 0;
	}
	
	void VertexArray::bind() {
		glBindVertexArray(uid);
	}
	
	void VertexArray::unbind() {
		glBindVertexArray(0);
	}

	Program::Program()
	:	uid(0)
	,	complete(GL_FALSE)
	{}
	
	void Program::create() {
		uid = glCreateProgram();
	}
	
	void Program::destroy() {
		glDeleteProgram(uid);
		uid = 0;
	}
	
	void Program::addSource(GLenum type, const std::string& src) {
		shaderSource[type] += src;
	}
	
	void Program::compile() {
		for(auto& shader : shaderSource) {
			GLuint suid = glCreateShader(shader.first);
			
			const char* ptr = shader.second.c_str();
			glShaderSource(suid, 1, &ptr, nullptr);
			
			glCompileShader(suid);
			
			GLint param;
			glGetShaderiv(suid, GL_COMPILE_STATUS, &param);
			
			if(param == GL_TRUE) {
				glAttachShader(uid, suid);
			}
			else {
				glGetShaderiv(suid, GL_INFO_LOG_LENGTH, &param);
				
				char* log = new char[param];
				glGetShaderInfoLog(suid, param, &param, log);
				printf("%s shader compile error:\n%s", glShaderTypenameLookup.at(shader.first), log);
				delete [] log;
				
				glDeleteShader(suid);
			}
		}
	}
	
	void Program::link(const std::string& fragName, GLuint fragLocation) {
		link({{fragName,fragLocation}}, {});
	}
	
	void Program::link(LocationMap fragLocations, LocationMap attribLocations) {
		for(auto& loc : fragLocations)
			glBindFragDataLocation(uid, loc.second, loc.first.c_str());
		for(auto& loc : attribLocations)
			glBindAttribLocation(uid, loc.second, loc.first.c_str());
		
		glLinkProgram(uid);
		
		glGetProgramiv(uid, GL_LINK_STATUS, &complete);
		
		if(complete != GL_TRUE) {
			GLint param;
			glGetProgramiv(uid, GL_INFO_LOG_LENGTH, &param);
			
			char* log = new char[param];
			glGetProgramInfoLog(uid, param, &param, log);
			printf("program link error:\n%s", log);
			delete [] log;
		}
	}
	
	void Program::bind() {
		glUseProgram(uid);
	}
	
	void Program::unbind() {
		glUseProgram(0);
	}
	
	GLint Program::getUniformLocation(const std::string& name) {
		if(uniformLocations.count(name)) {
			return uniformLocations[name];
		}
		
		GLint loc = glGetUniformLocation(uid, name.c_str());
		uniformLocations[name] = loc;
		return loc;
	}
	
	GLint Program::getAttributeLocation(const std::string &name) {
		if(attributeLocations.count(name))
			return attributeLocations[name];
		
		GLint loc = glGetAttribLocation(uid, name.c_str());
		attributeLocations[name] = loc;
		return loc;
	}
	
	void Program::setUniform(GLint loc, int val) {
		glUniform1f(loc, val);
	}

	void Program::setUniform(GLint loc, float val) {
		glUniform1f(loc, val);
	}
	
	void Program::setUniform(GLint loc, float* val, int count) {
		glUniform1fv(loc, count, val);
	}

	void Program::setUniform(GLint loc, glm::vec2 val) {
		glUniform2fv(loc, 1, &val.x);
	}
	
	void Program::setUniform(GLint loc, glm::vec3 val) {
		glUniform3fv(loc, 1, &val.x);
	}
	
	void Program::setUniform(GLint loc, glm::vec4 val) {
		glUniform4fv(loc, 1, &val.x);
	}
	
	void Program::setUniform(GLint loc, glm::mat4 val) {
		glUniformMatrix4fv(loc, 1, GL_FALSE, &val[0][0]);
	}

	TextureParameters::TextureParameters()
	:	minFilter(GL_NEAREST_MIPMAP_LINEAR)
	,	magFilter(GL_LINEAR)
	,	wrapS(GL_REPEAT)
	,	wrapT(GL_REPEAT)
	,	wrapR(GL_REPEAT)
	,	baseLevel(0)
	,	maxLevel(1000)
	{}
	
	TextureParameters& TextureParameters::setFilters(GLenum min, GLenum mag) {
		minFilter = min;
		magFilter = mag;
		return *this;
	}
	
	TextureParameters& TextureParameters::setWrap(GLenum s, GLenum t, GLenum r) {
		wrapS = s;
		wrapT = t;
		wrapR = r;
		return *this;
	}
	
	TextureParameters& TextureParameters::setLevels(GLint base, GLint max) {
		baseLevel = base;
		maxLevel = max;
		return *this;
	}

	Texture::Texture()
	:	uid(0)
	,	target(0)
	,	width(0)
	,	height(0)
	{}
	
	void Texture::create(GLenum target, GLenum format) {
		glGenTextures(1, &uid);
		this->target = target;
		this->format = format;
		bind();
	}
	
	void Texture::destroy() {
		glDeleteTextures(1, &uid);
	}
	
	void Texture::upload(GLsizei width, GLsizei height, GLenum uploadFormat, const GLubyte* pixels, int mipMapLevel) {
		this->width = width;
		this->height = height;
		glTexImage2D(target, mipMapLevel, format, width, height, 0, uploadFormat, GL_UNSIGNED_BYTE, pixels);
	}
	
	void Texture::generateMipMaps() {
		glGenerateMipmap(target);
	}
	
	void Texture::configure(TextureParameters params) {
		glTexParameteri(target, GL_TEXTURE_MIN_FILTER, params.minFilter);
		glTexParameteri(target, GL_TEXTURE_MAG_FILTER, params.magFilter);
		glTexParameteri(target, GL_TEXTURE_WRAP_S, params.wrapS);
		glTexParameteri(target, GL_TEXTURE_WRAP_T, params.wrapT);
		glTexParameteri(target, GL_TEXTURE_WRAP_R, params.wrapR);
		glTexParameteri(target, GL_TEXTURE_BASE_LEVEL, params.baseLevel);
		glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, params.maxLevel);
	}

	Texture::operator bool() {
		return uid != 0;
	}
	
	void Texture::bind() {
		glBindTexture(target, uid);
	}
	
	void Texture::unbind() {
		glBindTexture(target, 0);
	}
	
	void Texture::unbind(GLenum target) {
		glBindTexture(target, 0);
	}

	void Texture::setActiveUnit(int unit) {
		glActiveTexture(GL_TEXTURE0 + unit);
	}

}