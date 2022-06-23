#include <array>
#include <cassert>
#include <iostream>
#include <climits>
#include <cmath>
#include <filesystem>
#include <fstream>

#include "glcore.h"

using namespace yangl;
using namespace yangl::core;


initializer::initializer()
{
	const GLenum err = glewInit();
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

	global::initialized = true;
	default_assets.initialize();
}

model_storage::container::~container()
{
	if(vertex_array_object_id!=-1 && glIsVertexArray(vertex_array_object_id))
	{
		glBindVertexArray(vertex_array_object_id);

		if(vertex_buffer_object_id!=-1)
			glDeleteBuffers(1, &vertex_buffer_object_id);

		if(element_object_buffer_id!=-1)
			glDeleteBuffers(1, &element_object_buffer_id);

		glDeleteVertexArrays(1, &vertex_array_object_id);
	}
}

void model_storage::container::generate()
{
	need_update = true;

	glGenVertexArrays(1, &vertex_array_object_id);

	glBindVertexArray(vertex_array_object_id);

	glGenBuffers(1, &vertex_buffer_object_id);
	glGenBuffers(1, &element_object_buffer_id);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	glBindVertexArray(0);
}

model_storage::model_storage()
{
}

model_storage::model_storage(const std::filesystem::path model_path)
{
	yconv::model c_model(model_path);

	_vertices = std::move(c_model.vertices);
	_indices = std::move(c_model.indices);
}

model_storage::model_storage(const default_model id)
{
	if(!parse_default(id))
		throw std::runtime_error(std::string("error creating default model: ")
			+ std::to_string(static_cast<int>(id)));
}

model_storage::model_storage(const std::vector<float> vertices, const std::vector<unsigned> indices)
: _vertices(vertices), _indices(indices)
{
}

void model_storage::draw_buffers(const container& buffers) const noexcept
{
	if(empty())
		return;


	glBindVertexArray(buffers.vertex_array_object_id);
	glDrawElements(GL_TRIANGLES, _indices.size(), GL_UNSIGNED_INT, nullptr);
}

bool model_storage::empty() const noexcept
{
	return _indices.size()==0;
}

void model_storage::vertices_insert(const std::initializer_list<float> list) noexcept
{
	_vertices.insert(_vertices.end(), list);
}

void model_storage::indices_insert(const std::initializer_list<unsigned> list) noexcept
{
	_indices.insert(_indices.end(), list);
}

void model_storage::clear() noexcept
{
	_vertices.clear();
	_indices.clear();
}

void model_storage::update_buffers(container& buffers) const
{
	if(!buffers.need_update)
		return;

	buffers.need_update = false;


	glBindVertexArray(buffers.vertex_array_object_id);


	glBindBuffer(GL_ARRAY_BUFFER, buffers.vertex_buffer_object_id);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
						  5 * sizeof(float), (void*)0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
						  5 * sizeof(float), (void*)(3 * sizeof(float)));

	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * _vertices.size(), _vertices.data(), GL_STATIC_DRAW);


	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers.element_object_buffer_id);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned) * _indices.size(), _indices.data(), GL_STATIC_DRAW);

	glBindVertexArray(0);
}

bool model_storage::parse_default(const default_model id)
{
	switch(id)
	{
		case default_model::circle:
		{
			const float vertices_count = yconst::circle_lod*M_PI;
			const float circumference = M_PI*2;

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

			_indices.reserve(circumference*yconst::circle_lod+yconst::circle_lod);

			for(int i = 0; i <= circumference*yconst::circle_lod+yconst::circle_lod; i++)
			{
				_indices.push_back(0);
				_indices.push_back(i+2);
				_indices.push_back(i+1);
			}
		}
		break;

		case default_model::triangle:
		{
			_vertices = {
				-1.0f, -1.0f, 0.0f,   0.0f, 0.0f,
				0.0f, 1.0f, 0.0f,	 0.5f, 1.0f,
				1.0f, -1.0f, 0.0f,	1.0f, 0.0f
			};

			_indices = {
				0, 2, 1,
			};
		}
		break;

		case default_model::plane:
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
		}
		break;

		case default_model::cube:
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
		}
		break;

		case default_model::pyramid:
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
		}
		break;

		default:
			return false;
	}

	return true;
}

void model::draw() const
{
	update_buffers(_buffers.container);

	draw_buffers(_buffers.container);
}

void model::vertices_insert(const std::initializer_list<float> list) noexcept
{
	model_storage::vertices_insert(list);

	_buffers.container.need_update = true;
}

void model::indices_insert(const std::initializer_list<unsigned> list) noexcept
{
	model_storage::indices_insert(list);

	_buffers.container.need_update = true;
}

void model::clear() noexcept
{
	model_storage::clear();

	_buffers.container.need_update = true;
}

//------------------------------------------------------------------------------------------------------------------
void model_manual::generate_buffers() noexcept
{
	if(!_state.generated)
	{
		_buffers.generate();
		_state.generated = true;
	}
}

void model_manual::draw() const
{
	update_buffers(_buffers);

	draw_buffers(_buffers);
}

void model_manual::vertices_insert(const std::initializer_list<float> list) noexcept
{
	model_storage::vertices_insert(list);

	_buffers.need_update = true;
}

void model_manual::indices_insert(const std::initializer_list<unsigned> list) noexcept
{
	model_storage::indices_insert(list);

	_buffers.need_update = true;
}

void model_manual::clear() noexcept
{
	model_storage::clear();

	_buffers.need_update = true;
}

//------------------------------------------------------------------------------------------------------------------
texture::container::~container()
{
	if(texture_buffer_object_id!=-1)
		glDeleteTextures(1, &texture_buffer_object_id);
}

void texture::container::generate()
{
	glGenTextures(1, &texture_buffer_object_id);
}

texture::texture(const std::string image_path) : _empty(false)
{
	std::filesystem::path image_fpath(image_path);

	const std::string extension = image_fpath.filename().extension().string();

	if(!parse_image(image_path, extension))
		throw std::runtime_error(std::string("error parsing image: ")
			+ image_path);

	update_buffers();
}

texture::texture(const default_texture id) : _empty(false)
{
	if(!parse_default(id))
		throw std::runtime_error(std::string("error parsing default texture: ")
			+ std::to_string(static_cast<int>(id)));

	update_buffers();
}

texture::texture(const yconv::image image) : _image(image), _empty(false)
{
	_type = calc_type(image.bpp);
	_image.flip();

	update_buffers();
}

texture::texture(const int width, const int height, const std::vector<uint8_t> data)
: _empty(false)
{
	_image = yconv::image(width, height, data.size()/(width*height), data);

	update_buffers();
}

void texture::set_current() const
{
	assert(!_empty);


	glBindTexture(GL_TEXTURE_2D, _buffers.container.texture_buffer_object_id);
}

int texture::width() const
{
	return _image.width;
}

int texture::height() const
{
	return _image.height;
}

void texture::update_buffers() const
{
	assert(!_empty);


	glBindTexture(GL_TEXTURE_2D, _buffers.container.texture_buffer_object_id);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, _type, _image.width, _image.height, 0, _type, GL_UNSIGNED_BYTE, _image.data.data());
	glGenerateMipmap(GL_TEXTURE_2D);
}

bool texture::parse_image(const std::string image_path, const std::string file_format)
{
	if(yconv::image::can_parse(file_format))
	{
		_image = yconv::image(image_path);
		_image.flip();

		_type = calc_type(_image.bpp);

		return true;
	}

	return false;
}

bool texture::parse_default(const default_texture id)
{
	switch(id)
	{
		case default_texture::solid:
			_image = yconv::image(1, 1, 4, {255, 255, 255, 255});
			_type = GL_RGBA;
		break;

		default:
		return false;
	}

	return true;
}

unsigned texture::calc_type(const uint8_t bpp)
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

shader::shader(const std::string text, const shader_type type)
: _text(text), _type(type)
{
}

shader::shader(const default_shader shader, const shader_type type)
: _type(type)
{
	const std::string s_shader = std::to_string(static_cast<int>(shader));
	const std::string s_type = std::to_string(static_cast<int>(type));

	switch(shader)
	{
		case default_shader::world:
		{
			switch(type)
			{
				case shader_type::fragment:
				{
					_text = "#version 330 core\n"
					"out vec4 fragColor;\n"
					"in vec2 tex_coord;\n"
					"uniform sampler2D user_texture;\n"
					"void main()\n"
					"{\n"
					"fragColor = vec4(1, 1, 1, 1) * texture(user_texture, tex_coord);\n"
					"}";
				}
				break;

				case shader_type::vertex:
				{
					_text = "#version 330 core\n"
					"layout (location = 0) in vec3 a_pos;\n"
					"layout (location = 1) in vec2 a_tex_coordinate;\n"
					"uniform mat4 view_mat;\n"
					"uniform mat4 projection_mat;\n"
					"uniform mat4 model_mat;\n"
					"out vec2 tex_coord;\n"
					"void main()\n"
					"{\n"
					"gl_Position = projection_mat * view_mat * model_mat * vec4(a_pos, 1.0f);\n"
					"tex_coord = a_tex_coordinate;\n"
					"}";
				}
				break;

				default:
					throw std::runtime_error(std::string("default shader ")
						+ s_shader + std::string(" has no type: ") + s_type);
			}
		}
		break;

		case default_shader::gui:
		{
			switch(type)
			{
				case shader_type::fragment:
				{
					_text = "#version 330 core\n"
					"out vec4 fragColor;\n"
					"in vec2 tex_coord;\n"
					"uniform sampler2D user_texture;\n"
					"void main()\n"
					"{\n"
					"fragColor = vec4(vec3(1, 1, 1), texture(user_texture, tex_coord).r);\n"
					"}";
				}
				break;

				case shader_type::vertex:
				{
					_text = "#version 330 core\n"
					"layout (location = 0) in vec3 a_pos;\n"
					"layout (location = 1) in vec2 a_tex_coordinate;\n"
					"uniform mat4 view_mat;\n"
					"uniform mat4 projection_mat;\n"
					"uniform mat4 model_mat;\n"
					"out vec2 tex_coord;\n"
					"void main()\n"
					"{\n"
					"gl_Position = projection_mat * view_mat * model_mat * vec4(a_pos, 1.0f);\n"
					"tex_coord = a_tex_coordinate;\n"
					"}";
				}
				break;

				default:
					throw std::runtime_error(std::string("default shader ")
					+ s_shader + std::string(" has no type: ") + s_type);
			}
		}
		break;

		default:
			throw std::runtime_error(std::string("default shader doesnt exist: ")
				+ s_shader);
	}
}

const std::string& shader::text() const
{
	return _text;
}

shader_type shader::type() const
{
	return _type;
}

shader_program::container::~container()
{
	if(program_id!=-1 && glIsProgram(program_id))
		glDeleteProgram(program_id);
}

void shader_program::container::generate()
{
	need_update = true;
	program_id = glCreateProgram();
}

shader_program::shader_program(
	const shader fragment, const shader vertex, const shader geometry)
: _fragment(fragment), _vertex(vertex), _geometry(geometry)
{
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

void shader_program::apply_uniforms(const camera* cam, const glm::mat4& transform) const noexcept
{
	assert(cam!=nullptr);

	generate_shaders();


	glUseProgram(_buffers.container.program_id);

	std::array<int, 4> vec_index_counts = {0, 0, 0, 0};

	for(const prop& p : _props_vec)
	{
		switch(p.length)
		{
			case 4:
			{
				const yvec4 v4_prop = _shader_props_vec4[vec_index_counts[3]];
				glUniform4f(p.location, v4_prop.x, v4_prop.y, v4_prop.z, v4_prop.w);
				break;
			}
			
			case 3:
			{
				const yvec3 v3_prop = _shader_props_vec3[vec_index_counts[2]];
				glUniform3f(p.location, v3_prop.x, v3_prop.y, v3_prop.z);
				break;
			}
				
			case 2:
			{
				const yvec2 v2_prop = _shader_props_vec2[vec_index_counts[1]];
				glUniform2f(p.location, v2_prop.x, v2_prop.y);
				break;
			}
		
			default:
				glUniform1f(p.location, _shader_props[vec_index_counts[0]]);
				break;
		}
		
		++vec_index_counts[p.length-1];
	}

	glUniformMatrix4fv(_view_mat, 1, GL_FALSE, glm::value_ptr(cam->view_matrix()));
	glUniformMatrix4fv(_projection_mat, 1, GL_FALSE, glm::value_ptr(cam->projection_matrix()));

	glUniformMatrix4fv(_model_mat, 1, GL_FALSE, glm::value_ptr(transform));
}

void shader_program::output_error(const unsigned object_id, const bool is_program)
{
	char error_info[512];
	if(is_program)
	{
		glGetProgramInfoLog(object_id, 512, NULL, error_info);
	} else
	{
		glGetShaderInfoLog(object_id, 512, NULL, error_info);
	}

	throw std::runtime_error(std::string("LOAD ERROR: ")+std::string(error_info));
}

template<typename T, typename V>
unsigned shader_program::add_any(T& type, const V vec_type, const int index, const std::string name)
{
	generate_shaders();

	const unsigned prop_location = glGetUniformLocation(_buffers.container.program_id, name.c_str());
	assert(prop_location!=-1);
	
	_props_vec.push_back(prop{index, prop_location});
	
	type.push_back(vec_type);
	
	return (type.size()-1)*4;
}


unsigned shader_program::program() const
{
	return _buffers.container.program_id;
}


void shader_program::find_uniforms() const
{
	glUseProgram(_buffers.container.program_id);

	_view_mat = glGetUniformLocation(_buffers.container.program_id, "view_mat");
	_projection_mat = glGetUniformLocation(_buffers.container.program_id, "projection_mat");
	_model_mat = glGetUniformLocation(_buffers.container.program_id, "model_mat");
}

void shader_program::generate_shaders() const
{
	if(!_buffers.container.need_update)
		return;

	_buffers.container.need_update = false;


	const char* fragment_shader_src = _fragment.text().c_str();
	const char* vertex_shader_src = _vertex.text().c_str();

	if(fragment_shader_src=="")
		throw std::runtime_error("fragment shader is empty");

	if(vertex_shader_src=="")
		throw std::runtime_error("vertex shader is empty");


	int success;

	//fragment shader
	const unsigned fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader_id, 1, &fragment_shader_src, NULL);
	glCompileShader(fragment_shader_id);

	glGetShaderiv(fragment_shader_id, GL_COMPILE_STATUS, &success);
	if(!success) output_error(fragment_shader_id);

	glAttachShader(_buffers.container.program_id, fragment_shader_id);


	//vertex shader
	const unsigned vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader_id, 1, &vertex_shader_src, NULL);
	glCompileShader(vertex_shader_id);

	glGetShaderiv(vertex_shader_id, GL_COMPILE_STATUS, &success);
	if(!success) output_error(vertex_shader_id);

	glAttachShader(_buffers.container.program_id, vertex_shader_id);


	unsigned geometry_shader_id = -1;
	//geometry shader
	if(_geometry.text()!="")
	{
		const char* geometry_shader_src = _geometry.text().c_str();

		geometry_shader_id = glCreateShader(GL_GEOMETRY_SHADER);
		glShaderSource(geometry_shader_id, 1, &fragment_shader_src, NULL);
		glCompileShader(geometry_shader_id);

		glGetShaderiv(geometry_shader_id, GL_COMPILE_STATUS, &success);
		if(!success) output_error(geometry_shader_id);

		glAttachShader(_buffers.container.program_id, geometry_shader_id);
	}

	glLinkProgram(_buffers.container.program_id);

	glGetProgramiv(_buffers.container.program_id, GL_LINK_STATUS, &success);
	if(!success) output_error(_buffers.container.program_id, true);

	//clean up
	if(fragment_shader_id!=-1) glDeleteShader(fragment_shader_id);
	if(vertex_shader_id!=-1) glDeleteShader(vertex_shader_id);
	if(geometry_shader_id!=-1) glDeleteShader(geometry_shader_id);

	find_uniforms();
}

void default_assets_control::initialize()
{
	for(int i = 0; i < static_cast<int>(default_texture::LAST); ++i)
	{
		_textures[i] = core::texture(static_cast<default_texture>(i));
	}

	for(int i = 0; i < static_cast<int>(default_model::LAST); ++i)
	{
		_models[i] = core::model(static_cast<default_model>(i));
	}

	for(int i = 0; i < static_cast<int>(default_shader::LAST); ++i)
	{
		_shaders[i] = core::shader_program(
			core::shader(static_cast<default_shader>(i), shader_type::fragment),
			core::shader(static_cast<default_shader>(i), shader_type::vertex),
			core::shader());
	}
}

const core::texture*
default_assets_control::texture(const default_texture id) const noexcept
{
	return &_textures[static_cast<int>(id)];
}

const core::model*
default_assets_control::model(const default_model id) const noexcept
{
	return &_models[static_cast<int>(id)];
}

const core::shader_program*
default_assets_control::shader(const default_shader id) const noexcept
{
	return &_shaders[static_cast<int>(id)];
}
