#ifndef YANCONV_H
#define YANCONV_H

#include <vector>
#include <string>

//everything written by 57qr53r3dn4y to reinvent the wheel (and very poorly)

namespace yandereconv
{
	std::vector<std::string> string_split(std::string text, std::string delimeter);

	//reverses each character bit wise and returns in return_val
	template <typename T>
	void chars_to_number(char* val, T* return_val);

	template <typename T>
	std::vector<T> lengths_to_prefix(std::vector<T> lengths_arr, T max_val, uint8_t* highest_length);

	class yandere_image
	{
	public:
		enum class resize_type {nearest_neighbor, area_sample};

		yandere_image();
		yandere_image(std::string image_path);

		void bpp_resize(uint8_t set_bpp, uint8_t extra_channel=255);

		void resize(unsigned set_width, unsigned set_height, resize_type type);
		void flip();

		void grayscale();

		bool read(std::string load_path);
		bool save_to_file(std::string save_path);
		
		unsigned pixel_color_pos(unsigned x, unsigned y, uint8_t color);
		uint8_t pixel_color(unsigned x, unsigned y, uint8_t color);

		static bool can_parse(std::string extension);

		unsigned width;
		unsigned height;
		uint8_t bpp;

		std::vector<uint8_t> image;

	private:
		bool png_read(bool do_checksums = false);
		std::vector<uint8_t> yan_deflate(std::vector<uint8_t>& input_data);

		bool pgm_read();

		void pgm_save(std::string save_path);
		void ppm_save(std::string save_path);
		
		std::vector<uint8_t> yan_inflate(std::vector<uint8_t>& input_data);
		std::vector<uint32_t> crc_table_gen();
		void png_save(std::string save_path);

		uint8_t sub_positive(int lval, int rval);

		std::string _image_path;
	};
};

#endif
