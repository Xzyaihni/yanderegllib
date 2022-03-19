#ifndef YANGLCORE_H
#define YANGLCORE_H

#include <vector>
#include <mutex>

#include <GL/glew.h>
#include <GL/glu.h>
#include <GL/gl.h>

#include "yanconv.h"

namespace yanderegl
{
	namespace y_const
	{
		const int circle_lod = 10;
	};

	struct yan_position
	{
		float x = 0;
		float y = 0;
		float z = 0;
	};

	struct yan_scale
	{
		float x = 1;
		float y = 1;
		float z = 1;
	};

	struct yan_transforms
	{
		yan_position position = {};

		yan_scale scale = {};

		float rotation = 0;

		yan_position axis = {0, 0, 1};
	};

	struct yan_color
	{
		float r = 1;
		float g = 1;
		float b = 1;
		float a = 1;
	};

	struct yan_border
	{
		bool enabled = false;
		yan_color color = {};
		float width = 1.1f;
	};

	struct yvec2
	{
		float x = 0;
		float y = 0;
	};

	struct yvec3
	{
		float x = 0;
		float y = 0;
		float z = 0;
	};

	struct yvec4
	{
		float x = 0;
		float y = 0;
		float z = 0;
		float w = 0;
	};


	namespace core
	{
		struct letter_data
		{
			float width;
			float height;
			float origin_x;
			float origin_y;
			float hdist;
		};
		
		struct object_data
		{
			yan_color color;
			float matrix[16];
		};
		const int buffer_size = sizeof(float)*(sizeof(object_data)/sizeof(float));
	
		class initializer
		{
		public:
			initializer(bool stencil, bool antialiasing, bool culling);
		};

		class model
		{
		private:
			struct buffers_nocopy
			{
				bool buffers_made = false;
				bool need_update = true;
				
				unsigned vertex_buffer_object_id;
				unsigned element_object_buffer_id;
			
				unsigned object_data_buffer_id;
			
				unsigned vertex_array_object_id;
				
				buffers_nocopy();
				~buffers_nocopy();
				
				buffers_nocopy(const buffers_nocopy&);
				buffers_nocopy(buffers_nocopy&&) noexcept;
				
				buffers_nocopy& operator=(const buffers_nocopy&);
				buffers_nocopy& operator=(buffers_nocopy&&) noexcept;
				
				private:
					void gen_buffers();
			};
		
		public:
			model() {};
			model(std::string model_path);

			void draw(const object_data* obj_data) const;
			
			bool empty() const;
			
			void vertices_insert(std::initializer_list<float> list);
			void indices_insert(std::initializer_list<unsigned> list);
			
			void clear();
			
		private:
			bool parse_model(std::string model_path, std::string file_format);
			bool parse_default(const std::string name);
			
			void update_buffers() const;
			
			bool _empty = true;
			
			std::vector<float> _vertices;
			std::vector<unsigned> _indices;
			
			buffers_nocopy _buffers;
		};

		class texture
		{
		private:
			struct buffers_nocopy
			{
				bool buffers_made = false;
				
				unsigned texture_buffer_object_id;
				
				buffers_nocopy();
				~buffers_nocopy();
				
				buffers_nocopy(const buffers_nocopy&);
				buffers_nocopy(buffers_nocopy&&) noexcept;
				
				buffers_nocopy& operator=(const buffers_nocopy&);
				buffers_nocopy& operator=(buffers_nocopy&&) noexcept;
				
				private:
					void gen_buffers();
			};
			
		public:
			texture() {};
			texture(std::string image_path);
			texture(yandereconv::yandere_image image);

			void set_current() const;
			
			int width() const;
			int height() const;
			
		private:
			bool parse_image(std::string image_path, std::string file_format);

			static unsigned calc_type(uint8_t bpp);

			unsigned _type;
			yandereconv::yandere_image _image;
			
			bool _empty = true;
			
			buffers_nocopy _buffers;
		};


		enum class shader_type {fragment, vertex, geometry};

		class shader
		{
		public:
			shader() {};
			shader(std::string text, shader_type type);
			
			std::string& text();

			shader_type type();

		private:
			std::string _text;
			
			shader_type _type;
		};

		class shader_program
		{
		private:
			struct prop
			{
				int length;
				unsigned location;
			};

		public:
			shader_program() {};
			shader_program(unsigned program_id);
			
			unsigned add_num(const std::string name);
			unsigned add_vec2(const std::string name);
			unsigned add_vec3(const std::string name);
			unsigned add_vec4(const std::string name);

			template<typename T>
			void set_prop(unsigned prop_id, const T val)
			{
				if constexpr(std::is_same<T, yvec4>::value)
				{
					_shader_props_vec4[prop_id/4] = val;
				} else if constexpr(std::is_same<T, yvec3>::value)
				{
					_shader_props_vec3[prop_id/4] = val;
				} else if constexpr(std::is_same<T, yvec2>::value)
				{
					_shader_props_vec2[prop_id/4] = val;
				} else
				{
					_shader_props[prop_id/4] = val;
				}
			}

			void apply_uniforms();

			unsigned program() const;

			unsigned view_mat() const;
			unsigned projection_mat() const;

		private:
			template<typename T, typename V>
			unsigned add_any(T& type, V vec_type, int index, std::string name);

			void matrix_setup();

			unsigned _program_id;
			
			unsigned _view_mat;
			unsigned _projection_mat;

			std::vector<prop> _props_vec;
			std::vector<float> _shader_props;
			std::vector<yvec2> _shader_props_vec2;
			std::vector<yvec3> _shader_props_vec3;
			std::vector<yvec4> _shader_props_vec4;
		};
	};
};

#endif
