#ifndef YANCONV_H
#define YANCONV_H

#include <vector>
#include <string>
#include <filesystem>

//everything written by 57qr53r3dn4y to reinvent the wheel (and very poorly)

namespace yconv
{
	std::vector<std::string> string_split(std::string text, const std::string delimeter);

	class image
	{
	public:
		enum class resize_type {nearest_neighbor, area_sample};

		image();
		image(const std::filesystem::path image_path);
		image(const unsigned width, const unsigned height, const uint8_t bpp, const std::vector<uint8_t> data);

		void bpp_resize(const uint8_t bpp, const uint8_t extra_channel=255);

		void resize(const unsigned width, const unsigned height, const resize_type type);
		void flip();

		void grayscale();

		bool read(const std::filesystem::path load_path);
		bool save(const std::filesystem::path save_path) const;
		
		unsigned pixel_color_pos(const unsigned x, const unsigned y, const uint8_t color) const noexcept;
		uint8_t pixel_color(const unsigned x, const unsigned y, const uint8_t color) const noexcept;

		bool contains_transparent() const noexcept;
		static bool can_parse(const std::string extension) noexcept;

		unsigned width;
		unsigned height;
		uint8_t bpp;

		std::vector<uint8_t> data;
	};

	namespace png
	{
		image read(const std::filesystem::path load_path);
		void save(const image& img, const std::filesystem::path save_path);

		std::vector<char> create_chunk(const image& img, const std::string name);
		void chunk_header(const std::string name, std::vector<char>& write_data) noexcept;
		void chunk_ending(std::vector<char>& write_data) noexcept;

		typedef std::vector<uint8_t> storage_type;
		std::vector<char> create_data(const storage_type& data_bytes, storage_type::const_iterator iter_start);

		void emplace_line(const image& img, const uint8_t filter, const int line, storage_type& out) noexcept;

		struct filter_values
		{
			unsigned index;
			unsigned width;
			int x;
			int y;
			uint8_t bpp;
		};
		uint8_t defilter_value(const std::vector<uint8_t>& data, const uint8_t filter, const uint8_t val, const filter_values f) noexcept;
		uint8_t filter_value(const image& img, const uint8_t filter, const int x, const int y, const uint8_t col) noexcept;
		uint8_t best_filter(const image& img, const int line) noexcept;

		std::vector<uint32_t> crc_table_gen() noexcept;

		uint8_t paeth_predictor(const int left, const int up, const int up_left) noexcept;

		uint8_t modulo_sub(const int lval, const int rval) noexcept;
		uint8_t modulo_add(const int lval, const int rval) noexcept;
	};

	namespace pgm
	{
		image read(const std::filesystem::path load_path);
		void save(const image& img, const std::filesystem::path save_path);
	};

	namespace ppm
	{
		void save(const image& img, const std::filesystem::path save_path);
	};

	class model
	{
	public:
		model() {};
		model(const std::filesystem::path model_path);

		bool read(const std::filesystem::path load_path);

		std::vector<float> vertices;
		std::vector<int> indices;

	private:
		bool obj_read(const std::filesystem::path load_path);
	};

	namespace ydeflate
	{
		struct f_pos
		{
			int byte;
			uint_fast8_t bit;
		};

		std::vector<uint8_t> deflate(const std::vector<uint8_t>& input_data);

		void uncompressed_block(const std::vector<uint8_t>& input_vec, std::vector<uint8_t>& data_vec, f_pos& pos);
		void static_huffman_block(const std::vector<uint8_t>& input_vec, std::vector<uint8_t>& data_vec, f_pos& pos);
		void dynamic_huffman_block(const std::vector<uint8_t>& input_vec, std::vector<uint8_t>& data_vec, f_pos& pos);

		int huffman_distance(const std::vector<uint8_t>& input_vec, f_pos& pos, int input_bits);
		int huffman_length(const std::vector<uint8_t>& input_vec, f_pos& pos, const int input_bits);

		void shift_offsets(f_pos& pos, int num);
		void shift_offset(f_pos& pos);
		int read_forward(const std::vector<uint8_t>& input_vec, f_pos& pos, int num);

		std::vector<uint8_t> inflate(const std::vector<uint8_t>& input_data);

		//reverses each character bit wise
		template <typename T>
		T chars_to_number(const char* val);

		struct prefix_vector
		{
			std::vector<int> vec;
			uint8_t highest_length;
		};

		prefix_vector lengths_to_prefix(const std::vector<int>& lengths_vec);
	};
};

#endif
