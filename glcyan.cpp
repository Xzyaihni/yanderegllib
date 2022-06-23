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

using namespace yangl;

//----------------------------------------------------------------------------------------------------------------
controller::controller(const options opt)
: _initializer()
{
	config_options(opt);
}

void controller::config_options(const options opt)
{
	if(opt.antialiasing)
	{
		glEnable(GL_MULTISAMPLE);
	} else
	{
		glDisable(GL_MULTISAMPLE);
	}

	if(opt.culling)
	{
		glEnable(GL_CULL_FACE);
		glFrontFace(GL_CCW);
	} else
	{
		glDisable(GL_CULL_FACE);
	}

	if(opt.stencil)
	{
		glEnable(GL_STENCIL_TEST);
	} else
	{
		glDisable(GL_STENCIL_TEST);
	}

	if(opt.alpha)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	} else
	{
		glDisable(GL_BLEND);
	}
}

const font_data* controller::load_font(const std::filesystem::path font_path, const int font_size)
{
	if(!std::filesystem::exists(font_path))
		throw std::runtime_error("font file not found");
	
	FT_Library ft_lib;
	if(FT_Error err = FT_Init_FreeType(&ft_lib); err!=FT_Err_Ok)
		throw std::runtime_error("[ERROR_"+std::to_string(err)+"] couldnt load freetype lib");
	
	const std::string font_string = font_path.string();

	FT_Face face;
	if(FT_Error err = FT_New_Face(ft_lib, font_string.c_str(), 0, &face); err!=FT_Err_Ok)
		throw std::runtime_error("[ERROR_"+std::to_string(err)+"] loading font at: "+font_string);

	const font_data& c_font = _loaded_fonts.emplace_back(create_font(face, font_size));

	FT_Done_Face(face);
	FT_Done_FreeType(ft_lib);

	return &c_font;
}

std::map<std::string, core::shader>
controller::load_shaders_from(const std::filesystem::path shaders_folder)
{
	if(!std::filesystem::exists(shaders_folder))
		throw std::runtime_error("shader folder not found");

	std::map<std::string, core::shader> shaders_map;
	for(const auto& file : std::filesystem::directory_iterator(shaders_folder))
	{
		std::ifstream shader_src_file(file.path().string());

		if(!shader_src_file.good())
			throw std::runtime_error(std::string("error reading a shader file")
				+ file.path().string());

		const std::string shader_str = std::string(std::istreambuf_iterator(shader_src_file), {});

		const std::string c_extension = file.path().extension().string();

		const std::string shader_name = file.path().filename().string();

		shader_type c_type;
		if(c_extension==".fragment")
		{
			c_type = shader_type::fragment;
		} else if(c_extension==".vertex")
		{
			c_type = shader_type::vertex;
		} else if(c_extension==".geometry")
		{
			c_type = shader_type::geometry;
		} else
		{
			throw std::runtime_error(std::string("invalid shader type: ")+c_extension);
		}

		shaders_map[shader_name] = core::shader(shader_str, c_type);
	}

	return shaders_map;
}

std::map<std::string, core::model>
controller::load_models_from(const std::filesystem::path models_folder)
{
	if(!std::filesystem::exists(models_folder))
		throw std::runtime_error("models folder not found");

	std::map<std::string, core::model> models_map;
	for(const auto& file : std::filesystem::directory_iterator(models_folder))
	{
		const std::string model_name = file.path().filename().stem().string();

		models_map[model_name] = core::model(file.path().string());
	}

	return models_map;
}

std::map<std::string, core::texture>
controller::load_textures_from(const std::filesystem::path textures_folder)
{
	if(!std::filesystem::exists(textures_folder))
		throw std::runtime_error("textures folder not found");

	std::map<std::string, core::texture> textures_map;
	for(const auto& file : std::filesystem::directory_iterator(textures_folder))
	{
		const std::string texture_name = file.path().filename().stem().string();

		textures_map[texture_name] = core::texture(file.path().string());
	}

	return textures_map;
}

font_data controller::create_font(const FT_Face font, const int size) noexcept
{
	const raw_font_data c_font = load_raw_font(font, size);

	const int atlas_width = c_font.max_width*yconst::load_chars;
	std::vector<uint8_t> fonts_atlas(atlas_width*c_font.max_height);

	std::vector<core::model> letter_models;

	letter_models.reserve(yconst::load_chars);
	for(int i = 0; i < yconst::load_chars; ++i)
	{
		const std::vector<uint8_t>& c_pixels = c_font.images[i].pixels;


		const int x_start = c_font.max_width*i;

		const int letter_width = c_font.images[i].width;
		const int letter_height = letter_width!=0?c_pixels.size()/letter_width:0;

		const int width = c_font.max_width*yconst::load_chars;
		const float x_min = static_cast<float>(x_start)/width;
		const float x_max = x_min+letter_width/static_cast<float>(width);

		const float scaled_width = letter_width/static_cast<float>(c_font.max_width);
		const float scaled_height = letter_height/static_cast<float>(c_font.max_height);

		const float y_max = 1.0f-scaled_height;

		letter_models.emplace_back(
			std::vector<float>{
				0, scaled_height, 0,			x_min, 1,
				scaled_width, scaled_height, 0, x_max, 1,
				scaled_width, 0, 0,				x_max, y_max,
				0, 0, 0,						x_min, y_max},
			std::vector<unsigned>{0, 2, 1,
				2, 0, 3});

		for(int j = 0; j < c_pixels.size(); ++j)
		{
			const int x = j%letter_width;
			const int y = j/letter_width;
			fonts_atlas[y*atlas_width+x+c_font.max_width*i] = c_pixels[j];
		}
	}

	const yconv::image fonts_image(c_font.max_width*yconst::load_chars,
		c_font.max_height, 1, fonts_atlas);

	return font_data{c_font.letters, core::texture(fonts_image), letter_models};
}

raw_font_data controller::load_raw_font(const FT_Face font, const int size) noexcept
{
	std::array<letter_data, yconst::load_chars> letters;
	std::array<raw_font_data::letter_pixels, yconst::load_chars> letter_pixels;

	int max_width = 0;
	int max_height = 0;

	FT_Set_Pixel_Sizes(font, 0, size);
	for(int i = 0; i < yconst::load_chars; ++i)
	{
		FT_Load_Char(font, i, FT_LOAD_RENDER);

		const int c_width = font->glyph->bitmap.width;
		const int c_height = font->glyph->bitmap.rows;

		if(c_width>max_width)
		{
			max_width = c_width;
		}
		if(c_height>max_height)
		{
			max_height = c_height;
		}

		letters[i] = {
			static_cast<float>(c_width)/max_width,
			static_cast<float>(c_height)/max_height,
			static_cast<float>(font->glyph->bitmap_left)/max_width,
			static_cast<float>(font->glyph->bitmap_top)/max_height,
			static_cast<float>(font->glyph->advance.x)/max_width};


		const uint8_t* bitmap_arr = font->glyph->bitmap.buffer;

		const int pixels_amount = c_width*c_height;
		letter_pixels[i] = raw_font_data::letter_pixels{
			c_width, std::vector<uint8_t>(bitmap_arr, bitmap_arr+pixels_amount)};
	}

	return raw_font_data{letters, letter_pixels, max_width, max_height};
}

//---------------------------------------------------------------------------------------------------------------
generic_object::generic_object()
{
}

generic_object::generic_object(const camera* cam,
	const generic_shader* shader,
	const generic_model* model,
	const generic_texture* texture)
: _camera(cam), _shader(shader), _model(model), _texture(texture)
{
	set_matrix();
}

void generic_object::set_transform(const yvec3 position, const yvec3 scale, const yvec3 axis, const float rotation)
{
	_position = position;
	_scale = scale;
	_axis = axis;
	_rotation = rotation;

	set_matrix();
}

yvec3 generic_object::position() const noexcept
{
	return _position;
}

void generic_object::set_position(const yvec3 position)
{
	_position = position;
	
	set_matrix();
}

void generic_object::translate(const yvec3 delta)
{
	_position.x += delta.x;
	_position.y += delta.y;
	_position.z += delta.z;
		
	set_matrix();
}

yvec3 generic_object::scale() const noexcept
{
	return _scale;
}

void generic_object::set_scale(const yvec3 scale)
{
	_scale = scale;
		
	set_matrix();
}

float generic_object::rotation() const noexcept
{
	return _rotation;
}

void generic_object::set_rotation(const float rotation)
{
	_rotation = rotation;

	set_matrix();
}

void generic_object::rotate(const float delta)
{
	_rotation += delta;

	set_matrix();
}

yvec3 generic_object::axis() const noexcept
{
	return _axis;
}

void generic_object::set_rotation_axis(const yvec3 axis)
{
	_axis = axis;

	set_matrix();
}

void generic_object::draw() const noexcept
{
	assert(_model!=nullptr);
	assert(_texture!=nullptr);
	assert(_shader!=nullptr);
	assert(_camera!=nullptr);
	if(_model->empty())
		return;
	
	_shader->apply_uniforms(_camera, _transform);
	_texture->set_current();

	_model->draw();
}

void generic_object::set_matrix() noexcept
{
	const glm::mat4 translated_m = glm::translate(glm::mat4(1.0f), glm::vec3(_position.x, _position.y, _position.z));
	const glm::mat4 rotated_m = glm::rotate(translated_m, _rotation, glm::vec3(_axis.x, _axis.y, _axis.z));
	_transform = glm::scale(rotated_m, glm::vec3(_scale.x, _scale.y, _scale.z));
}

//-------------------------------------------------------------------------------------------------------------------
line_object::line_object(const camera* cam,
	const generic_shader* shader,
	const yvec3 point0, const yvec3 point1,
	const float width)
: _point0(point0), _point1(point1), _width(width),
generic_object(cam, shader,
default_assets.model(default_model::plane),
default_assets.texture(default_texture::solid))
{
	calculate_variables();
}

void line_object::set_positions(const yvec3 point0, const yvec3 point1)
{
	_point0 = point0;
	_point1 = point1;

	calculate_variables();
}

void line_object::set_rotations(const float rotation)
{
	_rotation = rotation;

	calculate_variables();
}

void line_object::set_widths(const float width)
{
	_width = width;

	_scale.y = _width;

	set_matrix();
}

void line_object::calculate_variables()
{
	const float line_width = _point1.x-_point0.x;
	const float line_height = _point1.y-_point0.y;

	const float line_length = std::sqrt((line_width*line_width)+(line_height*line_height))/2;

	const float rot = std::atan2(line_height, line_width);

	const float angle_offset_x = (1-std::cos(rot))*line_length;
	const float angle_offset_y = (std::sin(rot))*line_length;

	const yvec3 pos = {_point0.x+line_length-angle_offset_x, _point0.y+angle_offset_y, _point0.z};

	_position = pos;
	_scale = {line_length, _width, 1};
	_axis = {0, 0, 1};
	_rotation = rot;

	set_matrix();
}


text_object::text_object(const camera* cam,
	const generic_shader* shader,
	const font_data* font, const std::string text)
: _camera(cam), _shader(shader), _font(font), _text(text)
{
	set_text(_text);
}

text_object::text_object(const text_object& other)
: _camera(other._camera), _shader(other._shader),
_font(other._font),
_text(other._text),
_position(other._position), _scale(other._scale)
{
	set_text(_text);
}

text_object::text_object(text_object&& other) noexcept
: _letters(other._letters),
_camera(other._camera), _shader(other._shader),
_font(other._font),
_text(other._text),
_position(other._position), _scale(other._scale)
{
}

text_object& text_object::operator=(const text_object& other)
{
	if(this!=&other)
	{
		_camera = other._camera;
		_shader = other._shader;
		_font = other._font;
		_text = other._text;
		_position = other._position;
		_scale = other._scale;

		set_text(_text);
	}
	return *this;
}

text_object& text_object::operator=(text_object&& other) noexcept
{
	if(this!=&other)
	{
		_letters = std::move(other._letters);
		_camera = other._camera;
		_shader = other._shader;
		_font = other._font;
		_text = std::move(other._text);
		_position = std::move(other._position);
		_scale = std::move(other._scale);
	}
	return *this;
}

void text_object::draw() const noexcept
{
	for(const auto& obj : _letters)
	{
		obj.draw();
	}
}

void text_object::set_text(const std::string text)
{
	_text = text;
	_letters.clear();

	_text_width = 0;
	_text_height = 0;

	if(text.length()==0)
		return;

	float last_x = _position.x;

	_letters.reserve(text.length());
	for(int i = 0; i < text.length(); ++i)
	{
		const int letter_index = text[i];
		assert(letter_index<yconst::load_chars);

		const letter_data letter = _font->letters[letter_index];

		const letter_data scaled_letter =
		{letter.width*_scale.x,
		letter.height*_scale.y,
		letter.origin_x*_scale.x,
		letter.origin_y*_scale.y,
		letter.hdist/64.0f*_scale.x};

		if(letter.height>_text_height)
			_text_height = letter.height;

		const yvec3 c_letter_pos{last_x + scaled_letter.origin_x,
			_position.y - scaled_letter.height + scaled_letter.origin_y, _position.z};

		last_x += scaled_letter.hdist;
		//idk why i have to divide it by 64 twice
		_text_width += letter.hdist/64;
		
		_letters.emplace_back(create_letter(letter_index, c_letter_pos));
	}

	const letter_data first_letter = _font->letters[0];
	const letter_data last_letter = _font->letters[text.length()-1];
	_text_width -= first_letter.origin_x;
	_text_width -= last_letter.origin_x + last_letter.width;
}

std::string text_object::text() const noexcept
{
	return _text;
}

float text_object::text_width() const noexcept
{
	return _text_width;
}

float text_object::text_height() const noexcept
{
	return _text_height;
}

void text_object::set_position(const yvec3 position)
{
	for(auto& obj : _letters)
	{
		obj.translate({position.x-_position.x,
			position.y-_position.y,
			position.z-_position.z});
	}

	_position = position;
}

yvec3 text_object::position() const noexcept
{
	return _position;
}

void text_object::set_scale(const yvec2 scale)
{
	_scale = scale;

	set_text(_text);
}

yvec2 text_object::scale() const noexcept
{
	return _scale;
}

generic_object text_object::create_letter(const int code, const yvec3 position) const noexcept
{
	generic_object c_object(_camera, _shader,
		&_font->models[code], &_font->texture);

	c_object.set_position(position);
	c_object.set_scale({_scale.x, _scale.y, 1});

	return c_object;
}
