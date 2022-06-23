#include "ygui.h"

#include <iostream>

using namespace yangl;

gui::panel::panel(const camera* cam, const generic_shader* shader,
	const yvec2 corner0, const yvec2 corner1)
: _camera(cam), _shader(shader)
{
	if(corner1.x<corner0.x)
	{
		_corner0.x = corner1.x;
		_corner1.x = corner0.x;
	} else
	{
		_corner0.x = corner0.x;
		_corner1.x = corner1.x;
	}

	if(corner1.y<corner0.y)
	{
		_corner0.y = corner1.y;
		_corner1.y = corner0.y;
	} else
	{
		_corner0.y = corner0.y;
		_corner1.y = corner1.y;
	}
}

void gui::panel::resize(const int parent_width, const int parent_height)
{
	_parent_width = parent_width;
	_parent_height = parent_height;

	update();
}

void gui::panel::draw_update() noexcept
{
	update();
	draw();
}

void gui::panel::update()
{
	assert(_parent_width!=0 && _parent_height!=0);

	for(auto& text_obj : _texts_list)
	{
		const object_info& c_info = text_obj.info;
		const yvec2 c_obj_size = {text_obj.object.text_width(), text_obj.object.text_height()};

		const yvec2 c_scale = calculate_scale(c_info, c_obj_size);
		text_obj.object.set_scale(c_scale);

		const yvec2 c_pos = calculate_position(c_info, c_obj_size, c_scale);
		text_obj.object.set_position({c_pos.x, c_pos.y, c_info.pos.z});
	}

	for(auto& gen_obj : _gui_objects_list)
	{
		const object_info& c_info = gen_obj.info;
		const yvec2 c_obj_size = {gen_obj.object.scale().x, gen_obj.object.scale().x};

		const yvec2 c_scale = calculate_scale(c_info, c_obj_size);
		gen_obj.object.set_scale({c_scale.x, c_scale.y, 1});

		const yvec2 c_pos = calculate_position(c_info, c_obj_size, c_scale);
		gen_obj.object.set_position({c_pos.x, c_pos.y, c_info.pos.z});
	}
}

void gui::panel::draw() const noexcept
{
	assert(_parent_width!=0 && _parent_height!=0);

	if(_visible)
	{
		for(const auto& text_obj : _texts_list)
		{
			text_obj.object.draw();
		}

		for(const auto& gen_obj : _gui_objects_list)
		{
			gen_obj.object.draw();
		}
	}
}

void gui::panel::set_visible(const bool state) noexcept
{
	_visible = state;
}

bool gui::panel::visible() const noexcept
{
	return _visible;
}

gui::store_object<text_object>&
gui::panel::add_text(const object_info info, const font_data* font, const std::string text)
{
	const store_object<text_object> obj{
		info,
		text_object(_camera, _shader, font, text)};


	auto& c_ref = _texts_list.emplace_back(obj);
	update();

	return c_ref;
}

gui::store_object<generic_object>&
gui::panel::add_object(const object_info info, const core::texture texture)
{
	const store_object<generic_object> obj{
		info,
		generic_object(_camera, _shader,
			default_assets.model(default_model::plane),
			default_assets.texture(default_texture::solid))};

	auto& c_ref = _gui_objects_list.emplace_back(obj);
	update();

	return c_ref;
}

yvec2 gui::panel::calculate_scale(const object_info info,
	const yvec2 size) const noexcept
{
	const float panel_width = (_corner1.x-_corner0.x)*_parent_width;
	const float panel_height = (_corner1.y-_corner0.y)*_parent_height;


	const float c_width = panel_width/size.x;
	const float c_height = panel_height/size.y;

	const float scale_x = info.scale.x*c_width;
	const float scale_y = info.scale.y*c_height;

	const float min_size = std::min(scale_x, scale_y);

	return {min_size, min_size};
}

yvec2 gui::panel::calculate_position(const object_info info,
	const yvec2 size, const yvec2 scale) const noexcept
{
	const float panel_width = _corner1.x-_corner0.x;
	const float panel_height = _corner1.y-_corner0.y;


	const yvec2 ratio =
		{(size.x*scale.x)/(panel_width*_parent_width),
		(size.y*scale.y)/(panel_height*_parent_height)};

	const float position_x =
		(_corner0.x + panel_width*info.pos.x
		- info.origin.x*ratio.x)*_parent_width;

	const float position_y =
		(_corner0.y + panel_height*info.pos.y
		- info.origin.y*ratio.y)*_parent_height;

	return {position_x, position_y};
}

gui::controller::camera_storage::camera_storage()
: default_camera({0, 0, 1}, {0, 0, 0}), ptr(&default_camera)
{
}

gui::controller::camera_storage::camera_storage(const int width, const int height)
: default_camera({0, 0, 1}, {0, 0, 0}), ptr(&default_camera)
{
	resize(width, height);
}

gui::controller::camera_storage::camera_storage(const camera_storage& other)
: default_camera(other.default_camera),
ptr(other.ptr==&other.default_camera?&default_camera:other.ptr)
{
}

gui::controller::camera_storage::camera_storage(camera_storage&& other) noexcept
: default_camera(std::move(other.default_camera)),
ptr(other.ptr==&other.default_camera?&default_camera:other.ptr)
{
}

gui::controller::camera_storage& gui::controller::camera_storage::operator=(const camera_storage& other)
{
	if(this!=&other)
	{
		default_camera = other.default_camera;

		ptr = other.ptr==&other.default_camera?&default_camera:other.ptr;
	}
	return *this;
}

gui::controller::camera_storage& gui::controller::camera_storage::operator=(camera_storage&& other) noexcept
{
	if(this!=&other)
	{
		default_camera = std::move(other.default_camera);

		ptr = other.ptr==&other.default_camera?&default_camera:other.ptr;
	}
	return *this;
}

void gui::controller::camera_storage::resize(const int width, const int height)
{
	default_camera.create_projection({
		0,
		static_cast<float>(width),
		0,
		static_cast<float>(height)},
		{0, _max_depth});
}

void gui::controller::camera_storage::set_camera(const camera* cam)
{
	if(cam!=nullptr)
		ptr = cam;
	else
		ptr = &default_camera;
}

gui::controller::controller(const int width, const int height)
: _shader(default_assets.shader(default_shader::gui)), _camera(width, height), _empty(false)
{
	resize(width, height);
}

void gui::controller::resize(const int width, const int height)
{
	_width = width;
	_height = height;

	_camera.resize(width, height);

	for(auto& c_panel : _panels_list)
	{
		c_panel.resize(_width, _height);
	}
}

void gui::controller::draw() const noexcept
{
	assert(!_empty);

	for(const auto& c_panel : _panels_list)
	{
		c_panel.draw();
	}
}

gui::panel& gui::controller::add_panel(const yvec2 corner0, const yvec2 corner1)
{
	panel& c_ref = _panels_list.emplace_back(_camera.ptr, _shader, corner0, corner1);
	c_ref.resize(_width, _height);

	return c_ref;
}

void gui::controller::set_camera(const camera* cam)
{
	_camera.set_camera(cam);
}

void gui::controller::set_shader(const generic_shader* shader)
{
	_shader = shader;
}
