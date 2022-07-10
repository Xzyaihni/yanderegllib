#include <cmath>
#include <cstring>
#include <climits>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <tuple>
#include <filesystem>
#include <array>

#include "yanconv.h"

using namespace yconv;

std::vector<std::string> yconv::string_split(std::string text, const std::string delimeter)
{
    std::vector<std::string> r_vec;

    int c_delimeter_pos = text.find(delimeter);
    while(c_delimeter_pos!=-1)
    {
        r_vec.push_back(text.substr(0, c_delimeter_pos));

        text = text.substr(c_delimeter_pos+1);
        c_delimeter_pos = text.find(delimeter);
    }

    r_vec.push_back(text.substr(0));

    return r_vec;
}

image::image() : data({})
{
}

image::image(const std::filesystem::path image_path)
{
    if(!read(image_path))
    	throw std::runtime_error("cant read texture: " + image_path.string());
}

image::image(const unsigned width, const unsigned height, const uint8_t bpp, const std::vector<uint8_t> data)
: width(width), height(height), bpp(bpp), data(data)
{

}

void image::grayscale()
{
    if(bpp==1)
        return;

    std::vector<uint8_t> grayscale_image;
    grayscale_image.reserve(width*height);

    for(unsigned y = 0; y<height; ++y)
    {
        for(unsigned x = 0; x<width; ++x)
        {
            unsigned avg_pixel = 0;

            for(uint8_t bppi = 0; bppi < bpp; ++bppi)
            {
                avg_pixel += data[y*width*bpp+x*bpp+bppi];
            }
            avg_pixel /= bpp;
            grayscale_image.emplace_back(avg_pixel);
        }
    }

    bpp = 1;
    data = std::move(grayscale_image);
}

void image::bpp_resize(const uint8_t set_bpp, const uint8_t extra_channel)
{
    if(bpp==set_bpp)
        return;

    std::vector<uint8_t> recolored_image;
    recolored_image.reserve(width*height*set_bpp);

    for(unsigned y = 0; y<height; ++y)
    {
        for(unsigned x = 0; x<width; ++x)
        {
            for(uint8_t bppi = 0; bppi<set_bpp; ++bppi)
            {
                if(bppi<bpp)
                {
                    recolored_image.emplace_back(data[y*width*bpp+x*bpp+bppi]);
                } else
                {
                    recolored_image.emplace_back(extra_channel);
                }
            }
        }
    }

    bpp = set_bpp;
    data = std::move(recolored_image);
}

void image::resize(const unsigned set_width, const unsigned set_height, const resize_type type)
{
    if(set_width==width&&set_height==height)
        return;

    std::vector<uint8_t> resized_image;
    resized_image.reserve(set_width*set_height*bpp);

    const float resize_ratio_width = static_cast<float>(set_width)/width;
    const float resize_ratio_height = static_cast<float>(set_height)/height;

    const float c_pixel_width = 1/resize_ratio_width;
    const float c_pixel_height = 1/resize_ratio_height;
    const float c_pixel_width_half = c_pixel_width/2;
    const float c_pixel_height_half = c_pixel_height/2;

    std::vector<uint8_t> res_pixel(bpp);
    std::vector<unsigned> avg_pixel(bpp);

    for(unsigned y = 0; y < set_height; ++y)
    {
        for(unsigned x = 0; x < set_width; ++x)
        {
            switch(type)
            {
                case resize_type::nearest_neighbor:
                {
                    const float ratio_x = x/static_cast<float>(set_width);
                    const float ratio_y = y/static_cast<float>(set_height);

                    const float calc_x = std::round(width*ratio_x);
                    const float calc_y = std::round(height*ratio_y);

                    const unsigned pixel_start_index = calc_y*width*bpp+calc_x*bpp;
                    for(uint8_t bppi = 0; bppi < bpp; ++bppi)
                    {
                        res_pixel[bppi] = data[pixel_start_index+bppi];
                    }

                    break;
                }

                case resize_type::area_sample:
                {
                    const float left_border = std::max(0.0f, (x/resize_ratio_width-c_pixel_width_half));
                    const float top_border = std::max(0.0f, (y/resize_ratio_height-c_pixel_height_half));
                    const float right_border = std::min(static_cast<float>(width), (x/resize_ratio_width+c_pixel_width_half));
                    const float bottom_border = std::min(static_cast<float>(height), (y/resize_ratio_height+c_pixel_height_half));

                    const unsigned lowest_x = static_cast<unsigned>(left_border);
                    const unsigned lowest_y = static_cast<unsigned>(top_border);
                    const unsigned highest_x = std::ceil(right_border);
                    const unsigned highest_y = std::ceil(bottom_border);

                    float pixels_avgd = 0;

                    for(unsigned oy = lowest_y; oy < highest_y; ++oy)
                    {
                        for(unsigned ox = lowest_x; ox < highest_x; ++ox)
                        {
                            float pixel_inside_area_ratio;

                            if(oy!=lowest_y&&ox!=lowest_x&&oy!=(highest_y-1)&&ox!=(highest_x-1))
                            {
                                //pixel fully inside the area
                                pixel_inside_area_ratio = 1;
                            } else
                            {
                                //pixel partially outside the area

                                const float scaled_x = x/resize_ratio_width;
                                const float scaled_y = y/resize_ratio_height;

                                float overlap_width;
                                float overlap_height;

                                if(ox!=lowest_x&&ox!=(highest_x-1))
                                {
                                    if(scaled_x<=ox)
                                    {
                                        overlap_width = scaled_x+c_pixel_width_half-ox;
                                        overlap_width = overlap_width>1?1:overlap_width;
                                    } else
                                    {
                                        overlap_width = scaled_x-(ox+1);
                                        overlap_width = overlap_width>1?1:overlap_width;
                                    }
                                } else
                                {
                                    //this is just for a speedup
                                    overlap_width = 1;
                                }

                                if(ox!=lowest_x&&ox!=(highest_x-1))
                                {
                                    if(scaled_y<=oy)
                                    {
                                        overlap_height = scaled_y+c_pixel_height_half-oy;
                                        overlap_height = overlap_height>1?1:overlap_height;
                                    } else
                                    {
                                        overlap_height = scaled_y-(oy+1);
                                        overlap_height = overlap_height>1?1:overlap_height;
                                    }
                                } else
                                {
                                    //this is just for a speedup
                                    overlap_height = 1;
                                }

                                pixel_inside_area_ratio = overlap_width*overlap_height;
                            }

                            pixels_avgd += pixel_inside_area_ratio;

                            for(uint8_t bppi = 0; bppi < bpp; ++bppi)
                            {
                                avg_pixel[bppi] += data[oy*width*bpp+ox*bpp+bppi]*pixel_inside_area_ratio;
                            }
                        }
                    }

                    for(uint8_t bppi = 0; bppi < bpp; ++bppi)
                    {
                        res_pixel[bppi] = avg_pixel[bppi]/pixels_avgd;
                        avg_pixel[bppi] = 0;
                    }

                    break;
                }
            }

            for(uint8_t bppi = 0; bppi < bpp; ++bppi)
            {
                resized_image.emplace_back(res_pixel[bppi]);
            }
        }
    }

    width = set_width;
    height = set_height;

    data = std::move(resized_image);
}

void image::flip()
{
    std::vector<uint8_t> flipped_data;
    flipped_data.reserve(data.size());

    for(unsigned y = height; y>0; --y)
    {
        for(unsigned x = 0; x<width; ++x)
        {
            for(uint8_t bppi = 0; bppi<bpp; ++bppi)
            {
                flipped_data.emplace_back(data[(y-1)*width*bpp+x*bpp+bppi]);
            }
        }
    }

    data = std::move(flipped_data);
}

bool image::save(const std::filesystem::path save_path) const
{
    if(save_path.extension()==".pgm")
    {
        pgm::save(*this, save_path);

        return true;
    } else if(save_path.extension()==".ppm")
    {
    	ppm::save(*this, save_path);
    	
    	return true;
    } else if(save_path.extension()==".png")
    {
    	png::save(*this, save_path);
    	
    	return true;
    }

    return false;
}

bool image::read(const std::filesystem::path load_path)
{
    if(load_path.extension()==".png")
    {
        *this = png::read(load_path);
        return true;
    } else if(load_path.extension()==".pgm")
    {
        *this = pgm::read(load_path);
        return true;
    }
    
    return false;
}

unsigned image::pixel_color_pos(const unsigned x, const unsigned y, const uint8_t color) const noexcept
{
	return y*width*bpp+x*bpp+color;
}

uint8_t image::pixel_color(const unsigned x, const unsigned y, const uint8_t color) const noexcept
{
	return data[y*width*bpp+x*bpp+color];
}

bool image::contains_transparent() const noexcept
{
    if(bpp!=4)
        return false;

    unsigned index = 0;
    for(int y = 0; y < height; ++y)
    {
        for(int x = 0; x < width; ++x, index += bpp)
        {
            if(data[index+(bpp-1)]!=255)
                return true;
        }
    }

    return false;
}

bool image::can_parse(const std::string extension) noexcept
{
    if(extension=="png" || extension==".png")
    {
        return true;
    } else if(extension=="pgm" || extension==".pgm")
    {
        return true;
    }
    return false;
}

model::model(const std::filesystem::path model_path)
{
    if(!read(model_path))
        throw std::runtime_error("cant read model: " + model_path.string());
}

bool model::read(const std::filesystem::path load_path)
{
    if(load_path.extension()==".obj")
    {
        obj_read(load_path);
        return true;
    }

    return false;
}

bool model::obj_read(const std::filesystem::path load_path)
{
    std::ifstream model_stream(load_path);
    std::string c_model_line;

    vertices.clear();
    indices.clear();

    std::vector<float> c_verts;
    std::vector<int> vert_indices;
    std::vector<float> c_uvs;
    std::vector<int> uv_indices;

    std::vector<float> c_normals;
    std::vector<int> normal_indices;

    std::vector<std::string> saved_params;
    std::vector<int> repeated_verts;

    while(std::getline(model_stream, c_model_line))
    {
        if(c_model_line.find("#")!=-1)
            c_model_line = c_model_line.substr(0, c_model_line.find("#"));

        if(c_model_line.length()==0)
            continue;

        if(c_model_line.find("vt")==0)
        {
            std::stringstream texuv_stream(c_model_line.substr(3));

            float x, y;

            texuv_stream >> x >> y;

            c_uvs.reserve(3);
            c_uvs.push_back(x);
            c_uvs.push_back(y);
        } else if(c_model_line.find("vn")==0)
        {
            std::stringstream normal_stream(c_model_line.substr(3));

            float x, y, z;

            normal_stream >> x >> y >> z;

            c_normals.reserve(3);
            c_normals.push_back(x);
            c_normals.push_back(y);
            c_normals.push_back(z);
        } else if(c_model_line.find("v")==0)
        {
            std::stringstream vertex_stream(c_model_line.substr(2));

            float x, y, z;

            vertex_stream >> x >> y >> z;

            c_verts.reserve(3);
            c_verts.push_back(x);
            c_verts.push_back(y);
            c_verts.push_back(z);
        } else if(c_model_line.find("f")==0)
        {
            std::vector<std::string> params = string_split(c_model_line.substr(2), " ");

            const int params_size = params.size();

            if(params[params_size-1].length()==0)
            {
                params.pop_back();
            }

            std::vector<int> order = {0, 1, 2};

            if(params_size>3)
            {
                for(int i = 3; i < params_size; i++)
                {
                    order.push_back(0);
                    order.push_back(i-1);
                    order.push_back(i);
                }
            }

            for(const auto& p : order)
            {
                const auto find_index = std::find(saved_params.begin(), saved_params.end(), params[p]);

                if(find_index!=saved_params.end())
                {
                    repeated_verts.push_back(vert_indices.size()-std::distance(saved_params.begin(), find_index));
                } else
                {
                    repeated_verts.push_back(0);
                }

                saved_params.push_back(params[p]);

                const std::vector<std::string> face_vals = string_split(params[p], "/");

                vert_indices.push_back(std::stoi(face_vals[0])-1);

                if(face_vals.size()>=2 && face_vals[1].length()!=0)
                {
                    uv_indices.push_back(std::stoi(face_vals[1])-1);
                } else
                {
                    uv_indices.push_back(UINT_MAX);
                }

                if(face_vals.size()==3)
                {
                    normal_indices.push_back(std::stoi(face_vals[2])-1);
                } else
                {
                    normal_indices.push_back(UINT_MAX);
                }
            }
        } else
        {
            //std::cout << c_model_line << std::endl;
        }
    }

    int indexing_offset = 0;

    for(int v = 0; v < vert_indices.size(); v++)
    {
        if(repeated_verts[v]==0)
        {
            vertices.push_back(c_verts[vert_indices[v]*3]);
            vertices.push_back(c_verts[vert_indices[v]*3+1]);
            vertices.push_back(c_verts[vert_indices[v]*3+2]);

            indices.push_back(v-indexing_offset);

            if(uv_indices[v]!=UINT_MAX)
            {
                vertices.push_back(c_uvs[uv_indices[v]*2]);
                vertices.push_back(c_uvs[uv_indices[v]*2+1]);
            } else
            {
                vertices.push_back(0);
                vertices.push_back(0);
            }
        } else
        {
            indices.push_back(indices[v-repeated_verts[v]]);
            indexing_offset++;
        }
    }

    return true;
}

image png::read(const std::filesystem::path load_path)
{
    std::ifstream input_stream(load_path, std::ios::binary);

    //PNG files begin with 89 50 4e 47 0d 0a 1a 0a
    input_stream.seekg(8, std::ios_base::beg);

    uint8_t bit_depth;
    uint8_t values_per_pixel;

    bool pallete_used;
    bool rgb_used;
    bool alpha_channel;

    uint8_t vals_per_byte;

    bool interlacing;

    std::vector<uint8_t> deflate_stream;

    std::vector<uint8_t> pallete_vector;

    std::vector<uint8_t> temp_data;
    unsigned temp_width = 0;
    unsigned temp_height = 0;

    image img;
    while(input_stream.good())
    {
        //this is definitely not proper c++
        //THIS ISNT EVEN PROPER PROGRAMMING
        std::array<char, 4> chunk_length_str;
        input_stream.read(chunk_length_str.data(), 4);

        const unsigned chunk_length = ydeflate::chars_to_number<unsigned>(chunk_length_str.data());

        std::array<char, 4> chunk_type;
        input_stream.read(chunk_type.data(), 4);

        std::vector<char> chunk_data(chunk_length); //char is always 1 byte long
        input_stream.read(chunk_data.data(), chunk_length);

        std::array<char, 4> chunk_checksum;
        input_stream.read(chunk_checksum.data(), 4);

        //could do the checksum here if ur cool

        if(strncmp(chunk_type.data(), "IHDR", 4)==0)
        {
            std::array<char, 4> image_width_str;
            std::array<char, 4> image_height_str;
            for(int i = 0; i < 4; i++)
            {
                image_width_str[i] = chunk_data[i];
                image_height_str[i] = chunk_data[i+4];
            }
            temp_width = ydeflate::chars_to_number<unsigned>(image_width_str.data());
            temp_height = ydeflate::chars_to_number<unsigned>(image_height_str.data());

            bit_depth = static_cast<uint8_t>(chunk_data[8]);
            const uint8_t color_type = static_cast<uint8_t>(chunk_data[9]);

            pallete_used = color_type&0x1;
            rgb_used = (color_type>>1)&0x1;
            alpha_channel = (color_type>>2)&0x1;

            values_per_pixel = (2*rgb_used)+alpha_channel+1;

            img.bpp = std::ceil((values_per_pixel*bit_depth)/8.0f);
            vals_per_byte = 8/bit_depth;

            interlacing = static_cast<bool>(chunk_data[12]);
        } else if(strncmp(chunk_type.data(), "IDAT", 4)==0)
        {
            deflate_stream.reserve(chunk_length);
            for(int i = 0; i < chunk_length; i++)
            {
                deflate_stream.push_back(chunk_data[i]);
            }
        } else if(strncmp(chunk_type.data(), "PLTE", 4)==0)
        {
            if(chunk_length%3!=0)
                throw std::runtime_error("PLTE chunk length is not a multiple of 3");

            pallete_vector.reserve(chunk_length);
            for(int i = 0; i < chunk_length; i++)
            {
                pallete_vector.emplace_back(chunk_data[i]);
            }
        } else if(strncmp(chunk_type.data(), "IEND", 4)==0)
        {
            std::vector<uint8_t> image_data = ydeflate::deflate(deflate_stream);
            if(image_data.size()!=0)
            {
                temp_data.reserve(std::ceil(temp_width*temp_height*values_per_pixel/static_cast<float>(vals_per_byte)));

                unsigned stream_pos = 0;

                for(int y = 0; y < temp_height; ++y)
                {
                    const uint8_t filter_type = image_data[stream_pos++];

                    if(pallete_used)
                    {
                        for(int x = 0; x < temp_width; ++x, ++stream_pos)
                        {
                            const unsigned pallete_pixel = image_data[stream_pos]*3;
                            temp_data.emplace_back(pallete_vector[pallete_pixel]);
                            temp_data.emplace_back(pallete_vector[pallete_pixel+1]);
                            temp_data.emplace_back(pallete_vector[pallete_pixel+2]);
                        }
                    } else
                    {
                        for(int x = 0; x < temp_width; ++x)
                        {
                            for(uint8_t b = 0; b < values_per_pixel; ++b, ++stream_pos)
                            {
                                temp_data.emplace_back(defilter_value(temp_data, filter_type, image_data[stream_pos],
                                    filter_values{static_cast<unsigned>(temp_data.size()), temp_width, x, y, img.bpp}));
                            }
                        }
                    }
                }
            }


            img.data = std::move(temp_data);
            img.width = temp_width;
            img.height = temp_height;

            break;
        }
    }

    return img;
}

void png::save(const image& img, const std::filesystem::path save_path)
{
    std::ofstream out_stream(save_path, std::ios::binary);

    std::vector<char> image_header;
    //magic numbers for png
    image_header.push_back(0x89);

    //PNG in ascii
    image_header.push_back('P');
    image_header.push_back('N');
    image_header.push_back('G');

    //line ending
    image_header.push_back(0x0d);
    image_header.push_back(0x0a);

    image_header.push_back(0x1a);
    image_header.push_back(0x0a);


    out_stream.write(image_header.data(), image_header.size());

    const std::string width_string = std::to_string(img.width);
    image_header.insert(image_header.end(), width_string.begin(), width_string.end());

    const std::string height_string = std::to_string(img.height);
    image_header.insert(image_header.end(), height_string.begin(), height_string.end());

    const std::vector<std::string> encode_chunks{"IHDR", "IDAT", "IEND"};
    for(int i = 0; i < encode_chunks.size(); ++i)
    {
        const std::vector<char> data_chunk = create_chunk(img, encode_chunks[i]);
        out_stream.write(data_chunk.data(), data_chunk.size());
    }
}

std::vector<char> png::create_chunk(const image& img, const std::string name)
{
    if(name=="IDAT")
    {
        std::vector<uint8_t> image_bytes;
        image_bytes.reserve(img.height*img.width*img.bpp+img.height);

        for(int y = 0; y < img.height; ++y)
        {
            const uint8_t line_filter = best_filter(img, y);
            image_bytes.emplace_back(line_filter);

            emplace_line(img, line_filter, y, image_bytes);
        }

        const std::vector<uint8_t> inflated_bytes = ydeflate::inflate(image_bytes);

        return create_data(inflated_bytes, inflated_bytes.begin());
    }

    std::vector<char> data_chunk;

    chunk_header(name, data_chunk);

    if(name=="IHDR")
    {
        //4 bytes of width
        data_chunk.push_back((img.width&0xff000000)>>24);
        data_chunk.push_back((img.width&0xff0000)>>16);
        data_chunk.push_back((img.width&0xff00)>>8);
        data_chunk.push_back(img.width&0xff);

        //4 bytes of height
        data_chunk.push_back((img.height&0xff000000)>>24);
        data_chunk.push_back((img.height&0xff0000)>>16);
        data_chunk.push_back((img.height&0xff00)>>8);
        data_chunk.push_back(img.height&0xff);

        //bits per pixel, always 8 (bla bla bla not efficient)
        data_chunk.push_back(8);

        //color type
        //0100 bit adds alpha
        //0010 bit adds color
        //0001 bit means color from pallete
        switch(img.bpp)
        {
            case 1:
                //grayscale
                data_chunk.push_back(0);
                break;
            case 2:
                //grayscale with alpha
                data_chunk.push_back(4);
                break;
            case 3:
                //fullcolor
                data_chunk.push_back(2);
                break;
            case 4:
                //fullcolor with alpha
                data_chunk.push_back(6);
                break;

            default:
                throw std::runtime_error("invalid IHDR bpp type");
        }

        //stuff that doesnt change, like png using deflate algorithm for compression
        data_chunk.push_back(0);
        data_chunk.push_back(0);

        //interlacing off
        data_chunk.push_back(0);
    }

    chunk_ending(data_chunk);

    return data_chunk;
}

void png::chunk_header(const std::string name, std::vector<char>& write_data) noexcept
{
    write_data.reserve(4);
    for(int cn = 0; cn < 4; ++cn)
        write_data.emplace_back(name[cn]);
}

void png::chunk_ending(std::vector<char>& write_data) noexcept
{
    const std::vector<uint32_t> crc_table = crc_table_gen();

    uint32_t chunk_crc = 0xffffffff;

    const int crc_length = write_data.size();

    for(int b = 0; b < crc_length; ++b)
    {
        chunk_crc = (crc_table[(chunk_crc^write_data[b])&0xff]^(chunk_crc>>8));
    }

    chunk_crc ^= 0xffffffff;
    //crc stuff over

    const int chunk_length = crc_length-4; //remove the name bytes

    write_data.insert(write_data.begin(), chunk_length&0xff);
    write_data.insert(write_data.begin(), (chunk_length&0xff00)>>8);
    write_data.insert(write_data.begin(), (chunk_length&0xff0000)>>16);
    write_data.insert(write_data.begin(), (chunk_length&0xff000000)>>24);


    write_data.push_back((chunk_crc&0xff000000)>>24);
    write_data.push_back((chunk_crc&0xff0000)>>16);
    write_data.push_back((chunk_crc&0xff00)>>8);
    write_data.push_back(chunk_crc&0xff);
}

std::vector<char> png::create_data(const storage_type& data_bytes, storage_type::const_iterator iter_start)
{
    std::vector<char> data_chunk;

    chunk_header("IDAT", data_chunk);

    const int max_chunk_size = 8192; //idk where this comes from but gimp uses the same value :/
    if(std::distance(iter_start, data_bytes.end())>max_chunk_size)
    {
        const storage_type::const_iterator end_iter = iter_start+max_chunk_size;
        data_chunk.insert(data_chunk.end(), iter_start, end_iter);
        chunk_ending(data_chunk);

        const std::vector<char> next_chunk = create_data(data_bytes, end_iter);
        data_chunk.insert(data_chunk.end(), next_chunk.begin(), next_chunk.end());
    } else
    {
        data_chunk.insert(data_chunk.end(), iter_start, data_bytes.end());
        chunk_ending(data_chunk);
    }

    return data_chunk;
}

void png::emplace_line(const image& img, const uint8_t filter, const int line, storage_type& out) noexcept
{
    for(int x = 0; x < img.width; ++x)
        for(int b = 0; b < img.bpp; ++b)
            out.emplace_back(filter_value(img, filter, x, line, b));
}

uint8_t png::defilter_value(const std::vector<uint8_t>& data, const uint8_t filter, const uint8_t val, const filter_values f) noexcept
{
    switch(filter)
    {
        default:
        case 0:
        return val;

        case 1:
        {
            if(f.x==0)
                return val;
            return modulo_add(val, data[f.index-f.bpp]);
        }
        case 2:
        {
            if(f.y==0)
                return val;

            const unsigned above_index = f.index-(f.width*f.bpp);
            return modulo_add(val, data[above_index]);
        }
        case 3:
        {
            const unsigned above_index = f.index-(f.width*f.bpp);
            const uint8_t up_pixel = f.y==0 ? 0 : data[above_index];
            const uint8_t left_pixel = f.x==0 ? 0 : data[f.index-f.bpp];

            return modulo_add(val, (left_pixel+up_pixel)/2);
        }
        case 4:
        {
            const unsigned above_index = f.index-(f.width*f.bpp);
            const uint8_t up_pixel = f.y==0 ? 0 : data[above_index];
            const uint8_t left_pixel = f.x==0 ? 0 : data[f.index-f.bpp];
            const uint8_t up_left_pixel = ((f.y==0) || (f.x==0)) ? 0 : data[above_index-f.bpp];

            return modulo_add(val, paeth_predictor(left_pixel, up_pixel, up_left_pixel));
        }
    }
}

uint8_t png::filter_value(const image& img, const uint8_t filter, const int x, const int y, const uint8_t col) noexcept
{
    const uint8_t c_color = img.pixel_color(x, y, col);

    switch(filter)
    {
        default:
        case 0:
        return c_color;

        case 1:
        {
            if(x==0)
                return c_color;
            return modulo_sub(c_color, img.pixel_color(x-1, y, col));
        }

        case 2:
        {
            if(y==0)
                return c_color;
            return modulo_sub(c_color, img.pixel_color(x, y-1, col));
        }

        case 3:
        {
            const int left_pixel = x==0 ? 0 : img.pixel_color(x-1, y, col);
            const int up_pixel = y==0 ? 0 : img.pixel_color(x, y-1, col);
            return modulo_sub(c_color, (left_pixel+up_pixel)/2);
        }

        case 4:
        {
            const int left_pixel = x==0 ? 0 : img.pixel_color(x-1, y, col);
            const int up_pixel = y==0 ? 0 : img.pixel_color(x, y-1, col);
            const int up_left_pixel = ((x==0) || (y==0)) ? 0 : img.pixel_color(x-1, y-1, col);

            return modulo_sub(c_color, paeth_predictor(left_pixel, up_pixel, up_left_pixel));
        }
    }
}

uint8_t png::best_filter(const image& img, const int line) noexcept
{
    std::array<unsigned, 5> sum{0, 0, 0, 0, 0};

    for(uint8_t f = 0; f < 5; ++f)
    {
        for(int x = 0; x < img.width; ++x)
        {
            for(uint8_t b = 0; b < img.bpp; ++b)
            {
                sum[f] += filter_value(img, f, x, line, b);
            }
        }
    }

    if(sum[0]<=sum[1] && sum[0]<=sum[2] && sum[0]<=sum[3] && sum[0]<=sum[4])
        return 0;

    if(sum[1]<=sum[0] && sum[1]<=sum[2] && sum[1]<=sum[3] && sum[1]<=sum[4])
        return 1;

    if(sum[2]<=sum[1] && sum[2]<=sum[0] && sum[2]<=sum[3] && sum[2]<=sum[4])
        return 2;

    if(sum[3]<=sum[1] && sum[3]<=sum[2] && sum[3]<=sum[0] && sum[3]<=sum[4])
        return 3;

    if(sum[4]<=sum[1] && sum[4]<=sum[2] && sum[4]<=sum[3] && sum[4]<=sum[0])
        return 4;

    return 0;
}

std::vector<uint32_t> png::crc_table_gen() noexcept
{
    std::vector<uint32_t> crc_table;
    crc_table.reserve(256);

    for(int i = 0; i < 256; ++i)
    {
        uint32_t table_num = static_cast<uint32_t>(i);

        for(int l = 0; l < 8; ++l)
        {
            if(table_num&0x1)
            {
                table_num = 0xedb88320^(table_num>>1);
            } else
            {
                table_num >>= 1;
            }
        }

        crc_table.emplace_back(table_num);
    }

    return crc_table;
}

uint8_t png::paeth_predictor(const int left, const int up, const int up_left) noexcept
{
    const int initial_est = left + up - up_left;

    const int distance_left = std::abs(initial_est - left);
    const int distance_up = std::abs(initial_est - up);
    const int distance_up_left = std::abs(initial_est - up_left);

    if((distance_left<=distance_up) && (distance_left<=distance_up_left))
    {
        return left;
    } else if(distance_up<=distance_up_left)
    {
        return up;
    } else
    {
        return up_left;
    }
}

uint8_t png::modulo_sub(const int lval, const int rval) noexcept
{
    return (lval - rval) % 256;
}

uint8_t png::modulo_add(const int lval, const int rval) noexcept
{
    return lval + rval - 256;
}

image pgm::read(const std::filesystem::path load_path)
{
    //im not parsing comments because NO
    std::ifstream read_stream(load_path, std::ios::binary);

    std::array<char, 2> data_type;
    read_stream.read(data_type.data(), 2);

    image img;
    if(strncmp(data_type.data(), "P2", 2)==0||strncmp(data_type.data(), "P5", 2)==0)
    {
        std::string width_string;
        read_stream >> width_string;
        img.width = std::stoul(width_string);

        std::string height_string;
        read_stream >> height_string;
        img.height = std::stoul(height_string);

        const int image_size = img.width*img.height;
        img.data.reserve(image_size);

        std::string max_val_string;
        read_stream >> max_val_string;
        const float max_val = std::stof(max_val_string);

        if(strncmp(data_type.data(), "P2", 2)==0)
        {
            //ascii
            for(int i = 0; i < image_size; ++i)
            {
                std::string color_string;
                read_stream >> color_string;

                img.data.emplace_back(std::round(255*(std::stoi(color_string)/max_val)));
            }
        } else
        {
            //binary
            for(int i = 0; i < image_size; ++i)
            {
                img.data.emplace_back(std::round(255*(read_stream.get()/max_val)));
            }
        }
    } else
    {
        throw std::runtime_error("pgm::read wrong magic numbers");
    }

    img.bpp = 1;

    return img;
}

void pgm::save(const image& img, const std::filesystem::path save_path)
{
    if(img.bpp!=1)
        return;

    std::ofstream out_stream(save_path, std::ios::binary);

    std::vector<char> image_header;
    //magic number for the binary grayscale format
    image_header.push_back('P');
    image_header.push_back('5');

    image_header.push_back('\n');

    const std::string width_string = std::to_string(img.width);
    image_header.insert(image_header.end(), width_string.begin(), width_string.end());

    image_header.push_back('\n');

    const std::string height_string = std::to_string(img.height);
    image_header.insert(image_header.end(), height_string.begin(), height_string.end());

    image_header.push_back('\n');

    //max value for a color which is 255
    image_header.push_back('2');
    image_header.push_back('5');
    image_header.push_back('5');

    image_header.push_back('\n');

    out_stream.write(image_header.data(), image_header.size());


    const int image_size = img.data.size();

    std::vector<char> image_data;
    //each color value is separated by a whitespace (therefore twice the bytes)
    image_data.reserve(image_size);

    for(int i = 0; i < image_size; ++i)
    {
        image_data.emplace_back(static_cast<char>(img.data[i]));
    }

    out_stream.write(image_data.data(), image_data.size());
}

void ppm::save(const image& img, const std::filesystem::path save_path)
{
    if(img.bpp!=3)
        return;

    std::ofstream out_stream(save_path, std::ios::binary);

    std::vector<char> image_header;
    //magic number for the binary rgb format
    image_header.push_back('P');
    image_header.push_back('6');

    image_header.push_back('\n');

    const std::string width_string = std::to_string(img.width);
    image_header.insert(image_header.end(), width_string.begin(), width_string.end());

    image_header.push_back('\n');

    const std::string height_string = std::to_string(img.height);
    image_header.insert(image_header.end(), height_string.begin(), height_string.end());

    image_header.push_back('\n');

    //max value for a color which is 255
    image_header.push_back('2');
    image_header.push_back('5');
    image_header.push_back('5');

    image_header.push_back('\n');

    out_stream.write(image_header.data(), image_header.size());


    const int image_size = img.data.size();

    std::vector<char> image_data;
    //each color value is separated by a whitespace (therefore twice the bytes)
    image_data.reserve(image_size);

    for(int y = 0; y < img.height; ++y)
    {
        for(int x = 0; x < img.width; ++x)
        {
            for(uint8_t bppi = 0; bppi < img.bpp; ++bppi)
            {
                image_data.emplace_back(static_cast<char>(img.data[y*img.width*img.bpp+x*img.bpp+bppi]));
            }
        }
    }

    out_stream.write(image_data.data(), image_data.size());
}

std::vector<uint8_t> ydeflate::deflate(const std::vector<uint8_t>& input_data)
{
    std::vector<uint8_t> deflated_data;

    const uint8_t compression_info = (input_data[0]>>4);
    const uint8_t compression_method = (input_data[0]&0x0f);

    if(compression_method!=8 | compression_info>7)
    {
        //all deflate streams have 8 compression method (at the moment)
        return deflated_data;
    }

    const uint8_t compression_level = (input_data[1]>>6);
    const bool preset_dict = ((input_data[1]>>5)&0x1);

    const std::array<char, 4> checksum_str = {
        static_cast<char>(input_data[0]),
        static_cast<char>(input_data[1]), 0, 0};

    const unsigned checksum_num = chars_to_number<unsigned>(checksum_str.data());;

    if(checksum_num%31!=0)
    {
        //the checksum is just the 2 bytes being divisible by 31
        return deflated_data;
    }

    bool last_block = false;
    uint8_t compression_type;

    f_pos pos{2, 0};

    //2 first bytes are zlib stuff so start reading DEFLATE blocks at 2 offset
    //int might be too small for large data (gigabytes range)
    //but my implementation isnt even fast enough to read that much in a realistic amount of time
    while(!last_block)
    {
        last_block = ((input_data[pos.byte]>>pos.bit)&0x1);
        shift_offset(pos);

        compression_type = ((input_data[pos.byte]>>pos.bit)&0x1);
        shift_offset(pos);
        compression_type |= ((input_data[pos.byte]>>pos.bit)&0x1)<<1;
        shift_offset(pos);

        //00 - no compression
        //01 - compressed with fixed huffman codes
        //10 - compressed with dynamic huffman codes
        //11 - error
        switch(compression_type)
        {
            case 0:
                uncompressed_block(input_data, deflated_data, pos);
            break;

            case 1:
                static_huffman_block(input_data, deflated_data, pos);
            break;

            case 2:
                dynamic_huffman_block(input_data, deflated_data, pos);
            break;

            case 3:
            default:
                return deflated_data;
            break;
        }
    }

    return deflated_data;
}

void ydeflate::uncompressed_block(const std::vector<uint8_t>& input_vec, std::vector<uint8_t>& data_vec, f_pos& pos)
{
    if(pos.bit>0)
        ++pos.byte;

    pos.bit = 0; //skip the partial bytes

    const std::array<char, 4> data_length_str = {
        static_cast<char>(input_vec[pos.byte]),
        static_cast<char>(input_vec[pos.byte+1]), 0, 0};

    pos.byte+=4; //skip the one's complement of length
    unsigned data_length;
    std::memcpy(&data_length, data_length_str.data(), 4);

    data_vec.reserve(data_length);
    for(int d = 0; d < data_length; ++d)
    {
        data_vec.emplace_back(input_vec[pos.byte++]);
    }
}

void ydeflate::static_huffman_block(const std::vector<uint8_t>& input_vec, std::vector<uint8_t>& data_vec, f_pos& pos)
{
    while(true)
    {
        uint8_t check_bits = 0;
        for(int d = 0; d < 7; ++d)
        {
            check_bits |= (input_vec[pos.byte]>>pos.bit)&0x1;
            check_bits <<= 1;
            shift_offset(pos);
        }

        int out_bits;
        if(check_bits>0xc8)
        {
            //its 144-255
            check_bits |= (input_vec[pos.byte]>>pos.bit)&0x1;
            shift_offset(pos);

            out_bits = static_cast<unsigned>(check_bits);

            out_bits <<= 1;
            out_bits |= (input_vec[pos.byte]>>pos.bit)&0x1;
            shift_offset(pos);

            out_bits -= 0x100;

            data_vec.push_back(out_bits);
            continue;
        } else if(check_bits>0xbf)
        {
            //its 280-287
            check_bits |= (input_vec[pos.byte]>>pos.bit)&0x1;
            out_bits = check_bits + 0x58;

            shift_offset(pos);
        } else if(check_bits>0x2e)
        {
            //its 0-143
            check_bits |= (input_vec[pos.byte]>>pos.bit)&0x1;
            check_bits -= 0x30;

            shift_offset(pos);

            data_vec.push_back(check_bits);
            continue;
        } else
        {
            //its 256-279
            check_bits >>= 1;
            if(check_bits!=0)
                out_bits = check_bits + 0x100;
            else
                break;
        }

        const int read_length = huffman_length(input_vec, pos, out_bits);

        int start_distance = 0;
        for(int d = 0; d < 5; ++d)
        {
            start_distance <<= 1;
            start_distance |= (input_vec[pos.byte]>>pos.bit)&0x1;
            shift_offset(pos);
        }

        const int read_distance = huffman_distance(input_vec, pos, start_distance);

        data_vec.reserve(read_length);
        const unsigned deflate_data_end = data_vec.size();
        for(int rb = 0; rb < read_length; ++rb)
        {
            data_vec.emplace_back(data_vec[deflate_data_end-read_distance+rb]);
        }
    }
}

void ydeflate::dynamic_huffman_block(const std::vector<uint8_t>& input_vec, std::vector<uint8_t>& data_vec, f_pos& pos)
{
    int codes_length_total = read_forward(input_vec, pos, 5);
    codes_length_total += 257;

    int codes_distance_total = read_forward(input_vec, pos, 5);
    ++codes_distance_total;

    const int code_length = read_forward(input_vec, pos, 4) + 4;

    std::vector<int> length_codes_temp(19, 0);
    for(int lc = 0; lc < 19; ++lc)
    {
        if(lc<code_length)
            length_codes_temp[lc] = read_forward(input_vec, pos, 3);
    }

    //reorder the codes
    const std::vector<int> length_codes_vec = {length_codes_temp[3], length_codes_temp[17],
        length_codes_temp[15], length_codes_temp[13], length_codes_temp[11], length_codes_temp[9],
        length_codes_temp[7], length_codes_temp[5], length_codes_temp[4], length_codes_temp[6],
        length_codes_temp[8], length_codes_temp[10], length_codes_temp[12], length_codes_temp[14],
        length_codes_temp[16], length_codes_temp[18], length_codes_temp[0], length_codes_temp[1], length_codes_temp[2]};

    const std::vector<int> length_codes_bin = lengths_to_prefix(length_codes_vec).vec;

    const int lci_size = length_codes_bin.size();

    std::vector<int> codewords_vec;
    uint_fast16_t reading_bits = 0;
    uint_fast8_t currcode_length = 0;
    for(;codes_length_total!=0; reading_bits <<= 1)
    {
        reading_bits |= (input_vec[pos.byte]>>pos.bit)&0x1;
        shift_offset(pos);
        currcode_length++;

        int code_index = lci_size;
        for(int lci = 0; lci < lci_size; ++lci)
        {
            if(length_codes_bin[lci]==reading_bits)
            {
                code_index = lci;
                break;
            }
        }

        if(code_index<19 && length_codes_vec[code_index]==currcode_length)
        {
            if(code_index<16)
            {
                //code length 0-15
                codewords_vec.push_back(code_index);
                --codes_length_total;
            } else if(code_index==16)
            {
                //this code reads previous code length 3-6 times (2 extra bits to read)
                const int read_length = read_forward(input_vec, pos, 2) + 3;
                codewords_vec.insert(codewords_vec.end(), read_length, codewords_vec[codewords_vec.size()-1]);
                codes_length_total -= read_length;
            } else if(code_index==17)
            {
                //this code copies 0 code 3-10 times (3 extra bits to read)
                const int read_length = read_forward(input_vec, pos, 3) + 3;
                codewords_vec.insert(codewords_vec.end(), read_length, 0);
                codes_length_total -= read_length;
            } else
            {
                //this code copies 0 code 11-138 times (7 extra bits to read)
                const int read_length = read_forward(input_vec, pos, 7) + 11;
                codewords_vec.insert(codewords_vec.end(), read_length, 0);
                codes_length_total -= read_length;
            }

            currcode_length = 0;
            reading_bits = 0;
        }
    }

    const prefix_vector ret_vec = lengths_to_prefix(codewords_vec);
    const std::vector<int> codewords_code_vec = ret_vec.vec;
    const uint8_t max_codeword_code_length = ret_vec.highest_length;

    //copy pasting :(
    std::vector<int> distance_code_temp;
    reading_bits = 0;
    currcode_length = 0;
    for(;codes_distance_total!=0; reading_bits <<= 1)
    {
        reading_bits |= (input_vec[pos.byte]>>pos.bit)&0x1;
        shift_offset(pos);
        ++currcode_length;

        int code_index = lci_size;
        for(int lci = 0; lci < lci_size; ++lci)
        {
            if(length_codes_bin[lci]==reading_bits)
            {
                code_index = lci;
                break;
            }
        }

        if(code_index<19 && length_codes_vec[code_index]==currcode_length)
        {
            int read_length = 0;
            if(code_index<16)
            {
                //code length 0-15
                distance_code_temp.push_back(code_index);
                --codes_distance_total;
            } else if(code_index==16)
            {
                //this code reads previous code length 3-6 times (2 extra bits to read)
                read_length = read_forward(input_vec, pos, 2);
                read_length += 0x3;

                distance_code_temp.insert(distance_code_temp.end(),
                                         read_length, distance_code_temp[distance_code_temp.size()-1]);

                codes_distance_total -= read_length;
            } else if(code_index==17)
            {
                //this code copies 0 code 3-10 times (3 extra bits to read)
                read_length = read_forward(input_vec, pos, 3);
                read_length += 0x3;

                distance_code_temp.insert(distance_code_temp.end(), read_length, 0);

                codes_distance_total -= read_length;
            } else
            {
                //this code copies 0 code 11-138 times (7 extra bits to read)
                read_length = read_forward(input_vec, pos, 7);
                read_length += 0xb;

                distance_code_temp.insert(distance_code_temp.end(), read_length, 0);

                codes_distance_total -= read_length;
            }

            currcode_length = 0;
            reading_bits = 0;
        }
    }

    const std::vector<int> distance_code_length_vec = distance_code_temp;

    const prefix_vector temp_ret_vec = lengths_to_prefix(distance_code_temp);
    const std::vector<int> distance_code_vec = temp_ret_vec.vec;
    const uint8_t max_distance_code_length = temp_ret_vec.highest_length;

    const int cv_size = codewords_code_vec.size();

    std::vector<uint_fast8_t> codewords_length_code_size(max_codeword_code_length+1);
    std::vector<std::vector<uint_fast16_t>> codewords_length_code_lookup(max_codeword_code_length+1);

    codewords_length_code_size[0] = 0;
    codewords_length_code_lookup[0] = std::vector<uint_fast16_t>();
    for(int clci = 1; clci <= max_codeword_code_length; ++clci)
    {
        std::vector<uint_fast16_t> inner_vec;
        for(int clcii = 0; clcii < cv_size; ++clcii)
        {
            if(codewords_vec[clcii]==clci)
            {
                inner_vec.push_back(clcii);
            }
        }

        codewords_length_code_size[clci] = static_cast<uint_fast8_t>(inner_vec.size());
        codewords_length_code_lookup[clci] = inner_vec;
    }

    const int dcv_size = distance_code_length_vec.size();

    std::vector<std::vector<uint_fast16_t>> distance_length_code_lookup(max_distance_code_length+1);

    distance_length_code_lookup[0] = std::vector<uint_fast16_t>();
    for(int dlci = 1; dlci <= max_distance_code_length; ++dlci)
    {
        std::vector<uint_fast16_t> inner_vec;
        for(int dlcii = 0; dlcii < dcv_size; ++dlcii)
        {
            if(distance_code_length_vec[dlcii]==dlci)
            {
                inner_vec.push_back(dlcii);
            }
        }

        distance_length_code_lookup[dlci] = inner_vec;
    }

    uint_fast8_t currcode_length_l = 0;
    uint_fast16_t reading_bits_l = 0;

    for(;; reading_bits_l <<= 1)
    {
        reading_bits_l |= (input_vec[pos.byte]>>pos.bit)&0x1;

        ++pos.bit;
        if(((pos.bit)&0x8)>>3)
        {
            ++pos.byte;
            pos.bit &= 0x7;
        }
        ++currcode_length_l;

        int_fast16_t code_index = cv_size;

        const uint_fast8_t c_len_lookup_length = codewords_length_code_size[currcode_length_l];
        for(uint_fast8_t cci = 0; cci < c_len_lookup_length; ++cci)
        {
            if(codewords_code_vec[codewords_length_code_lookup[currcode_length_l][cci]]==reading_bits_l)
            {
                code_index = codewords_length_code_lookup[currcode_length_l][cci];
                break;
            }
        }

        if(code_index!=cv_size)
        {
            if(code_index<256)
            {
                data_vec.push_back(static_cast<uint8_t>(code_index));
            } else
            {
                if(code_index==256)
                    break;

                const int read_length = huffman_length(input_vec, pos, code_index);

                uint8_t c_inner_bits_length = 0;
                int temp_distance = 0;
                for(;; temp_distance <<= 1)
                {
                    temp_distance |= (input_vec[pos.byte]>>pos.bit)&0x1;
                    shift_offset(pos);
                    ++c_inner_bits_length;

                    int distance_index = dcv_size;

                    const int c_len_lookup_length = distance_length_code_lookup[c_inner_bits_length].size();
                    for(int dci = 0; dci < c_len_lookup_length; ++dci)
                    {
                        if(distance_code_vec[distance_length_code_lookup[c_inner_bits_length][dci]]==temp_distance)
                        {
                            distance_index = distance_length_code_lookup[c_inner_bits_length][dci];
                            break;
                        }
                    }

                    if(distance_index!=dcv_size)
                    {
                        temp_distance = distance_index;
                        break;
                    }
                }

                const int read_distance = huffman_distance(input_vec, pos, temp_distance);

                data_vec.reserve(read_length);
                const unsigned deflate_data_end = data_vec.size();
                for(int rb = 0; rb < read_length; ++rb)
                {
                    data_vec.emplace_back(data_vec[deflate_data_end-read_distance+rb]);
                }
            }

            currcode_length_l = 0;
            reading_bits_l = 0;
        }
    }
}

int ydeflate::huffman_distance(const std::vector<uint8_t>& input_vec, f_pos& pos, int input_bits)
{
    const int extra_distance_bits = std::max(0, (input_bits-0x2)) / 2;

    const int mover_val = input_bits-5;

    const int loop_range = mover_val/2+1;
    for(int mli = 0; mli < loop_range; ++mli)
    {
        input_bits += (mover_val+1-(2*mli))*(1<<mli);
    }

    ++input_bits;

    for(int ext = 0; ext < extra_distance_bits; ++ext)
    {
        input_bits += ((input_vec[pos.byte]>>pos.bit)&0x1)<<ext;
        shift_offset(pos);
    }

    return input_bits;
}

int ydeflate::huffman_length(const std::vector<uint8_t>& input_vec, f_pos& pos, const int input_bits)
{
    const int extra_length_bits = std::max(0, (input_bits-0x105)) / 4;
    switch(extra_length_bits)
    {
        case 0:
            return input_bits-0xfe;

        case 6:
            return 258;

        default:
            const int mover_val = input_bits-0x10a;
            const int loop_range = mover_val/4+1;

            int read_length = std::max(0, input_bits-0xfe);
            for(int mli = 0; mli < loop_range; ++mli)
            {
                read_length += (mover_val+1-(4*mli))*(1<<mli);
            }

            for(int ext = 0; ext < extra_length_bits; ++ext)
            {
                read_length += ((input_vec[pos.byte]>>pos.bit)&0x1)<<ext;
                shift_offset(pos);
            }

            return read_length;
    }
}

void ydeflate::shift_offsets(f_pos& pos, int num)
{
    pos.bit += num;
    if(pos.bit>7)
    {
        ++pos.byte;
        pos.bit &= 0x7;
    }
}

void ydeflate::shift_offset(f_pos& pos)
{
    ++pos.bit;
    pos.byte += ((pos.bit)&0x8)>>3;
    pos.bit &= 0x7;
}

int ydeflate::read_forward(const std::vector<uint8_t>& input_vec, f_pos& pos, int num)
{
    const int ret_val =
    ((((input_vec[pos.byte]>>pos.bit)&((2<<(num-1))-1))
    | ((input_vec[pos.byte+1]<<(8-pos.bit))&((2<<(num-1))-1))));
    shift_offsets(pos, num);
    return ret_val;
}

std::vector<uint8_t> ydeflate::inflate(const std::vector<uint8_t>& input_data)
{
    std::vector<uint8_t> inflated_data;

    const uint8_t compression_method = 8; //the only one that exists atm
    const uint8_t compression_info = 7; //max window size, not even gonna try calculating it properly

    const uint8_t cmf_data = (compression_info<<4)|(compression_method&0x0f);
    inflated_data.push_back(cmf_data);

    uint8_t check_val = 0; //has to be that (cmf_data*256 + flg_data) % 31 == 0
    const uint8_t dict_set = 0; //i dont know how enabling it helps me in any way
    const uint8_t compression_level = 0; //i clearly used the least compressed one there is because its made by me

    uint8_t flg_data = ((compression_level&0x3)<<6)|((dict_set&0x1)<<5)|(check_val&0x1f);

    const uint8_t crc_remainder = (static_cast<int>(cmf_data)*256+static_cast<int>(flg_data))%31;
    //make it mod 31 == 0
    if(crc_remainder!=0)
    {
        flg_data |= (31-crc_remainder)&0x1f;
    }
    inflated_data.push_back(flg_data);

    bool last_block = false;

    const int max_distance = 32768;

    for(int i = 0; !last_block;)
    {
        //0 = no compression
        //1 = static huffman tree
        //2 = dynamic huffman tree
        const uint8_t encoding_method = 0;

        const int block_size = std::min(static_cast<int>(input_data.size()-i), 0xffff);

        const int next_block_edge = i+block_size;

        int max_bound = next_block_edge;
        if(next_block_edge >= input_data.size())
        {
            last_block = true;
            max_bound = input_data.size();
        }

        const uint8_t bit_info = ((encoding_method&0x3)<<1)|(static_cast<uint8_t>(last_block));

        switch(encoding_method)
        {
            case 0:
            {
                inflated_data.push_back(bit_info);

                //block length
                inflated_data.push_back(block_size&0xff);
                inflated_data.push_back((block_size&0xff00)>>8);

                //ones complement of block length
                inflated_data.push_back((block_size&0xff)^0xff);
                inflated_data.push_back(((block_size&0xff00)>>8)^0xff);

                inflated_data.reserve(block_size);
                for(;i < max_bound; ++i)
                {
                    inflated_data.emplace_back(input_data[i]);
                }
                break;
            }
        }
    }

    const uint32_t adler_mod = 65521;

    uint32_t adler_sum1 = 1;
    uint32_t adler_sum2 = 0;

    //how inefficient ;-;
    //dont care
    for(const auto& byte : input_data)
    {
        adler_sum1 = (adler_sum1 + byte) % adler_mod;
        adler_sum2 = (adler_sum2 + adler_sum1) % adler_mod;
    }

    inflated_data.push_back((adler_sum2&0xff00)>>8);
    inflated_data.push_back(adler_sum2&0xff);

    inflated_data.push_back((adler_sum1&0xff00)>>8);
    inflated_data.push_back(adler_sum1&0xff);

    return inflated_data;
}

template <typename T>
T ydeflate::chars_to_number(const char* val)
{
    T temp_val;

    T return_val = 0;
    std::memcpy(&temp_val, val, sizeof(T));

    uint8_t order_byte = 1;
    T byte_mask = 0xff;
    bool right_move = true;

    for(int i = 0; i < sizeof(T); ++i)
    {
        if(right_move)
        {
            return_val |= ((temp_val >> 8*(sizeof(T)-order_byte)) & byte_mask);
        } else
        {
            return_val |= ((temp_val << 8*(sizeof(T)-order_byte)) & byte_mask);
        }
        order_byte += right_move ? 2 : -2;
        right_move = right_move && (order_byte < sizeof(T));
        order_byte = order_byte > sizeof(T) ? sizeof(T)-1 : order_byte;
        byte_mask = byte_mask << 8;
    }

    return return_val;
}

ydeflate::prefix_vector ydeflate::lengths_to_prefix(const std::vector<int>& lengths_vec)
{
    const int max_val = INT_MAX; //hardcoded, who cares

    std::vector<uint8_t> counts_vec(lengths_vec.size(), 0);

    uint8_t max_bit_length = 0;
    for(const auto& c_length : lengths_vec)
    {
        ++counts_vec[c_length];
        if(max_bit_length<c_length)
            max_bit_length = c_length;
    }

    int code = 0;
    counts_vec[0] = 0;
    std::vector<int> code_lengths(max_bit_length+1, 0);
    for(int i = 1; i <= max_bit_length; ++i)
    {
        code = (code+counts_vec[i-1])<<1;
        code_lengths[i] = code;
    }

    std::vector<int> return_vec(lengths_vec.size(), max_val);

    const int lengths_vec_size = lengths_vec.size();
    for(int i = 0; i < lengths_vec_size; ++i)
    {
        const int c_length = lengths_vec[i];
        if(c_length!=0)
        {
            return_vec[i] = code_lengths[c_length];
            ++code_lengths[c_length];
        }
    }

    return {return_vec, max_bit_length};
}
