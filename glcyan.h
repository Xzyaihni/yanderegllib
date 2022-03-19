#ifndef GLCYAN_H
#define GLCYAN_H

#include <glm/glm.hpp>

#include <vector>
#include <array>
#include <map>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "glcore.h"

namespace yanderegl
{
	enum default_texture {solid = 0, TEXTURE_LAST};
	enum default_model {square = 0, circle, triangle, cube, pyramid, MODEL_LAST};

	class yandere_camera
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
		yandere_camera() {};
		yandere_camera(const yan_position position, const yan_position look_position);

		void create_projection(const std::array<float, 4> ortho_box, const std::array<float, 2> planes);
		void create_projection(const float fov, const float aspect, const std::array<float, 2> planes);

		void set_position(const yan_position position);
		void set_position_x(const float x);
		void set_position_y(const float y);
		void set_position_z(const float z);

		void set_rotation(const float yaw, const float pitch);
		void set_direction(const std::array<float, 3> direction);
		void look_at(const std::array<float, 3> look_position);

		void translate_position(const yan_position delta);

		yan_position position() const noexcept;

		//returns -1 if the point is in the negative direction, 1 if positive, 0 if inside the camera planes
		int x_point_side(const yan_position point) const noexcept;
		int y_point_side(const yan_position point) const noexcept;
		int z_point_side(const yan_position point) const noexcept;
		
		//i could check for cuboid but im too lazy taking in 8 coordinate points
		bool cube_in_frustum(const yan_position middle, const float size) const noexcept;

		const glm::mat4* proj_view_matrix_ptr() const noexcept;
		const glm::mat4* view_matrix_ptr() const noexcept;
		const glm::mat4* projection_matrix_ptr() const noexcept;
		
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
		
		glm::mat4 _proj_view_matrix;
		
		std::array<plane, 6> _planes;
		
		bool _projection_exists = false;
		bool _view_exists = false;
		
		float _clip_far;
		float _clip_close;
	};

	class yandere_controller;
	
	class yandere_shader_program
	{
	public:
		yandere_shader_program() {};
		yandere_shader_program(const yandere_controller* ctl, std::vector<core::shader_program>* shader_programs_ptr, const unsigned id);
		yandere_shader_program(const yandere_controller* ctl, core::shader_program* shader_program_ptr);
		
		void set_current() const;
		
		unsigned add_num(const std::string name);
		unsigned add_vec2(const std::string name);
		unsigned add_vec3(const std::string name);
		unsigned add_vec4(const std::string name);

		template<typename T>
		void set_prop(unsigned prop_id, const T val)
		{
			c_shader_program()->set_prop(prop_id, val);
		}
		
	private:
		core::shader_program* c_shader_program() const noexcept;
		
		const yandere_controller* _ctl;
	
		unsigned _id = -1;
		core::shader_program* _shader_program_ptr = nullptr;
		std::vector<core::shader_program>* _shader_programs_ptr;
	};


	class yandere_model
	{
	public:
		yandere_model() {};
		yandere_model(const std::vector<core::model>* models_ptr, const unsigned id);
		yandere_model(const core::model* model_ptr);
		
		bool empty() const;
		
		void draw(const core::object_data* obj_data) const;
		
	private:
		const core::model* c_model_ptr() const noexcept;
	
		unsigned _id = -1;
		const core::model* _model_ptr = nullptr;
		const std::vector<core::model>* _models_ptr;
	};
	
	class yandere_texture
	{
	public:
		yandere_texture() {};
		yandere_texture(const std::vector<core::texture>* textures_ptr, const unsigned id);
		yandere_texture(const core::texture* texture_ptr);
		
		void set_current() const;
		
		int width() const;
		int height() const;
		
	private:
		const core::texture* c_texture_ptr() const noexcept;
	
		unsigned _id = -1;
		const core::texture* _texture_ptr = nullptr;
		const std::vector<core::texture>* _textures_ptr;
	};

	class yandere_resources
	{
	public:
		yandere_resources();
		~yandere_resources();
		
		std::map<std::string, unsigned> load_shaders_from(std::string shaders_folder);
		
		std::map<std::string, unsigned> load_models_from(std::string models_folder);
		void load_default_models();
		
		std::map<std::string, unsigned> load_textures_from(std::string textures_folder);
		void load_default_textures();
		
		
		yandere_model create_model(const unsigned id) const noexcept;
		yandere_model create_model(const core::model* model) const noexcept;
		
		yandere_texture create_texture(const unsigned id) const noexcept;
		yandere_texture create_texture(const core::texture* texture) const noexcept;
		
		
		unsigned add_model(core::model& model);
		unsigned add_model(core::model&& model);
		void remove_model(const unsigned model_id);
		void set_model(const unsigned model_id, core::model& model);
		void set_model(const unsigned model_id, core::model&& model);
		core::model model(const unsigned model_id) const noexcept;
		
		unsigned add_texture(core::texture& texture);
		unsigned add_texture(core::texture&& texture);
		void remove_texture(const unsigned texture_id);
		void set_texture(const unsigned texture_id, core::texture& texture);
		void set_texture(const unsigned texture_id, core::texture&& texture);
		core::texture texture(const unsigned texture_id) const noexcept;
		
		unsigned add_shader(core::shader& shader);
		unsigned add_shader(core::shader&& shader);
		
		yandere_shader_program create_shader_program(const yandere_controller* ctl, std::vector<unsigned> shader_id_vec);
		yandere_shader_program shader_program(const yandere_controller* ctl, const unsigned program_id);
		
	private:
		unsigned add_shader_program(core::shader_program& program);
		unsigned add_shader_program(core::shader_program&& program);
		
		void output_error(unsigned object_id, bool is_program = false);
	
		std::vector<unsigned> _empty_models;
		std::vector<core::model> _model_vec;
		
		std::vector<unsigned> _empty_textures;
		std::vector<core::texture> _texture_vec;
		std::vector<core::shader> _shader_vec;
		
		std::vector<core::shader_program> _shader_program_vec;
	};


	class yandere_text;

	class yandere_controller
	{
	public:
		yandere_controller(bool stencil, bool antialiasing, bool culling);
		~yandere_controller();

		void config_options(bool stencil, bool antialiasing, bool culling);
		
		void set_draw_camera(yandere_camera* camera);
		
		const glm::mat4* view_matrix_ptr() const noexcept;
		const glm::mat4* projection_matrix_ptr() const noexcept;

		void load_font(const std::string font_path);
		FT_Face font(const std::string name) const noexcept;
		
		yandere_resources& resources();

		yandere_shader_program create_shader_program(std::vector<unsigned> shader_id_vec);

	private:
		static void debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam);

		core::initializer _initializer;
		yandere_resources _resources;
		yandere_camera* _main_camera = nullptr;

		unsigned _stored_shader_id = -1;

		size_t _current_model_size;
		
		std::map<std::string, FT_Face> _fonts_map;
		
		FT_Library _ft_lib;
		bool _font_init = false;
	};

	class yandere_object
	{
	public:
		yandere_object() {};

		yandere_object(const yandere_shader_program program,
		const yandere_model model,
		const yandere_texture texture,
		const yan_transforms transform,
		const yan_color color = {});

		void set_position(yan_position position);
		void translate(yan_position delta);

		void set_scale(yan_scale scale);
		void set_rotation(float rotation);
		void rotate(float rotation);

		void set_rotation_axis(yan_position axis);

		void set_color(yan_color color);

		void draw_update() const noexcept;
		
	protected:
		void mats_update();
		static glm::mat4 calculate_matrix(yan_transforms transforms);

		yan_transforms _transform;
		yan_color _color;
		
	private:
		yandere_shader_program _shader;
		yandere_model _model;
		yandere_texture _texture;

		core::object_data _obj_data;
	};


	class yandere_line : public yandere_object
	{
	public:
		yandere_line() {};

		yandere_line(yandere_resources& resources, const yandere_shader_program shader,
		const yan_position point0, const yan_position point1,
		const float width,
		const yan_color color = {});

		void set_positions(yan_position point0, yan_position point1);
		void set_widths(float width);
		void set_rotations(float rotation);
		
	private:
		void calculate_variables();

		yan_position _point0;
		yan_position _point1;
		float _width;
	};
	
	class yandere_text
	{
	public:
		yandere_text(const yandere_shader_program shader, 
		const FT_Face font, const std::string text,
		const int size, const float x, const float y);
		
		void draw_update() const noexcept;
		
		void set_text(const std::string text);
		
		void set_position(const float x, const float y);
		std::array<float, 2> position() const noexcept;
		
		float text_width() const noexcept;
		float text_height() const noexcept;
		
		std::string text() const noexcept;
		
	private:
		static const int _load_chars = 128;
	
		std::vector<yandere_object> _letter_objs;
		
		std::vector<core::model> _letter_models;
		std::array<core::letter_data, _load_chars> _letters;

		yandere_shader_program _shader;
		core::texture _texture;
		
		std::string _text;
		
		float _x = 0;
		float _y = 0;
		
		int _size = 0;
		
		float _text_width = 0;
		float _text_height = 0;
	};


	class yandere_gui;
	
	class yandere_panel
	{
	public:
		template<typename T>
		struct object_position
		{
			float x;
			float y;
			T object;
		};
			
		yandere_panel(yandere_gui* gui, float x0, float y0, float x1, float y1);
		
		unsigned add_text(object_position<yandere_text> text);
		
	private:
		yandere_gui* _gui;

		float _x;
		float _y;
		
		float _width;
		float _height;

		float _x0;
		float _y0;
		float _x1;
		float _y1;
		
		std::vector<object_position<yandere_text>> _texts_vec;
		std::vector<object_position<yandere_object>> _gui_objects;
		
		std::vector<uint8_t> _order_vec;
	};

	class yandere_gui
	{
	public:
		yandere_gui() {};
		
		yandere_panel add_panel(float x0, float y0, float x1, float y1);
	};
};

#endif

//surprisingly this has nothing to do with yanderes
