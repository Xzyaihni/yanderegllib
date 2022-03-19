#include <array>
#include <cassert>
#include <iostream>
#include <climits>
#include <cmath>
#include <filesystem>
#include <fstream>

#include "glcore.h"

using namespace yandereconv;
using namespace yanderegl;
using namespace yanderegl::core;


initializer::initializer(bool stencil, bool antialiasing, bool culling)
{
	GLenum err = glewInit();
	if(err!=GLEW_OK)
	{
		throw std::runtime_error(reinterpret_cast<const char*>(glewGetErrorString(err)));
		
		#ifdef YANDEBUG
		glDebugMessageCallback(yandere_controller::debug_callback, NULL);
		#endif
	}
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_DEPTH_TEST);
}

model::buffers_nocopy::buffers_nocopy()
{
	gen_buffers();
}

model::buffers_nocopy::~buffers_nocopy()
{
	if(buffers_made && glIsVertexArray(vertex_array_object_id))
	{
		glBindVertexArray(vertex_array_object_id);
	
		if(glIsBuffer(vertex_buffer_object_id))
		{
			glDeleteBuffers(1, &vertex_buffer_object_id);
		}
		
		if(glIsBuffer(element_object_buffer_id))
		{
			glDeleteBuffers(1, &element_object_buffer_id);
		}
		
		if(glIsBuffer(object_data_buffer_id))
		{
			glDeleteBuffers(1, &object_data_buffer_id);
		}
		
		glDeleteVertexArrays(1, &vertex_array_object_id);
	}
}

model::buffers_nocopy::buffers_nocopy(const buffers_nocopy&)
{
	gen_buffers();
}

model::buffers_nocopy::buffers_nocopy(buffers_nocopy&& other) noexcept
{
	buffers_made = other.buffers_made;
	other.buffers_made = false;
	
	vertex_buffer_object_id = std::exchange(other.vertex_buffer_object_id, 0);
	element_object_buffer_id = std::exchange(other.element_object_buffer_id, 0);
	object_data_buffer_id = std::exchange(other.object_data_buffer_id, 0);
	vertex_array_object_id = std::exchange(other.vertex_array_object_id, 0);
	
	gen_buffers();
}

model::buffers_nocopy& model::buffers_nocopy::operator=(const buffers_nocopy&)
{
	gen_buffers();

	return *this;
}

model::buffers_nocopy& model::buffers_nocopy::operator=(buffers_nocopy&& other) noexcept
{
	if(this!=&other)
	{
		buffers_made = other.buffers_made;
		other.buffers_made = false;
		
		vertex_buffer_object_id = std::exchange(other.vertex_buffer_object_id, 0);
		element_object_buffer_id = std::exchange(other.element_object_buffer_id, 0);
		object_data_buffer_id = std::exchange(other.object_data_buffer_id, 0);
		vertex_array_object_id = std::exchange(other.vertex_array_object_id, 0);
	}
	
	return *this;
}

void model::buffers_nocopy::gen_buffers()
{
	if(buffers_made)
		return;

	buffers_made = true;


	glGenVertexArrays(1, &vertex_array_object_id);
	
	glBindVertexArray(vertex_array_object_id);

	glGenBuffers(1, &vertex_buffer_object_id);
	glGenBuffers(1, &element_object_buffer_id);
	glGenBuffers(1, &object_data_buffer_id);
	
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glEnableVertexAttribArray(3);
	glEnableVertexAttribArray(4);
	glEnableVertexAttribArray(5);
	glEnableVertexAttribArray(6);
	
	glVertexAttribDivisor(2, 1);
	glVertexAttribDivisor(3, 1);
	glVertexAttribDivisor(4, 1);
	glVertexAttribDivisor(5, 1);
	glVertexAttribDivisor(6, 1);
	
	glBindVertexArray(0);
}

model::model(std::string model_path) : _empty(false)
{
	if(model_path.find("!d")==0)
	{
		const std::string c_name = model_path.substr(2);
	
		if(!parse_default(c_name))
		{
			throw std::runtime_error(std::string("error creating default: ") + c_name);
		}
	} else
	{
		std::filesystem::path model_fpath(model_path);

		std::string extension = model_fpath.filename().extension().string();

		if(!parse_model(model_path, extension))
		{
			throw std::runtime_error(std::string("error parsing: ") + model_fpath.filename().string());
		}
	}

	_buffers.need_update = true;
}

void model::draw(const object_data* obj_data) const
{
	if(_empty)
		return;
		
	if(_buffers.need_update)
		update_buffers();

	glBindVertexArray(_buffers.vertex_array_object_id);
	
	glBindBuffer(GL_ARRAY_BUFFER, _buffers.object_data_buffer_id);
	glBufferData(GL_ARRAY_BUFFER, buffer_size, obj_data, GL_DYNAMIC_DRAW);
	
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, core::buffer_size, reinterpret_cast<void*>(0));
	glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, core::buffer_size, reinterpret_cast<void*>((sizeof(float)*4)));
	glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, core::buffer_size, reinterpret_cast<void*>((2*sizeof(float)*4)));
	glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, core::buffer_size, reinterpret_cast<void*>((3*sizeof(float)*4)));
	glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, core::buffer_size, reinterpret_cast<void*>((4*sizeof(float)*4)));
	
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _buffers.element_object_buffer_id);
	
	glDrawElements(GL_TRIANGLES, _indices.size(), GL_UNSIGNED_INT, 0);
	
	glBindVertexArray(0);
}

bool model::empty() const
{
	return _empty;
}

void model::vertices_insert(std::initializer_list<float> list)
{
	_vertices.insert(_vertices.end(), list);

	_buffers.need_update = true;
}

void model::indices_insert(std::initializer_list<unsigned> list)
{
	_empty = false;
	
	_indices.insert(_indices.end(), list);
	
	_buffers.need_update = true;
}

void model::clear()
{
	_empty = true;
	
	_vertices.clear();
	_indices.clear();

	_buffers.need_update = true;
}

bool model::parse_model(std::string model_path, std::string file_format)
{
	if(file_format==".obj")
	{
		std::ifstream model_stream(model_path);
		std::string c_model_line;

		std::vector<float> c_verts;
		std::vector<unsigned> vert_indices;
		std::vector<float> c_uvs;
		std::vector<unsigned> uv_indices;

		std::vector<float> c_normals;
		std::vector<unsigned> normal_indices;

		std::vector<std::string> saved_params;
		std::vector<unsigned> repeated_verts;

		while(std::getline(model_stream, c_model_line))
		{
			if(c_model_line.find("#")!=-1)
			{
				c_model_line = c_model_line.substr(0, c_model_line.find("#"));
			}

			if(c_model_line.length()==0)
			{
				continue;
			}

			if(c_model_line.find("vt")==0)
			{
				std::stringstream texuv_stream(c_model_line.substr(3));

				float x, y;

				texuv_stream >> x >> y;

				c_uvs.reserve(3);
				c_uvs.push_back(x);
				c_uvs.push_back(y);
			} else if(c_model_line.find("vn")==0)
			{
				std::stringstream normal_stream(c_model_line.substr(3));

				float x, y, z;

				normal_stream >> x >> y >> z;

				c_normals.reserve(3);
				c_normals.push_back(x);
				c_normals.push_back(y);
				c_normals.push_back(z);
			} else if(c_model_line.find("v")==0)
			{
				std::stringstream vertex_stream(c_model_line.substr(2));

				float x, y, z;

				vertex_stream >> x >> y >> z;

				c_verts.reserve(3);
				c_verts.push_back(x);
				c_verts.push_back(y);
				c_verts.push_back(z);
			} else if(c_model_line.find("f")==0)
			{
				std::vector<std::string> params = string_split(c_model_line.substr(2), " ");

				size_t params_size = params.size();

				if(params[params_size-1].length()==0)
				{
					params.pop_back();
				}

				std::vector<unsigned> order = {0, 1, 2};

				if(params_size>3)
				{
					for(int i = 3; i < params_size; i++)
					{
						order.push_back(0);
						order.push_back(i-1);
						order.push_back(i);
					}
				}

				for(const auto& p : order)
				{
					auto find_index = std::find(saved_params.begin(), saved_params.end(), params[p]);

					if(find_index!=saved_params.end())
					{
						repeated_verts.push_back((unsigned)(vert_indices.size()-std::distance(saved_params.begin(), find_index)));
					} else
					{
						repeated_verts.push_back(0);
					}

					saved_params.push_back(params[p]);

					std::vector<std::string> faceVals = string_split(params[p], "/");

					vert_indices.push_back((unsigned)(std::stoi(faceVals[0]))-1);

					if(faceVals.size()>=2 && faceVals[1].length()!=0)
					{
						uv_indices.push_back((unsigned)(std::stoi(faceVals[1])-1));
					} else
					{
						uv_indices.push_back(UINT_MAX);
					}

					if(faceVals.size()==3)
					{
						normal_indices.push_back((unsigned)(std::stoi(faceVals[2])-1));
					} else
					{
						normal_indices.push_back(UINT_MAX);
					}
				}
			} else
			{
				//std::cout << c_model_line << std::endl;
			}
		}

		size_t uvs_size = c_uvs.size();
		size_t uv_indicesSize = uv_indices.size();

		_vertices.clear();
		_indices.clear();

		int indexing_offset = 0;

		for(int v = 0; v < vert_indices.size(); v++)
		{
			if(repeated_verts[v]==0)
			{
				_vertices.push_back(c_verts[vert_indices[v]*3]);
				_vertices.push_back(c_verts[vert_indices[v]*3+1]);
				_vertices.push_back(c_verts[vert_indices[v]*3+2]);

				_indices.push_back(v-indexing_offset);

				if(uv_indices[v]!=UINT_MAX)
				{
					_vertices.push_back(c_uvs[uv_indices[v]*2]);
					_vertices.push_back(c_uvs[uv_indices[v]*2+1]);
				} else
				{
					_vertices.push_back(0);
					_vertices.push_back(0);
				}
			} else
			{
				_indices.push_back(_indices[v-repeated_verts[v]]);
				indexing_offset++;
			}
		}

		model_stream.close();

		return true;
	} else
	{
		return false;
	}
}

bool model::parse_default(const std::string name)
{
	if(name == "CIRCLE")
	{
		float vertices_count = y_const::circle_lod*M_PI;
		float circumference = M_PI*2;

		_vertices.reserve(circumference*vertices_count*(5/3)+5);

		for(int i = 0; i <= circumference*vertices_count; i+=3)
		{
			float scale_var = (i/static_cast<float>(vertices_count));
			_vertices.push_back(std::sin(scale_var));
			_vertices.push_back(std::cos(scale_var));
			_vertices.push_back(0.0f);

			//texture uvs
			_vertices.push_back(std::sin(scale_var));
			_vertices.push_back(std::cos(scale_var));
		}

		_indices.reserve(circumference*y_const::circle_lod+y_const::circle_lod);

		for(int i = 0; i <= circumference*y_const::circle_lod+y_const::circle_lod; i++)
		{
			_indices.push_back(0);
			_indices.push_back(i+2);
			_indices.push_back(i+1);
		}
	} else if(name == "TRIANGLE")
	{
		_vertices = {
		-1.0f, -1.0f, 0.0f,   0.0f, 0.0f,
		0.0f, 1.0f, 0.0f,	 0.5f, 1.0f,
		1.0f, -1.0f, 0.0f,	1.0f, 0.0f
		};

		_indices = {
		0, 2, 1,
		};
	} else if(name == "SQUARE")
	{
		_vertices = {
		-1, 1, 0,	0, 1,
		1, 1, 0,	 1, 1,
		1, -1, 0,	1, 0,
		-1, -1, 0,   0, 0,
		};

		_indices = {
		0, 2, 1,
		2, 0, 3,
		};
	} else if(name == "CUBE")
	{
		_vertices = {
		-1, 1, 1,	0, 1,
		1, -1, 1,	1, 0,
		-1, -1, 1,   0, 0,
		1, 1, 1,	 1, 1,
		-1, -1, 1,   0, 1,
		1, -1, 1,	1, 1,
		-1, -1, -1,  0, 0,
		1, -1, -1,   1, 0,
		-1, 1, 1,	0, 0,
		1, 1, 1,	 1, 0,
		-1, 1, -1,   0, 1,
		1, 1, -1,	1, 1,
		-1, 1, 1,	1, 1,
		-1, -1, 1,   1, 0,
		1, 1, 1,	 0, 1,
		1, -1, 1,	0, 0,
		};

		_indices = {
		0, 2, 1,
		0, 1, 3,
		4, 7, 5,
		4, 6, 7,
		8, 9, 10,
		9, 11, 10,
		10, 7, 6,
		10, 11, 7,
		10, 13, 12,
		10, 6, 13,
		14, 7, 11,
		14, 15, 7,
		};
	} else if(name == "PYRAMID")
	{
		_vertices = {
		0.0f, -1.0f, -1.0f,   0.0f, 0.0f,
		0.0f, 1.0f, 0.0f,	 0.5f, 0.5f,
		1.0f, -1.0f, 0.64f,   0.0f, 1.0f,
		-1.0f, -1.0f, 0.64f,  1.0f, 0.0f,
		};

		_indices = {
		0, 2, 1,
		3, 1, 2,
		3, 0, 1,
		0, 3, 2,
		};
	} else
	{
		return false;
	}
	
	return true;
}

void model::update_buffers() const
{
	glBindVertexArray(_buffers.vertex_array_object_id);

	glBindBuffer(GL_ARRAY_BUFFER, _buffers.vertex_array_object_id);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * _vertices.size(), _vertices.data(), GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _buffers.element_object_buffer_id);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned) * _indices.size(), _indices.data(), GL_STATIC_DRAW);
	
	glBindVertexArray(0);
}

//------------------------------------------------------------------------------------------------------------------
texture::buffers_nocopy::buffers_nocopy()
{
	gen_buffers();
}

texture::buffers_nocopy::~buffers_nocopy()
{
	if(buffers_made)
	{
		if(glIsBuffer(texture_buffer_object_id))
		{
			glDeleteBuffers(1, &texture_buffer_object_id);
		}
	}
}

texture::buffers_nocopy::buffers_nocopy(const buffers_nocopy&)
{
	gen_buffers();
}

texture::buffers_nocopy::buffers_nocopy(buffers_nocopy&& other) noexcept
{
	buffers_made = other.buffers_made;
	other.buffers_made = false;
	
	texture_buffer_object_id = std::exchange(other.texture_buffer_object_id, 0);
	
	gen_buffers();
}

texture::buffers_nocopy& texture::buffers_nocopy::operator=(const buffers_nocopy&)
{
	gen_buffers();

	return *this;
}

texture::buffers_nocopy& texture::buffers_nocopy::operator=(buffers_nocopy&& other) noexcept
{
	if(this!=&other)
	{
		buffers_made = other.buffers_made;
		other.buffers_made = false;
		
		texture_buffer_object_id = std::exchange(other.texture_buffer_object_id, 0);
	}
	
	return *this;
}

void texture::buffers_nocopy::gen_buffers()
{
	if(buffers_made)
		return;

	buffers_made = true;
	
	glGenBuffers(1, &texture_buffer_object_id);
}

texture::texture(std::string image_path) : _empty(false)
{
	if(image_path.find("!d")==0)
	{
		std::string default_texture = image_path.substr(2);
		if(default_texture == "SOLID")
		{
			_image = yandere_image();
			_image.width = 1;
			_image.height = 1;
			_type = GL_RGBA;
			_image.image = {255, 255, 255, 255};
		}
	} else
	{
		std::filesystem::path image_fpath(image_path);

		std::string extension = image_fpath.filename().extension().string();

		if(!parse_image(image_path, extension))
		{
			std::cout << "error parsing: " << image_fpath.filename() << std::endl;
		}
	}
}

texture::texture(yandere_image image) : _image(image), _empty(false)
{
	_type = calc_type(image.bpp);
	_image.flip();
}

void texture::set_current() const
{
	assert(!_empty);


	glBindTexture(GL_TEXTURE_2D, _buffers.texture_buffer_object_id);

	switch(_type)
	{
		case GL_RED:
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			break;
		case GL_RG:
			glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
			break;
		case GL_RGB:
			glPixelStorei(GL_UNPACK_ALIGNMENT, 3);
			break;
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, _type, _image.width, _image.height, 0, _type, GL_UNSIGNED_BYTE, _image.image.data());
	glGenerateMipmap(GL_TEXTURE_2D);
}

int texture::width() const
{
	return _image.width;
}

int texture::height() const
{
	return _image.height;
}

bool texture::parse_image(std::string image_path, std::string file_format)
{
	if(yandere_image::can_parse(file_format))
	{
		_image = yandere_image(image_path);
		_image.flip();

		_type = calc_type(_image.bpp);

		return true;
	}

	return false;
}

unsigned texture::calc_type(uint8_t bpp)
{
	switch(bpp)
	{
		case 4:
			return GL_RGBA;
		case 3:
			return GL_RGB;
		case 2:
			return GL_RG;
		case 1:
		default:
			return GL_RED;
	}
}

//----------------------------------------------------------------------------------------------------------------

shader::shader(std::string text, shader_type type) : _text(text), _type(type)
{
}

std::string& shader::text()
{
	return _text;
}

shader_type shader::type()
{
	return _type;
}


shader_program::shader_program(unsigned program_id) : _program_id(program_id)
{
	matrix_setup();
}

unsigned shader_program::add_num(const std::string name)
{
	return add_any(_shader_props, 0, 1, name);
}

unsigned shader_program::add_vec2(const std::string name)
{
	return add_any(_shader_props_vec2, yvec2{}, 2, name);
}

unsigned shader_program::add_vec3(const std::string name)
{
	return add_any(_shader_props_vec3, yvec3{}, 3, name);
}

unsigned shader_program::add_vec4(const std::string name)
{
	return add_any(_shader_props_vec4, yvec4{}, 4, name);
}


void shader_program::apply_uniforms()
{
	std::array<int, 4> vec_index_counts = {0, 0, 0, 0};

	for(prop& p : _props_vec)
	{
		switch(p.length)
		{
			case 4:
			{
				yvec4 v4_prop = _shader_props_vec4[vec_index_counts[3]];
				glUniform4f(p.location, v4_prop.x, v4_prop.y, v4_prop.z, v4_prop.w);
				break;
			}
			
			case 3:
			{
				yvec3 v3_prop = _shader_props_vec3[vec_index_counts[2]];
				glUniform3f(p.location, v3_prop.x, v3_prop.y, v3_prop.z);
				break;
			}
				
			case 2:
			{
				yvec2 v2_prop = _shader_props_vec2[vec_index_counts[1]];
				glUniform2f(p.location, v2_prop.x, v2_prop.y);
				break;
			}
		
			default:
				glUniform1f(p.location, _shader_props[vec_index_counts[0]]);
				break;
		}
		
		++vec_index_counts[p.length-1];
	}
}


template<typename T, typename V>
unsigned shader_program::add_any(T& type, V vec_type, int index, std::string name)
{
	unsigned prop_location = glGetUniformLocation(_program_id, name.c_str());
	assert(prop_location!=-1);
	
	_props_vec.push_back(prop{index, prop_location});
	
	type.push_back(vec_type);
	
	return (type.size()-1)*4;
}


unsigned shader_program::program() const
{
	return _program_id;
}


void shader_program::matrix_setup()
{
	glUseProgram(_program_id);

	_view_mat = glGetUniformLocation(_program_id, "view_mat");
	_projection_mat = glGetUniformLocation(_program_id, "projection_mat");
}

unsigned shader_program::view_mat() const
{
	return _view_mat;
}

unsigned shader_program::projection_mat() const
{
	return _projection_mat;
}
