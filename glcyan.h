#ifndef GLCYAN_H
#define GLCYAN_H

#include <vector>
#include <array>
#include <map>
#include <list>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "glcore.h"

namespace yangl
{
	struct letter_data
	{
		float width;
		float height;
		float origin_x;
		float origin_y;
		float hdist;
	};

	struct raw_font_data
	{
		struct letter_pixels
		{
			int width;
			std::vector<uint8_t> pixels;
		};

		std::array<letter_data, yconst::load_chars> letters;
		std::array<letter_pixels, yconst::load_chars> images;
		int max_width;
		int max_height;
	};

	struct font_data
	{
		std::array<letter_data, yconst::load_chars> letters;
		core::texture texture;
		std::vector<core::model> models;
	};

	class controller
	{
	public:
		struct options
		{
			bool stencil = true;
			bool antialiasing = true;
			bool culling = true;
			bool alpha = true;
		};

		controller(const options opt);

		void config_options(const options opt);

		const font_data* load_font(const std::filesystem::path font_path, const int font_size);

		static std::map<std::string, core::shader>
		load_shaders_from(const std::filesystem::path shaders_folder);

		static std::map<std::string, core::model>
		load_models_from(const std::filesystem::path models_folder);

		static std::map<std::string, core::texture>
		load_textures_from(const std::filesystem::path textures_folder);

	private:
		static font_data create_font(const FT_Face font, const int size) noexcept;
		static raw_font_data load_raw_font(const FT_Face font, const int size) noexcept;

		core::initializer _initializer;

		std::list<font_data> _loaded_fonts;
	};

	class generic_object
	{
	public:
		generic_object();

		generic_object(const camera* cam,
			const generic_shader* program,
			const generic_model* model,
			const generic_texture* texture);

		void set_transform(const yvec3 position, const yvec3 scale, const yvec3 axis, const float rotation);

		yvec3 position() const noexcept;
		void set_position(const yvec3 position);
		void translate(const yvec3 delta);

		yvec3 scale() const noexcept;
		void set_scale(const yvec3 scale);

		float rotation() const noexcept;
		void set_rotation(const float rotation);
		void rotate(const float rotation);

		yvec3 axis() const noexcept;
		void set_rotation_axis(const yvec3 axis);

		void draw() const noexcept;
		
	protected:
		void set_matrix() noexcept;

		yvec3 _position = {0, 0, 0};
		yvec3 _scale = {1, 1, 1};
		yvec3 _axis = {0, 1, 0};
		float _rotation = 0;
		
	private:
		const camera* _camera = nullptr;

		const generic_shader* _shader = nullptr;
		const generic_model* _model = nullptr;
		const generic_texture* _texture = nullptr;

		glm::mat4 _transform;
	};


	class line_object : public generic_object
	{
	public:
		line_object() {};

		line_object(const camera* cam,
			const generic_shader* shader,
			const yvec3 point0, const yvec3 point1,
			const float width);

		void set_positions(const yvec3 point0, const yvec3 point1);
		void set_widths(const float width);
		void set_rotations(const float rotation);
		
	private:
		void calculate_variables();

		yvec3 _point0;
		yvec3 _point1;
		float _width;
	};
	
	class text_object
	{
	public:
		text_object(const camera* cam,
			const generic_shader* shader,
			const font_data* font, const std::string text);

		text_object(const text_object& other);
		text_object(text_object&& other) noexcept;

		text_object& operator=(const text_object& other);
		text_object& operator=(text_object&& other) noexcept;
		
		void draw() const noexcept;
		
		void set_text(const std::string text);
		std::string text() const noexcept;
		
		float text_width() const noexcept;
		float text_height() const noexcept;
		
		void set_position(const yvec3 position);
		yvec3 position() const noexcept;

		void set_scale(const yvec2 scale);
		yvec2 scale() const noexcept;
		
		generic_object create_letter(const int code, const yvec3 position) const noexcept;

	private:
		void generate_letters(const int size);
		
		std::vector<generic_object> _letters;

		const camera* _camera;
		const generic_shader* _shader;
		const font_data* _font;
		
		std::string _text;
		
		yvec3 _position;
		yvec2 _scale;
		
		float _text_width = 0;
		float _text_height = 0;
	};
};

#endif

//surprisingly this has nothing to do with yanderes
