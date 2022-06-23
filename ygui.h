#ifndef Y_YGUI_H
#define Y_YGUI_H

#include <list>

#include "glcyan.h"

namespace yangl
{
	namespace gui
	{
		struct object_info
		{
			//relative to gui size in that direction (z pos is depth)
			yvec3 pos;

			//the smallest size of 2 is applied to both directions
			yvec2 scale;

			//1 on x for rightmost side 1 on y for topmost side
			yvec2 origin;
		};

		template<typename T>
		struct store_object
		{
			object_info info;

			T object;
		};

		class panel
		{
		public:
			panel(const camera* cam, const generic_shader* shader,
				const yvec2 corner0, const yvec2 corner1);

			void resize(const int parent_width, const int parent_height);

			void draw_update() noexcept;
			void update();

			//doesnt update the positions/sizes
			void draw() const noexcept;

			void set_visible(const bool state) noexcept;
			bool visible() const noexcept;

			store_object<text_object>& add_text(const object_info info, const font_data* font, const std::string text);
			store_object<generic_object>& add_object(const object_info info, const core::texture texture);

		private:
			yvec2 calculate_scale(const object_info info, const yvec2 size) const noexcept;
			yvec2 calculate_position(const object_info info, const yvec2 size, const yvec2 scale) const noexcept;

			float _parent_width = 0;
			float _parent_height = 0;

			yvec2 _corner0;
			yvec2 _corner1;
			
			bool _visible = true;

			const camera* _camera;
			const generic_shader* _shader;

			std::list<store_object<text_object>> _texts_list;
			std::list<store_object<generic_object>> _gui_objects_list;
		};

		class controller
		{
		private:
			struct camera_storage
			{
				camera_storage();
				camera_storage(const int width, const int height);

				camera_storage(const camera_storage&);
				camera_storage(camera_storage&&) noexcept;
				camera_storage& operator=(const camera_storage&);
				camera_storage& operator=(camera_storage&&) noexcept;

				void resize(const int width, const int height);
				void set_camera(const camera* cam);

				camera default_camera;
				const camera* ptr = nullptr;

			private:
				static const int _max_depth = 1024;
			};

		public:
			controller() {};
			controller(const int width, const int height);

			void resize(const int width, const int height);

			void draw() const noexcept;

			panel& add_panel(const yvec2 corner0, const yvec2 corner1);

			void set_camera(const camera* cam);
			void set_shader(const generic_shader* shader);

		private:
			bool _empty = true;

			int _width;
			int _height;

			const generic_shader* _shader = nullptr;

			camera_storage _camera;

			std::list<panel> _panels_list;
		};
	};
};

#endif
