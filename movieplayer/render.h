//
//  render.h
//  curve
//
//  Created by Joshua Fisher on 12/31/12.
//  Copyright (c) 2012 Joshua Fisher. All rights reserved.
//

#pragma once

#include <map>
#include <vector>
#include <string>

#include <OpenGL/gl3.h>

#include <glm/glm.hpp>

namespace jf {
	
	class Buffer {
	public:
		GLuint uid;
		GLenum target;
		GLenum usage;
		GLsizeiptr size;
		
		Buffer();
		void create(GLenum target, GLenum usage);
		void destroy();
		
		void bind();
		void unbind();

		void upload(GLsizeiptr size, const void* data);
		
		void* map(GLenum usage);
		void unmap();
		
		explicit operator bool();
	};

	struct VertexLayout {
	public:
		struct Attribute {
			GLint location;
			GLint size;
			GLenum type;
		};
		GLsizei stride;
		std::vector<Attribute> attributes;
		
		VertexLayout();
		void addAttribute(GLint loc, GLint size, GLenum type);
		void fitStrideToAttributes();
		void configure();
	};

	class VertexArray {
	public:
		GLuint uid;
		
		VertexArray();
		void create();
		void destroy();
		void bind();
		void unbind();
	};
	
	class Program {
	public:
		typedef std::map<std::string,GLint> LocationMap;
		
		GLuint uid;
		GLint complete;
		LocationMap uniformLocations;
		LocationMap attributeLocations;
		std::map<GLenum,std::string> shaderSource;
		
		Program();
		void addSource(GLenum type, const std::string& src);
		
		void create();
		void destroy();
		void compile();
		void link(const std::string& fragName, GLuint fragLocation);
		void link(LocationMap fragLocations, LocationMap attribLocations);

		void bind();
		void unbind();
		
		GLint getUniformLocation(const std::string& name);
		GLint getAttributeLocation(const std::string& name);
		
		void setUniform(GLint loc, int val);
		void setUniform(GLint loc, float val);
		void setUniform(GLint loc, float* val, int count);
		void setUniform(GLint loc, glm::vec2 val);
		void setUniform(GLint loc, glm::vec3 val);
		void setUniform(GLint loc, glm::vec4 val);
		void setUniform(GLint loc, glm::mat4 val);
	};
	
	class TextureParameters {
	public:
		GLenum minFilter, magFilter;
		GLenum wrapS, wrapT, wrapR;
		GLint baseLevel, maxLevel;
		
		TextureParameters();
		TextureParameters& setFilters(GLenum min, GLenum mag);
		TextureParameters& setWrap(GLenum s, GLenum t, GLenum r=GL_REPEAT);
		TextureParameters& setLevels(GLint base, GLint max);
	};
	
	class Texture {
	public:
		GLuint uid;
		GLenum target;
		GLenum format;
		GLsizei width, height;
		
		Texture();
		void create(GLenum target, GLenum format);
		void destroy();
		void upload(GLsizei width, GLsizei height, GLenum uploadFormat, const GLubyte* pixels, int mipMapLevel=0);
		void generateMipMaps();
		void configure(TextureParameters params);
		
		explicit operator bool();
		
		void bind();
		void unbind();
		static void unbind(GLenum target);
		
		static void setActiveUnit(int unit);
	};
	
}