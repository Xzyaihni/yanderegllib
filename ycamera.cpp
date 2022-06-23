#include "ycamera.h"

using namespace yangl;

camera::camera(const yvec3 position, const yvec3 look_position) : _projection_matrix(glm::mat4(1.0f))
{
	_camera_position = glm::vec3(position.x, position.y, position.z);
	_camera_direction = glm::normalize(glm::vec3(look_position.x, look_position.y, look_position.z)-_camera_position);

	calculate_view();
}

void camera::create_projection(const yvec4 ortho_box, const yvec2 planes)
{
	_projection_exists = true;

	_projection_matrix = glm::ortho(ortho_box.x, ortho_box.y, ortho_box.z, ortho_box.w, ortho_box.x, planes.y);
	_clip_close = planes.x;
	_clip_far = planes.y;

	calculate_planes();
}

void camera::create_projection(const float fov, const float aspect, const yvec2 planes)
{
	_projection_exists = true;

	_projection_matrix = glm::perspective(glm::radians(fov), aspect, planes.x, planes.y);
	_clip_close = planes.x;
	_clip_far = planes.y;

	calculate_planes();
}

void camera::set_position(const yvec3 position)
{
	_camera_position = glm::vec3(position.x, position.y, position.z);

	calculate_view();
}

void camera::set_position_x(const float x)
{
	_camera_position.x = x;

	calculate_view();
}

void camera::set_position_y(const float y)
{
	_camera_position.y = y;

	calculate_view();
}

void camera::set_position_z(const float z)
{
	_camera_position.z = z;

	calculate_view();
}

void camera::set_rotation(const float yaw, const float pitch)
{
	_camera_direction.x = std::cos(yaw) * std::cos(pitch);
	_camera_direction.y = std::sin(pitch);
	_camera_direction.z = std::sin(yaw) * std::cos(pitch);

	calculate_view();
}

void camera::set_direction(const yvec3 direction)
{
	_camera_direction = glm::vec3(direction.x, direction.y, direction.z);

	calculate_view();
}

void camera::look_at(const yvec3 look_position)
{
	_camera_direction = glm::normalize(glm::vec3(look_position.x, look_position.y, look_position.z)-_camera_position);

	calculate_view();
}

void camera::translate_position(const yvec3 delta)
{
	_camera_position += delta.x*_camera_right;
	_camera_position += delta.y*_camera_up;
	_camera_position += delta.z*_camera_forward;

	calculate_view();
}

yvec3 camera::position() const noexcept
{
	return {_camera_position.x, _camera_position.y, _camera_position.z};
}

int camera::x_point_side(const yvec3 point) const noexcept
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

int camera::y_point_side(const yvec3 point) const noexcept
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

int camera::z_point_side(const yvec3 point) const noexcept
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

bool camera::cube_in_frustum(const yvec3 middle, const float size) const noexcept
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


const glm::mat4& camera::view_matrix() const noexcept
{
	assert(_view_exists);
	return _view_matrix;
}

const glm::mat4& camera::projection_matrix() const noexcept
{
	assert(_projection_exists);
	return _projection_matrix;
}

float camera::clip_far() const noexcept
{
	return _clip_far;
}

float camera::clip_close() const noexcept
{
	return _clip_close;
}

void camera::calculate_view()
{
	_view_exists = true;

	_camera_forward = glm::normalize(_camera_direction);
	_camera_right = glm::normalize(glm::cross(glm::vec3(0, 1, 0), _camera_direction));
	_camera_up = glm::cross(_camera_direction, _camera_right);

	_view_matrix = glm::lookAt(_camera_position, _camera_position + _camera_forward, _camera_up);

	calculate_planes();
}

void camera::calculate_planes()
{
	const glm::mat4 proj_view_matrix =  _projection_matrix * _view_matrix;

	_planes[side::left] = {proj_view_matrix[0][3]+proj_view_matrix[0][0],
		proj_view_matrix[1][3]+proj_view_matrix[1][0],
		proj_view_matrix[2][3]+proj_view_matrix[2][0],
		proj_view_matrix[3][3]+proj_view_matrix[3][0]};

	_planes[side::right] = {proj_view_matrix[0][3]-proj_view_matrix[0][0],
		proj_view_matrix[1][3]-proj_view_matrix[1][0],
		proj_view_matrix[2][3]-proj_view_matrix[2][0],
		proj_view_matrix[3][3]-proj_view_matrix[3][0]};

	_planes[side::down] = {proj_view_matrix[0][3]+proj_view_matrix[0][1],
		proj_view_matrix[1][3]+proj_view_matrix[1][1],
		proj_view_matrix[2][3]+proj_view_matrix[2][1],
		proj_view_matrix[3][3]+proj_view_matrix[3][1]};

	_planes[side::up] = {proj_view_matrix[0][3]-proj_view_matrix[0][1],
		proj_view_matrix[1][3]-proj_view_matrix[1][1],
		proj_view_matrix[2][3]-proj_view_matrix[2][1],
		proj_view_matrix[3][3]-proj_view_matrix[3][1]};

	_planes[side::back] = {proj_view_matrix[0][3]+proj_view_matrix[0][2],
		proj_view_matrix[1][3]+proj_view_matrix[1][2],
		proj_view_matrix[2][3]+proj_view_matrix[2][2],
		proj_view_matrix[3][3]+proj_view_matrix[3][2]};

	_planes[side::forward] = {proj_view_matrix[0][3]-proj_view_matrix[0][2],
		proj_view_matrix[1][3]-proj_view_matrix[1][2],
		proj_view_matrix[2][3]-proj_view_matrix[2][2],
		proj_view_matrix[3][3]-proj_view_matrix[3][2]};
}
