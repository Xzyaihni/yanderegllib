#ifndef YAN_YCAMERA_H
#define YAN_YCAMERA_H

#include <array>

#include <glm/ext.hpp>

#include "ytypes.h"

namespace yangl
{
	class camera
	{
	private:
		struct plane
		{
			float a;
			float b;
			float c;
			float d;
		};

		enum side
		{
			left = 0,
			right,
			down,
			up,
			back,
			forward
		};

	public:
		camera() {};
		camera(const yvec3 position, const yvec3 look_position);

		void create_projection(const yvec4 ortho_box, const yvec2 planes);
		void create_projection(const float fov, const float aspect, const yvec2 planes);

		void set_position(const yvec3 position);
		void set_position_x(const float x);
		void set_position_y(const float y);
		void set_position_z(const float z);

		void set_rotation(const float yaw, const float pitch);
		void set_direction(const yvec3 direction);
		void look_at(const yvec3 look_position);

		void translate_position(const yvec3 delta);

		yvec3 position() const noexcept;

		//returns -1 if the point is in the negative direction, 1 if positive, 0 if inside the camera planes
		int x_point_side(const yvec3 point) const noexcept;
		int y_point_side(const yvec3 point) const noexcept;
		int z_point_side(const yvec3 point) const noexcept;

		//i could check for cuboid but im too lazy taking in 8 coordinate points
		bool cube_in_frustum(const yvec3 middle, const float size) const noexcept;

		const glm::mat4& view_matrix() const noexcept;
		const glm::mat4& projection_matrix() const noexcept;

		float clip_far() const noexcept;
		float clip_close() const noexcept;

	private:
		void calculate_view();

		void calculate_planes();

		glm::vec3 _camera_position;
		glm::vec3 _camera_direction;
		glm::vec3 _camera_up;
		glm::vec3 _camera_right;
		glm::vec3 _camera_forward;

		glm::mat4 _view_matrix;
		glm::mat4 _projection_matrix;

		std::array<plane, 6> _planes;

		bool _projection_exists = false;
		bool _view_exists = false;

		float _clip_far;
		float _clip_close;
	};
};

#endif
