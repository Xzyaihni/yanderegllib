#include <iostream>
#include <fstream>
#include <cmath>
#include <filesystem>
#include <algorithm>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include "glcyan.h"
#include "glcore.h"

using namespace yanderegl;
using namespace yandereconv;

yandere_camera::yandere_camera(const yan_position position, const yan_position look_position) : _projection_matrix(glm::mat4(1.0f))
{
	_camera_position = glm::vec3(position.x, position.y, position.z);
	_camera_direction = glm::normalize(glm::vec3(look_position.x, look_position.y, look_position.z)-_camera_position);

	calculate_view();
}

void yandere_camera::create_projection(const std::array<float, 4> ortho_box, const std::array<float, 2> planes)
{
	_projection_exists = true;

	_projection_matrix = glm::ortho(ortho_box[0], ortho_box[1], ortho_box[2], ortho_box[3], ortho_box[0], planes[1]);
	_clip_close = planes[0];
	_clip_far = planes[1];
	
	calculate_planes();
}

void yandere_camera::create_projection(const float fov, const float aspect, const std::array<float, 2> planes)
{
	_projection_exists = true;

	_projection_matrix = glm::perspective(glm::radians(fov), aspect, planes[0], planes[1]);
	_clip_close = planes[0];
	_clip_far = planes[1];
	
	calculate_planes();
}

void yandere_camera::set_position(const yan_position position)
{
	_camera_position = glm::vec3(position.x, position.y, position.z);

	calculate_view();
}

void yandere_camera::set_position_x(const float x)
{
	_camera_position.x = x;

	calculate_view();
}

void yandere_camera::set_position_y(const float y)
{
	_camera_position.y = y;

	calculate_view();
}

void yandere_camera::set_position_z(const float z)
{
	_camera_position.z = z;

	calculate_view();
}

void yandere_camera::set_rotation(const float yaw, const float pitch)
{
	_camera_direction.x = std::cos(yaw) * std::cos(pitch);
	_camera_direction.y = std::sin(pitch);
	_camera_direction.z = std::sin(yaw) * std::cos(pitch);

	calculate_view();
}

void yandere_camera::set_direction(const std::array<float, 3> direction)
{
	_camera_direction = glm::vec3(direction[0], direction[1], direction[2]);

	calculate_view();
}

void yandere_camera::look_at(const std::array<float, 3> look_position)
{
	_camera_direction = glm::normalize(glm::vec3(look_position[0], look_position[1], look_position[2])-_camera_position);

	calculate_view();
}

void yandere_camera::translate_position(const yan_position delta)
{
	_camera_position += delta.x*_camera_right;
	_camera_position += delta.y*_camera_up;
	_camera_position += delta.z*_camera_forward;

	calculate_view();
}

yan_position yandere_camera::position() const noexcept
{
	return {_camera_position.x, _camera_position.y, _camera_position.z};
}

int yandere_camera::x_point_side(const yan_position point) const noexcept
{
	if(_planes[side::left].a*point.x+_planes[side::left].b*point.y+_planes[side::left].c*point.z+_planes[side::left].d>0
	&& _planes[side::right].a*point.x+_planes[side::right].b*point.y+_planes[side::right].c*point.z+_planes[side::right].d>0)
	{
		return 0;
	} else
	{
		return _planes[side::left].a*point.x+_planes[side::left].b*point.y+_planes[side::left].c*point.z+_planes[side::left].d>0 ? 1 : -1;
	}
}

int yandere_camera::y_point_side(const yan_position point) const noexcept
{
	if(_planes[side::down].a*point.x+_planes[side::down].b*point.y+_planes[side::down].c*point.z+_planes[side::down].d>0
	&& _planes[side::up].a*point.x+_planes[side::up].b*point.y+_planes[side::up].c*point.z+_planes[side::up].d>0)
	{
		return 0;
	} else
	{
		return _planes[side::down].a*point.x+_planes[side::down].b*point.y+_planes[side::down].c*point.z+_planes[side::down].d>0 ? 1 : -1;
	}
}

int yandere_camera::z_point_side(const yan_position point) const noexcept
{
	if(_planes[side::back].a*point.x+_planes[side::back].b*point.y+_planes[side::back].c*point.z+_planes[side::back].d>0
	&& _planes[side::forward].a*point.x+_planes[side::forward].b*point.y+_planes[side::forward].c*point.z+_planes[side::forward].d>0)
	{
		return 0;
	} else
	{
		return _planes[side::back].a*point.x+_planes[side::back].b*point.y+_planes[side::back].c*point.z+_planes[side::back].d>0 ? 1 : -1;
	}
}

bool yandere_camera::cube_in_frustum(const yan_position middle, const float size) const noexcept
{
	assert(_view_exists && _projection_exists);

	const float left_wall = middle.x-size;
	const float right_wall = middle.x+size;
	const float down_wall = middle.y-size;
	const float up_wall = middle.y+size;
	const float back_wall = middle.z-size;
	const float forward_wall = middle.z+size;

	for(int i = 0; i < 6; ++i)
	{
		if(_planes[i].a * left_wall + _planes[i].b * down_wall + _planes[i].c * back_wall > -_planes[i].d)
			continue;
			
		if(_planes[i].a * right_wall + _planes[i].b * down_wall + _planes[i].c * back_wall > -_planes[i].d)
			continue;
			
		if(_planes[i].a * left_wall + _planes[i].b * down_wall + _planes[i].c * forward_wall > -_planes[i].d)
			continue;
			
		if(_planes[i].a * right_wall + _planes[i].b * down_wall + _planes[i].c * forward_wall > -_planes[i].d)
			continue;
			
		if(_planes[i].a * left_wall + _planes[i].b * up_wall + _planes[i].c * back_wall > -_planes[i].d)
			continue;
			
		if(_planes[i].a * right_wall + _planes[i].b * up_wall + _planes[i].c * back_wall > -_planes[i].d)
			continue;
			
		if(_planes[i].a * left_wall + _planes[i].b * up_wall + _planes[i].c * forward_wall > -_planes[i].d)
			continue;
			
		if(_planes[i].a * right_wall + _planes[i].b * up_wall + _planes[i].c * forward_wall > -_planes[i].d)
			continue;
		
		return false;
	}
	
	return true;
}


const glm::mat4* yandere_camera::proj_view_matrix_ptr() const noexcept
{
	assert(_projection_exists && _view_exists);
	return &_proj_view_matrix;
}

const glm::mat4* yandere_camera::view_matrix_ptr() const noexcept
{
	assert(_view_exists);
	return &_view_matrix;
}

const glm::mat4* yandere_camera::projection_matrix_ptr() const noexcept
{
	assert(_projection_exists);
	return &_projection_matrix;
}

float yandere_camera::clip_far() const noexcept
{
	return _clip_far;
}

float yandere_camera::clip_close() const noexcept
{
	return _clip_close;
}

void yandere_camera::calculate_view()
{
	_view_exists = true;

	_camera_forward = glm::normalize(_camera_direction);
	_camera_right = glm::normalize(glm::cross(glm::vec3(0, 1, 0), _camera_direction));
	_camera_up = glm::cross(_camera_direction, _camera_right);

	_view_matrix = glm::lookAt(_camera_position, _camera_position + _camera_forward, _camera_up);
	
	calculate_planes();
}

void yandere_camera::calculate_planes()
{
	_proj_view_matrix =  _projection_matrix * _view_matrix;
	
	_planes[side::left] = {_proj_view_matrix[0][3]+_proj_view_matrix[0][0],
				  _proj_view_matrix[1][3]+_proj_view_matrix[1][0],
				  _proj_view_matrix[2][3]+_proj_view_matrix[2][0],
				  _proj_view_matrix[3][3]+_proj_view_matrix[3][0]};
				  
	_planes[side::right] = {_proj_view_matrix[0][3]-_proj_view_matrix[0][0],
				   _proj_view_matrix[1][3]-_proj_view_matrix[1][0],
				   _proj_view_matrix[2][3]-_proj_view_matrix[2][0],
				   _proj_view_matrix[3][3]-_proj_view_matrix[3][0]};
				  
	_planes[side::down] = {_proj_view_matrix[0][3]+_proj_view_matrix[0][1],
				  _proj_view_matrix[1][3]+_proj_view_matrix[1][1],
				  _proj_view_matrix[2][3]+_proj_view_matrix[2][1],
				  _proj_view_matrix[3][3]+_proj_view_matrix[3][1]};
				  
	_planes[side::up] = {_proj_view_matrix[0][3]-_proj_view_matrix[0][1],
				_proj_view_matrix[1][3]-_proj_view_matrix[1][1],
				_proj_view_matrix[2][3]-_proj_view_matrix[2][1],
				_proj_view_matrix[3][3]-_proj_view_matrix[3][1]};
				  
	_planes[side::back] = {_proj_view_matrix[0][3]+_proj_view_matrix[0][2],
				  _proj_view_matrix[1][3]+_proj_view_matrix[1][2],
				  _proj_view_matrix[2][3]+_proj_view_matrix[2][2],
				  _proj_view_matrix[3][3]+_proj_view_matrix[3][2]};
				  
	_planes[side::forward] = {_proj_view_matrix[0][3]-_proj_view_matrix[0][2],
					 _proj_view_matrix[1][3]-_proj_view_matrix[1][2],
					 _proj_view_matrix[2][3]-_proj_view_matrix[2][2],
					 _proj_view_matrix[3][3]-_proj_view_matrix[3][2]};
}

//------------------------------------------------------------------------------------------------------------------
yandere_shader_program::yandere_shader_program(const yandere_controller* ctl, std::vector<core::shader_program>* shader_programs_ptr, const unsigned id)
: _ctl(ctl), _shader_programs_ptr(shader_programs_ptr), _id(id)
{
}

yandere_shader_program::yandere_shader_program(const yandere_controller* ctl, core::shader_program* shader_program_ptr)
: _ctl(ctl), _shader_program_ptr(shader_program_ptr)
{
}

void yandere_shader_program::set_current() const
{
	core::shader_program* c_program = c_shader_program();

	glUseProgram(c_program->program());

	glUniformMatrix4fv(c_program->view_mat(), 1, GL_FALSE, glm::value_ptr(*_ctl->view_matrix_ptr()));
	glUniformMatrix4fv(c_program->projection_mat(), 1, GL_FALSE, glm::value_ptr(*_ctl->projection_matrix_ptr()));

	c_program->apply_uniforms();
}

unsigned yandere_shader_program::add_num(const std::string name)
{
	return c_shader_program()->add_num(name);
}

unsigned yandere_shader_program::add_vec2(const std::string name)
{
	
	return c_shader_program()->add_vec2(name);
}

unsigned yandere_shader_program::add_vec3(const std::string name)
{
	return c_shader_program()->add_vec3(name);
}

unsigned yandere_shader_program::add_vec4(const std::string name)
{
	return c_shader_program()->add_vec4(name);
}

core::shader_program* yandere_shader_program::c_shader_program() const noexcept
{
	if(_id!=-1)
	{
		return _shader_programs_ptr->data()+_id;
	} else
	{
		assert(_shader_program_ptr!=nullptr);
		return _shader_program_ptr;
	}
}

//----------------------------------------------------------------------------------------------------------------
yandere_model::yandere_model(const std::vector<core::model>* models_ptr, const unsigned id)
: _models_ptr(models_ptr), _id(id)
{
}

yandere_model::yandere_model(const core::model* model_ptr)
: _model_ptr(model_ptr)
{
}

bool yandere_model::empty() const
{
	return c_model_ptr()->empty();
}

void yandere_model::draw(const core::object_data* obj_data) const
{	
	c_model_ptr()->draw(obj_data);
}

const core::model* yandere_model::c_model_ptr() const noexcept
{
	if(_id!=-1)
	{
		return _models_ptr->data()+_id;
	} else
	{
		assert(_model_ptr!=nullptr);
		return _model_ptr;
	}
}


yandere_texture::yandere_texture(const std::vector<core::texture>* textures_ptr, const unsigned id)
: _textures_ptr(textures_ptr), _id(id)
{
}

yandere_texture::yandere_texture(const core::texture* texture_ptr)
: _texture_ptr(texture_ptr)
{
}

void yandere_texture::set_current() const
{
	c_texture_ptr()->set_current();
}

int yandere_texture::width() const
{
	return c_texture_ptr()->width();
}

int yandere_texture::height() const
{
	return c_texture_ptr()->height();
}

const core::texture* yandere_texture::c_texture_ptr() const noexcept
{
	if(_id!=-1)
	{
		return _textures_ptr->data()+_id;
	} else
	{
		assert(_texture_ptr!=nullptr);
		return _texture_ptr;
	}
}

//----------------------------------------------------------------------------------------------------------------
yandere_resources::yandere_resources()
{
	load_default_models();
	load_default_textures();
}

yandere_resources::~yandere_resources()
{
	for(const auto& program : _shader_program_vec)
	{
		if(glIsProgram(program.program()))
		{
			glDeleteProgram(program.program());
		}
	}
}

std::map<std::string, unsigned> yandere_resources::load_shaders_from(std::string shaders_folder)
{
	std::filesystem::path shaders_fpath(shaders_folder);

	if(!std::filesystem::exists(shaders_fpath))
	{
		throw std::runtime_error("shader folder not found");
	}

	std::map<std::string, unsigned> shader_ids;
	for(const auto& file : std::filesystem::directory_iterator(shaders_fpath))
	{
		std::ifstream shader_src_file(file.path().string());
		std::string shader_str;

		std::string shader_filename = file.path().filename().string();

		if(shader_src_file.good())
		{
			shader_str = std::string(std::istreambuf_iterator(shader_src_file), {});

			shader_src_file.close();
		} else
		{
			std::cout << "error reading a shader file" << std::endl;
		}

		std::string c_extension = file.path().extension().string();

		core::shader_type c_type = c_extension==".fragment" ? core::shader_type::fragment : (c_extension==".vertex" ? core::shader_type::vertex : core::shader_type::geometry);
		shader_ids[shader_filename] = add_shader(core::shader(shader_str, c_type));
	}
	
	return shader_ids;
}

std::map<std::string, unsigned> yandere_resources::load_models_from(std::string models_folder)
{
	std::filesystem::path models_fpath(models_folder);

	if(!std::filesystem::exists(models_fpath))
	{
		throw std::runtime_error("models folder not found");
	}

	std::map<std::string, unsigned> model_ids;
	for(const auto& file : std::filesystem::directory_iterator(models_fpath))
	{
		std::string model_filename = file.path().filename().stem().string();

		model_ids[model_filename] = add_model(core::model(file.path().string()));
	}
	
	return model_ids;
}

void yandere_resources::load_default_models()
{
	std::vector<std::string> default_names = {"!dSQUARE", "!dCIRCLE", "!dTRIANGLE", "!dCUBE", "!dPYRAMID"};
	
	_model_vec.reserve(default_model::MODEL_LAST);
	for(int i = 0; i < default_model::MODEL_LAST; ++i)
	{
		_model_vec.emplace_back(default_names[i]);
	}
}

std::map<std::string, unsigned> yandere_resources::load_textures_from(std::string textures_folder)
{
	std::filesystem::path textures_fpath(textures_folder);

	if(!std::filesystem::exists(textures_fpath))
	{
		throw std::runtime_error("textures folder not found");
	}

	std::map<std::string, unsigned> texture_ids;
	for(const auto& file : std::filesystem::directory_iterator(textures_fpath))
	{
		std::string textureFilename = file.path().filename().stem().string();

		texture_ids[textureFilename] = add_texture(core::texture(file.path().string()));
	}
	
	return texture_ids;
}

void yandere_resources::load_default_textures()
{
	std::vector<std::string> default_names = {"!dSOLID"};
	
	_texture_vec.reserve(default_texture::TEXTURE_LAST);
	for(int i = 0; i < default_texture::TEXTURE_LAST; ++i)
	{
		_texture_vec.emplace_back(default_names[i]);
	}
}

yandere_model yandere_resources::create_model(const unsigned id) const noexcept
{
	return yandere_model(&_model_vec, id);
}

yandere_model yandere_resources::create_model(const core::model* model) const noexcept
{
	return yandere_model(model);
}
		
yandere_texture yandere_resources::create_texture(const unsigned id) const noexcept
{
	return yandere_texture(&_texture_vec, id);
}

yandere_texture yandere_resources::create_texture(const core::texture* texture) const noexcept
{
	return yandere_texture(texture);
}

unsigned yandere_resources::add_model(core::model& model)
{
	return add_model(core::model(model));
}

unsigned yandere_resources::add_model(core::model&& model)
{
	if(_empty_models.empty())
	{
		_model_vec.emplace_back(model);
		
		return _model_vec.size()-1;
	} else
	{
		auto c_iter = _empty_models.begin();
		
		unsigned inserted_id = *c_iter;
		_model_vec[inserted_id] = model;
	
		//doesnt keep order but way faster than vector::erase
		auto new_vec_end = _empty_models.end()-1;
		if(new_vec_end!=c_iter)
			*c_iter = *new_vec_end;
		_empty_models.pop_back();
		
		return inserted_id;
	}
}

void yandere_resources::remove_model(const unsigned model_id)
{
	_empty_models.push_back(model_id);
}

void yandere_resources::set_model(const unsigned model_id, core::model& model)
{
	return set_model(model_id, core::model(model));
}

void yandere_resources::set_model(const unsigned model_id, core::model&& model)
{
	_model_vec[model_id] = model;
}

core::model yandere_resources::model(const unsigned model_id) const noexcept
{
	return _model_vec.at(model_id);
}


unsigned yandere_resources::add_texture(core::texture& texture)
{
	return add_texture(core::texture(texture));
}

unsigned yandere_resources::add_texture(core::texture&& texture)
{
	if(_empty_textures.empty())
	{
		_texture_vec.emplace_back(texture);
		
		return _texture_vec.size()-1;
	} else
	{
		auto c_iter = _empty_textures.begin();
		
		unsigned inserted_id = *c_iter;
		_texture_vec[inserted_id] = texture;
	
		//doesnt keep order but way faster than vector::erase
		auto new_vec_end = _empty_textures.end()-1;
		if(new_vec_end!=c_iter)
			*c_iter = *new_vec_end;
		_empty_textures.pop_back();
		
		return inserted_id;
	}
}

void yandere_resources::remove_texture(const unsigned texture_id)
{
	_empty_textures.push_back(texture_id);
}

void yandere_resources::set_texture(const unsigned texture_id, core::texture& texture)
{
	return set_texture(texture_id, core::texture(texture));
}

void yandere_resources::set_texture(const unsigned texture_id, core::texture&& texture)
{
	_texture_vec[texture_id] = texture;
}

core::texture yandere_resources::texture(const unsigned texture_id) const noexcept
{
	return _texture_vec.at(texture_id);
}

unsigned yandere_resources::add_shader(core::shader& shader)
{
	return add_shader(core::shader(shader));
}

unsigned yandere_resources::add_shader(core::shader&& shader)
{
	_shader_vec.emplace_back(shader);
	
	return _shader_vec.size()-1;
}

yandere_shader_program yandere_resources::create_shader_program(const yandere_controller* ctl, std::vector<unsigned> shader_id_vec)
{
	assert(!_shader_vec.empty());

	int success;

	const char* vertex_shader_src = nullptr;
	const char* fragment_shader_src = nullptr;
	const char* geometry_shader_src = nullptr;

	for(auto& shader_id : shader_id_vec)
	{
		if(_shader_vec.size()<=shader_id)
			throw std::runtime_error("shader file doesnt exist");
	
		if(_shader_vec[shader_id].type()==core::shader_type::fragment)
		{
			fragment_shader_src = _shader_vec[shader_id].text().c_str();
		} else if(_shader_vec[shader_id].type()==core::shader_type::vertex)
		{
			vertex_shader_src = _shader_vec[shader_id].text().c_str();
		} else
		{
			geometry_shader_src = _shader_vec[shader_id].text().c_str();
		}
	}

	unsigned vertex_shader_id;
	unsigned fragment_shader_id;
	unsigned geometry_shader_id;

	unsigned c_program = glCreateProgram();

	if(vertex_shader_src!=nullptr)
	{
		vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertex_shader_id, 1, &vertex_shader_src, NULL);
		glCompileShader(vertex_shader_id);

		glGetShaderiv(vertex_shader_id, GL_COMPILE_STATUS, &success);
		if(!success) output_error(vertex_shader_id);

		glAttachShader(c_program, vertex_shader_id);
	}
	
	if(fragment_shader_src!=nullptr)
	{
		fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragment_shader_id, 1, &fragment_shader_src, NULL);
		glCompileShader(fragment_shader_id);

		glGetShaderiv(fragment_shader_id, GL_COMPILE_STATUS, &success);
		if(!success) output_error(fragment_shader_id);

		glAttachShader(c_program, fragment_shader_id);
	}

	if(geometry_shader_src!=nullptr)
	{
		geometry_shader_id = glCreateShader(GL_GEOMETRY_SHADER);
		glShaderSource(geometry_shader_id, 1, &fragment_shader_src, NULL);
		glCompileShader(geometry_shader_id);

		glGetShaderiv(geometry_shader_id, GL_COMPILE_STATUS, &success);
		if(!success) output_error(geometry_shader_id);

		glAttachShader(c_program, geometry_shader_id);
	}

	glLinkProgram(c_program);

	glGetProgramiv(c_program, GL_LINK_STATUS, &success);
	if(!success) output_error(c_program, true);
	
	if(vertex_shader_src!=nullptr) glDeleteShader(vertex_shader_id);
	if(fragment_shader_src!=nullptr) glDeleteShader(fragment_shader_id);
	if(geometry_shader_src!=nullptr) glDeleteShader(geometry_shader_id);
	
	return shader_program(ctl, add_shader_program(core::shader_program(c_program)));
}

void yandere_resources::output_error(unsigned object_id, bool is_program)
{
	char error_info[512];
	if(is_program)
	{
		glGetProgramInfoLog(object_id, 512, NULL, error_info);
	} else
	{
		glGetShaderInfoLog(object_id, 512, NULL, error_info);
	}
	
	std::stringstream sstream;
	
	sstream << "LOAD ERROR: " << error_info;
	
	throw std::runtime_error(sstream.str());
}

yandere_shader_program yandere_resources::shader_program(const yandere_controller* ctl, const unsigned program_id)
{
	return yandere_shader_program(ctl, &_shader_program_vec, program_id);
}

unsigned yandere_resources::add_shader_program(core::shader_program& program)
{
	return add_shader_program(core::shader_program(program));
}

unsigned yandere_resources::add_shader_program(core::shader_program&& program)
{
	_shader_program_vec.emplace_back(program);
	
	return _shader_program_vec.size()-1;
}

//----------------------------------------------------------------------------------------------------------------
yandere_controller::yandere_controller(bool stencil, bool antialiasing, bool culling)
: _initializer(stencil, antialiasing, culling)
{
	config_options(stencil, antialiasing, culling);
}

yandere_controller::~yandere_controller()
{
	//c libraries :/
	for(auto& [name, face] : _fonts_map)
	{
		FT_Done_Face(face);
	}
	
	if(_font_init)
	{
		FT_Done_FreeType(_ft_lib);
	}
}

void yandere_controller::set_draw_camera(yandere_camera* camera)
{
	_main_camera = camera;
}

const glm::mat4* yandere_controller::view_matrix_ptr() const noexcept
{
	assert(_main_camera!=nullptr);
	return _main_camera->view_matrix_ptr();
}

const glm::mat4* yandere_controller::projection_matrix_ptr() const noexcept
{
	assert(_main_camera!=nullptr);
	return _main_camera->projection_matrix_ptr();
}

void yandere_controller::config_options(bool stencil, bool antialiasing, bool culling)
{
	if(antialiasing)
	{
		glEnable(GL_MULTISAMPLE);
	} else
	{
		glDisable(GL_MULTISAMPLE);
	}

	if(culling)
	{
		glEnable(GL_CULL_FACE);
		glFrontFace(GL_CCW);
	} else
	{
		glDisable(GL_CULL_FACE);
	}

	if(stencil)
	{
		glEnable(GL_STENCIL_TEST);
	} else
	{
		glDisable(GL_STENCIL_TEST);
	}
}

void yandere_controller::load_font(const std::string font_path)
{
	const std::filesystem::path font_fpath(font_path);
	const std::string font_string = font_fpath.string();
	
	if(!std::filesystem::exists(font_fpath))
		throw std::runtime_error("font file not found");
	
	if(!_font_init)
	{	
		if(FT_Error err = FT_Init_FreeType(&_ft_lib); err!=FT_Err_Ok)
			throw std::runtime_error("[ERROR_"+std::to_string(err)+"] couldnt load freetype lib");

		_font_init = true;
	}
	
	FT_Face face;
	if(FT_Error err = FT_New_Face(_ft_lib, font_string.c_str(), 0, &face); err!=FT_Err_Ok)
		throw std::runtime_error("[ERROR_"+std::to_string(err)+"] loading font at: "+font_fpath.string());
	
	_fonts_map.insert({font_fpath.filename().stem().string(), std::move(face)});
}

FT_Face yandere_controller::font(const std::string name) const noexcept
{
	return _fonts_map.at(name);
}

yandere_resources& yandere_controller::resources()
{
	return _resources;
}

yandere_shader_program yandere_controller::create_shader_program(std::vector<unsigned> shader_id_vec)
{
	return _resources.create_shader_program(this, std::move(shader_id_vec));
}

void GLAPIENTRY yandere_controller::debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
	std::cout << "opengl error " << type << ": " << message << std::endl;
}

//---------------------------------------------------------------------------------------------------------------
yandere_object::yandere_object(const yandere_shader_program shader,
const yandere_model model,
const yandere_texture texture,
const yan_transforms transform,
const yan_color color)
 : _shader(shader), _model(model), _texture(texture), _transform(transform), _color(color)
{
	mats_update();
}

void yandere_object::set_position(yan_position position)
{
	_transform.position = position;
	
	mats_update();
}

void yandere_object::translate(yan_position delta)
{
	_transform.position.x += delta.x;
	_transform.position.y += delta.y;
	_transform.position.z += delta.z;
		
	mats_update();
}

void yandere_object::set_scale(yan_scale scale)
{
	_transform.scale = scale;
		
	mats_update();
}

void yandere_object::set_rotation(float rotation)
{
	_transform.rotation = rotation;

	mats_update();
}

void yandere_object::rotate(float rotationDelta)
{
	_transform.rotation += rotationDelta;

	mats_update();
}

void yandere_object::set_rotation_axis(yan_position axis)
{
	_transform.axis = axis;

	mats_update();
}

void yandere_object::set_color(yan_color color)
{
	_color = _obj_data.color = color;
}

void yandere_object::draw_update() const noexcept
{
	if(_model.empty())
		return;
	
	_shader.set_current();
	_texture.set_current();
	
	_model.draw(&_obj_data);
}

void yandere_object::mats_update()
{
	glm::mat4 temp_mat = calculate_matrix(_transform);

	_obj_data.color = _color;

	for(int m = 0; m<4; ++m)
	{
		for(int v = 0; v<4; ++v)
		{
			_obj_data.matrix[m*4+v] = temp_mat[m][v];
		}
	}
}

glm::mat4 yandere_object::calculate_matrix(yan_transforms transforms)
{
	glm::mat4 model_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(transforms.position.x, transforms.position.y, transforms.position.z));
	model_matrix = glm::rotate(model_matrix, transforms.rotation, glm::vec3(transforms.axis.x, transforms.axis.y, transforms.axis.z));
	model_matrix = glm::scale(model_matrix, glm::vec3(transforms.scale.x, transforms.scale.y, transforms.scale.z));
	return model_matrix;
}

//-------------------------------------------------------------------------------------------------------------------
yandere_line::yandere_line(yandere_resources& resources, const yandere_shader_program shader,
const yan_position point0, const yan_position point1,
const float width, const yan_color color)
 : _point0(point0), _point1(point1), _width(width), yandere_object(shader,
resources.create_model(default_model::square), resources.create_texture(default_texture::solid), {}, color)
{
	calculate_variables();
}

void yandere_line::set_positions(yan_position point0, yan_position point1)
{
	_point0 = point0;
	_point1 = point1;

	calculate_variables();
}

void yandere_line::set_rotations(float rotation)
{
	_transform.rotation = rotation;

	calculate_variables();
}

void yandere_line::set_widths(float width)
{
	_width = width;

	_transform.scale.y = _width;

	mats_update();
}

void yandere_line::calculate_variables()
{
	float line_width = (_point1.x-_point0.x);
	float line_height = (_point1.y-_point0.y);

	float line_length = std::sqrt((line_width*line_width)+(line_height*line_height))/2;

	float rot = std::atan2(line_height, line_width);

	float angle_offset_x = (1-std::cos(rot))*line_length;
	float angle_offset_y = (std::sin(rot))*line_length;

	yan_position pos = {_point0.x+line_length-angle_offset_x, _point0.y+angle_offset_y, _point0.z};

	_transform = {pos, line_length, _width, 1, rot, 0, 0, 1};

	mats_update();
}


yandere_text::yandere_text(const yandere_shader_program shader, 
const FT_Face font, const std::string text,
const int size, const float x, const float y)
: _shader(shader), _text(text), _size(size), _x(x), _y(y)
{
	std::vector<std::vector<uint8_t>> letter_pixels;
	
	int max_width = 0;
	int height = 0;

	FT_Set_Pixel_Sizes(font, 0, size);
	
	letter_pixels.reserve(_load_chars);
	
	for(int i = 0; i < _load_chars; ++i)
	{
		FT_Load_Char(font, i, FT_LOAD_RENDER);
		
		if(font->glyph->bitmap.width>max_width)
		{
			max_width = font->glyph->bitmap.width;
		}
		if(font->glyph->bitmap.rows>height)
		{
			height = font->glyph->bitmap.rows;
		}
		
		_letters[i] = {static_cast<float>(font->glyph->bitmap.width), 
		static_cast<float>(font->glyph->bitmap.rows),
		static_cast<float>(font->glyph->bitmap_left),
		static_cast<float>(font->glyph->bitmap_top), 
		static_cast<float>(font->glyph->advance.x)};
		
		
		int pixels_amount = font->glyph->bitmap.width * font->glyph->bitmap.rows;
		
		uint8_t* bitmap_arr = font->glyph->bitmap.buffer;
		
		std::vector<uint8_t> l_pixels;
		l_pixels.reserve(pixels_amount);
		for(int l = 0; l < pixels_amount; ++l)
		{
			l_pixels.emplace_back(bitmap_arr[l]);
		}
		
		letter_pixels.emplace_back(l_pixels);
	}
	
	const int width = max_width*_load_chars;
	
	std::vector<uint8_t> tex_data(width*height, 0);
	
	_letter_models.reserve(_load_chars);
	for(int c = 0; c < _load_chars; ++c)
	{
		const int x_start = max_width*c;
		
		const int letter_width = _letters[c].width;
		const int letter_total = letter_width*_letters[c].height;
		for(int l = 0; l < letter_total; ++l)
		{
			int letter_x = l%letter_width;
			int letter_y = l/letter_width;
			tex_data[x_start+letter_x+letter_y*width] = letter_pixels[c][l];
		}
		
		const float x_min = x_start/static_cast<float>(width);
		
		const float x_max = x_min+_letters[c].width/static_cast<float>(width);
		const float y_max = 1.0f-_letters[c].height/static_cast<float>(height);
		
		
		core::model c_model;
		c_model.vertices_insert({
		-1, 1, 0,	 x_min, 1,
		1, 1, 0,	  x_max, 1,
		1, -1, 0,	 x_max, y_max,
		-1, -1, 0,	x_min, y_max});
		
		c_model.indices_insert({
		0, 2, 1,
		2, 0, 3});
		
		_letter_models.emplace_back(c_model);
		
		
		_letters[c].origin_y *= 2;
	}

	yandere_image img;
	img.width = width;
	img.height = height;
	img.bpp = 1;
	img.image = tex_data;
	
	_texture = core::texture(img);
	
	set_text(text);
}

void yandere_text::draw_update() const noexcept
{
	for(const auto& obj : _letter_objs)
	{
		obj.draw_update();
	}
}

void yandere_text::set_text(const std::string text)
{
	_text = text;

	_letter_objs.clear();

	if(text.length()==0)
		return;

	float last_x = _x;
	
	_text_height = 0;
	_text_width = 0;

	_letter_objs.reserve(text.length());
	for(int i = 0; i < text.length(); ++i)
	{
		const int letter_index = text[i];
	
		core::letter_data letter = _letters[letter_index];
		
		if(letter.height>_text_height)
		{
			_text_height = letter.height;
		}
		
		yan_position letter_pos = yan_position{
		last_x + letter.width,
		_y - letter.height + letter.origin_y
		};
		
		yan_transforms letter_transform = yan_transforms{{letter_pos.x, letter_pos.y, letter_pos.z},
		{letter.width, letter.height, 1}};
	

		last_x += letter.width+letter.origin_x+letter.hdist/64.0f;
		_text_width += letter.width+letter.hdist/64.0f;
		if(i==text.length()-1)
		{
			_text_width -= letter.hdist/64.0f;
		}
		
		_letter_objs.emplace_back(_shader, yandere_model(&_letter_models, letter_index), yandere_texture(&_texture), letter_transform);
	}
}

void yandere_text::set_position(const float x, const float y)
{
	float x_diff = x-(_x);
	float y_diff = y-(_y);

	_x = x;
	_y = y;
	
	for(auto& obj : _letter_objs)
	{
		obj.translate({x_diff, y_diff, 0});
	}
}

std::array<float, 2> yandere_text::position() const noexcept
{
	return {_x, _y};
}

float yandere_text::text_width() const noexcept
{
	return _text_width;
}

float yandere_text::text_height() const noexcept
{
	return _text_height*2;
}

std::string yandere_text::text() const noexcept
{
	return _text;
}


yandere_panel::yandere_panel(yandere_gui* gui, float x0, float y0, float x1, float y1)
: _gui(gui), _x0(x0), _y0(y0), _x1(x1), _y1(y1)
{
	
}

unsigned yandere_panel::add_text(object_position<yandere_text> text)
{
	
}

yandere_panel yandere_gui::add_panel(float x0, float y0, float x1, float y1)
{
	return yandere_panel(this, x0, y0, x1, y1);
}
