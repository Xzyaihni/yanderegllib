#ifndef YANGLCORE_H
#define YANGLCORE_H

#include <vector>
#include <mutex>
#include <filesystem>

#include <GL/glew.h>
#include <GL/glu.h>
#include <GL/gl.h>

#include "ycamera.h"
#include "yanconv.h"

namespace yangl
{
	enum class default_texture {solid = 0, LAST};
	enum class default_model {plane = 0, circle, triangle, cube, pyramid, LAST};
	enum class default_shader {world = 0, gui, LAST};

	enum class shader_type {fragment, vertex, geometry};

	namespace yconst
	{
		const int load_chars = 128;
		const int circle_lod = 10;
	};

	class generic_model
	{
	public:
		virtual ~generic_model() = default;

		virtual void draw() const = 0;

		virtual bool empty() const = 0;
	};

	class generic_texture
	{
	public:
		virtual ~generic_texture() = default;

		virtual void set_current() const = 0;

		virtual int width() const = 0;
		virtual int height() const = 0;
	};

	class generic_shader
	{
	public:
		virtual ~generic_shader() = default;

		virtual void apply_uniforms(const camera* cam, const glm::mat4& transform) const noexcept = 0;
	};


	namespace global
	{
		inline bool initialized = false;
	};

	template<typename T>
	struct buffers_nocopy
	{
		T container;

		buffers_nocopy()
		{
			if(global::initialized)
				container.generate();
		}

		~buffers_nocopy() {};

		buffers_nocopy(const buffers_nocopy&)
		{
			container.generate();
		}

		buffers_nocopy(buffers_nocopy&& other) noexcept
		: container(other.container)
		{
			other.container = T{};
		}

		buffers_nocopy& operator=(const buffers_nocopy& other)
		{
			if(this!=&other)
			{
				container.generate();
			}
			return *this;
		}

		buffers_nocopy& operator=(buffers_nocopy&& other) noexcept
		{
			if(this!=&other)
			{
				container = other.container;
				other.container = T{};
			}
			return *this;
		}
	};


	namespace core
	{
		class initializer
		{
		public:
			initializer();
		};

		class model_storage : virtual public generic_model
		{
		protected:
			struct container
			{
				~container();

				bool need_update = true;

				unsigned vertex_buffer_object_id = -1;
				unsigned element_object_buffer_id = -1;
				unsigned vertex_array_object_id = -1;

				void generate();
			};

		public:
			model_storage();
			model_storage(const std::filesystem::path model_path);
			model_storage(const default_model id);
			model_storage(const std::vector<float> vertices, const std::vector<unsigned> indices);

			model_storage(const model_storage&) = default;
			model_storage(model_storage&&) noexcept = default;
			model_storage& operator=(const model_storage&) = default;
			model_storage& operator=(model_storage&&) noexcept = default;

			virtual ~model_storage() = default;

			void draw_buffers(const container& buffers) const noexcept;

			bool empty() const noexcept;

			virtual void vertices_insert(const std::initializer_list<float> list) noexcept;
			virtual void indices_insert(const std::initializer_list<unsigned> list) noexcept;

			virtual void clear() noexcept;

		protected:
			void update_buffers(container& buffers) const;

			bool parse_default(const default_model id);

			std::vector<float> _vertices;
			std::vector<unsigned> _indices;
		};

		class model : public model_storage, virtual public generic_model
		{
		public:
			using model_storage::model_storage;

			void draw() const;

			void vertices_insert(const std::initializer_list<float> list) noexcept override;
			void indices_insert(const std::initializer_list<unsigned> list) noexcept override;

			void clear() noexcept override;
			
		private:
			mutable buffers_nocopy<model_storage::container> _buffers;
		};

		class model_manual : public model_storage, virtual public generic_model
		{
		private:
			struct state_holder
			{
				bool generated = false;

				state_holder() {};
				state_holder(const state_holder&) {};
				state_holder(state_holder&& other) noexcept : generated(other.generated) {};
				state_holder& operator=(const state_holder&) {return *this;};
				state_holder& operator=(state_holder&& other) noexcept
				{
					if(this!=&other)
					{
						generated = other.generated;
					}
					return *this;
				}

			} _state;

		public:
			using model_storage::model_storage;

			void generate_buffers() noexcept;

			void draw() const;

			void vertices_insert(const std::initializer_list<float> list) noexcept override;
			void indices_insert(const std::initializer_list<unsigned> list) noexcept override;

			void clear() noexcept override;

		private:
			mutable model_storage::container _buffers;
		};

		class texture : public generic_texture
		{
		private:
			struct container
			{
				~container();

				unsigned texture_buffer_object_id = -1;
				
				void generate();
			};
			
		public:
			texture() {};
			texture(const std::string image_path);
			texture(const default_texture id);
			texture(const yconv::image image);
			texture(const int width, const int height, const std::vector<uint8_t> data);

			void set_current() const;
			
			int width() const;
			int height() const;
			
		private:
			void update_buffers() const;

			bool parse_image(const std::string image_path, const std::string file_format);
			bool parse_default(const default_texture id);

			static unsigned calc_type(const uint8_t bpp);

			unsigned _type;
			yconv::image _image;
			
			bool _empty = true;
			
			buffers_nocopy<container> _buffers;
		};


		class shader
		{
		public:
			shader() {};
			shader(const std::string text, const shader_type type);
			shader(const default_shader shader, const shader_type type);
			
			const std::string& text() const;

			shader_type type() const;

		private:
			std::string _text = "";
			
			shader_type _type;
		};

		class shader_program : public generic_shader
		{
		private:
			struct container
			{
				~container();

				bool need_update = true;

				unsigned program_id = -1;

				void generate();
			};

			struct prop
			{
				int length;
				unsigned location;
			};

		public:
			shader_program() {};
			shader_program(const shader fragment, const shader vertex, const shader geometry);
			
			unsigned add_num(const std::string name);
			unsigned add_vec2(const std::string name);
			unsigned add_vec3(const std::string name);
			unsigned add_vec4(const std::string name);

			template<typename T>
			void set_prop(const unsigned prop_id, const T val)
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

			void apply_uniforms(const camera* cam, const glm::mat4& transform) const noexcept;

			unsigned program() const;

		private:
			static void output_error(const unsigned object_id, const bool is_program = false);

			template<typename T, typename V>
			unsigned add_any(T& type, const V vec_type, const int index, const std::string name);

			void find_uniforms() const;
			void generate_shaders() const;
			
			mutable unsigned _view_mat;
			mutable unsigned _projection_mat;
			mutable unsigned _model_mat;

			std::vector<prop> _props_vec;
			std::vector<float> _shader_props;
			std::vector<yvec2> _shader_props_vec2;
			std::vector<yvec3> _shader_props_vec3;
			std::vector<yvec4> _shader_props_vec4;

			shader _fragment;
			shader _vertex;
			shader _geometry;

			mutable buffers_nocopy<container> _buffers;
		};
	};

	class default_assets_control
	{
	public:
		default_assets_control() {};

		void initialize();

		const core::texture* texture(const default_texture id) const noexcept;
		const core::model* model(const default_model id) const noexcept;
		const core::shader_program* shader(const default_shader id) const noexcept;

	private:
		std::array<core::texture,
			static_cast<int>(default_texture::LAST)> _textures;

		std::array<core::model,
			static_cast<int>(default_model::LAST)> _models;

		std::array<core::shader_program,
			static_cast<int>(default_shader::LAST)> _shaders;
	};

	//cuz its inline the destructor gets called after the gl context is destroyed so its kinda bad??
	inline default_assets_control default_assets;
};

#endif
