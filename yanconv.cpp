#include <cmath>
#include <cstring>
#include <climits>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <tuple>
#include <filesystem>

#include "yanconv.h"

using namespace yandereconv;

std::vector<std::string> yandereconv::string_split(std::string text, std::string delimeter)
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

template <typename T>
void yandereconv::chars_to_number(char* val, T* r_val)
{
    T temp_val;

    *r_val = 0;
    memcpy(&temp_val, val, sizeof(T));

    uint8_t order_byte = 1;
    T byte_mask = 0xff;
    bool right_move = true;

    for(int i = 0; i < sizeof(T); ++i)
    {
        if(right_move)
        {
            *r_val |= ((temp_val >> 8*(sizeof(T)-order_byte)) & byte_mask);
        } else
        {
            *r_val |= ((temp_val << 8*(sizeof(T)-order_byte)) & byte_mask);
        }
        order_byte += right_move ? 2 : -2;
        right_move = right_move && (order_byte < sizeof(T));
        order_byte = order_byte > sizeof(T) ? sizeof(T)-1 : order_byte;
        byte_mask = byte_mask << 8;
    }
}

template <typename T>
std::vector<T> yandereconv::lengths_to_prefix(std::vector<T> lengths_vec, T max_val, uint8_t* highest_length)
{
    size_t lengths_vec_size = lengths_vec.size(); //this could be not big enough if the max length in the vector is bigger than its size
    std::vector<uint8_t> counts_vec(lengths_vec_size, 0);

    uint8_t max_bit_length = 0;
    for(const auto& c_length : lengths_vec)
    {
        counts_vec[c_length]++;
        if(max_bit_length<c_length) max_bit_length = c_length;
    }

    if(highest_length!=nullptr) *highest_length = max_bit_length;

    int code = 0;
    counts_vec[0] = 0;
    std::vector<int> code_lengths(max_bit_length+1, 0);
    for(int i = 1; i <= max_bit_length; ++i)
    {
        code = (code+counts_vec[i-1])<<1;
        code_lengths[i] = code;
    }

    std::vector<T> r_vec(lengths_vec_size, max_val);
    for(int i = 0; i < lengths_vec_size; ++i)
    {
        int c_length = lengths_vec[i];
        if(c_length!=0)
        {
            r_vec[i] = code_lengths[c_length];
            code_lengths[c_length]++;
        }
    }

    return r_vec;
}

yandere_image::yandere_image() : image({})
{
}

yandere_image::yandere_image(std::string image_path) : _image_path(image_path)
{
    if(!read(image_path))
    {
    	throw std::runtime_error("cant read " + image_path);
    }
}

std::vector<uint8_t> yandere_image::yan_deflate(std::vector<uint8_t>& input_data)
{
    std::vector<uint8_t> deflated_data;

    uint8_t compression_info = (input_data[0]>>4);
    uint8_t compression_method = (input_data[0]&0x0f);

    if(compression_method!=8 | compression_info>7)
    {
        //all deflate streams have 8 compression method (at the moment)
        return std::move(deflated_data);
    }

    uint8_t compression_level = (input_data[1]>>6);
    bool preset_dict = ((input_data[1]>>5)&0x1);

    char checksum_str[4] = {};
    checksum_str[0] = static_cast<char>(input_data[0]);
    checksum_str[1] = static_cast<char>(input_data[1]);
    checksum_str[2] = 0;
    checksum_str[3] = 0;

    unsigned checksum_num;
    chars_to_number<unsigned>(checksum_str, &checksum_num);

    if(checksum_num%31!=0)
    {
        //the checksum is just the 2 bytes being divisible by 31
        return std::move(deflated_data);
    }

    bool last_block = false;
    uint8_t compression_type;
    
    uint_fast8_t bit_offset = 0;
	int i = 2;
	
	auto shift_offsets = [&bit_offset, &i](uint_fast8_t num) mutable { bit_offset+=num; if(bit_offset>7) { ++i; bit_offset &= 0x7;} };
	auto shift_offset = [&bit_offset, &i]() mutable {++bit_offset; i += ((bit_offset)&0x8)>>3; bit_offset &= 0x7;};
	auto read_forward = [&bit_offset, &i, &input_data, &shift_offsets](uint_fast8_t num) mutable -> unsigned {auto retVal = ((((input_data[i]>>bit_offset)&((2<<(num-1))-1)) | ((input_data[i+1]<<(8-bit_offset))&((2<<(num-1))-1)))); shift_offsets(num); return retVal;};
    //2 first bytes are zlib stuff so start reading DEFLATE blocks at 2 offset
    //int might be too small for large data (gigabytes range)
    //but my implementation isnt even fast enough to read that much in a realistic amount of time
    while(!last_block)
    {
        last_block = ((input_data[i]>>bit_offset)&0x1);
        shift_offset();

        compression_type = ((input_data[i]>>bit_offset)&0x1);
        shift_offset();
        compression_type |= ((input_data[i]>>bit_offset)&0x1)<<1;
        shift_offset();

        //00 - no compression
        //01 - compressed with fixed huffman codes
        //10 - compressed with dynamic huffman codes
        //11 - error
        if(compression_type==0)
        {
            if(bit_offset>0) ++i;
            bit_offset = 0; //skip the partial bytes
            char data_length_str[4] = {};
            data_length_str[0] = static_cast<char>(input_data[i]);
            data_length_str[1] = static_cast<char>(input_data[i+1]);
            data_length_str[2] = 0;
            data_length_str[3] = 0;
            i+=4; //skip the one's complement of length
            unsigned data_length;
            memcpy(&data_length, data_length_str, 4);

            deflated_data.reserve(data_length);
            for(int d = 0; d < data_length; ++d)
            {
                deflated_data.push_back(input_data[i++]);
            }
        } else
        {
            int read_length = 0;
            int read_distance = 0;

            if(compression_type==3)
            {
                return std::move(deflated_data);
            } else if(compression_type==2)
            {
                unsigned codes_length_total = static_cast<unsigned>(read_forward(5));
                codes_length_total += 257;

                unsigned codes_distance_total = static_cast<unsigned>(read_forward(5));
                ++codes_distance_total;

                unsigned code_length = static_cast<unsigned>(read_forward(4));
                code_length += 4;

                std::vector<unsigned> length_codes_bin;
                length_codes_bin.reserve(19);
                for(int lc = 0; lc < 19; ++lc)
                {
                    uint8_t length_code;
                    if(lc<code_length)
                    {
                        length_code = read_forward(3);
                    } else
                    {
                        length_code = 0;
                    }

                    length_codes_bin.push_back(length_code);
                }

                //reorder the codes
                length_codes_bin = {length_codes_bin[3], length_codes_bin[17], length_codes_bin[15], length_codes_bin[13], length_codes_bin[11], length_codes_bin[9], length_codes_bin[7], length_codes_bin[5], length_codes_bin[4], length_codes_bin[6], length_codes_bin[8], length_codes_bin[10], length_codes_bin[12], length_codes_bin[14], length_codes_bin[16], length_codes_bin[18], length_codes_bin[0], length_codes_bin[1], length_codes_bin[2]};

                std::vector<unsigned> length_codes_vec = length_codes_bin;
                length_codes_bin = lengths_to_prefix<unsigned>(length_codes_bin, UINT_MAX, nullptr);

                size_t lci_size = length_codes_bin.size();

                std::vector<unsigned> codewords_vec;
                uint_fast16_t reading_bits = 0;
                uint_fast8_t currcode_length = 0;
                for(;codes_length_total!=0; reading_bits <<= 1)
                {
                    reading_bits |= (input_data[i]>>bit_offset)&0x1;
                    shift_offset();
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
                        read_length = 0;
                        if(code_index<16)
                        {
                            //code length 0-15
                            codewords_vec.push_back(code_index);
                            --codes_length_total;
                        } else if(code_index==16)
                        {
                            //this code reads previous code length 3-6 times (2 extra bits to read)
                            read_length = read_forward(2);
                            read_length += 0x3;
                            uint8_t prevcode_length = codewords_vec[codewords_vec.size()-1];
                            codewords_vec.insert(codewords_vec.end(), read_length, prevcode_length);
                            codes_length_total -= read_length;
                        } else if(code_index==17)
                        {
                            //this code copies 0 code 3-10 times (3 extra bits to read)
                            read_length = read_forward(3);
                            read_length += 0x3;
                            codewords_vec.insert(codewords_vec.end(), read_length, 0);
                            codes_length_total -= read_length;
                        } else
                        {
                            //this code copies 0 code 11-138 times (7 extra bits to read)
                            read_length = read_forward(7);
                            read_length += 0xb;
                            codewords_vec.insert(codewords_vec.end(), read_length, 0);
                            codes_length_total -= read_length;
                        }

                        currcode_length = 0;
                        reading_bits = 0;
                    }
                }

                std::vector<unsigned> codewords_code_vec = codewords_vec;
                uint8_t max_codeword_code_length;
                codewords_code_vec = lengths_to_prefix<unsigned>(codewords_code_vec, UINT_MAX, &max_codeword_code_length);

                //copy pasting :(
                std::vector<unsigned> distance_code_vec;
                reading_bits = 0;
                currcode_length = 0;
                for(;codes_distance_total!=0; reading_bits <<= 1)
                {
                    reading_bits |= (input_data[i]>>bit_offset)&0x1;
                    shift_offset();
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
                        read_length = 0;
                        if(code_index<16)
                        {
                            //code length 0-15
                            distance_code_vec.push_back(code_index);
                            --codes_distance_total;
                        } else if(code_index==16)
                        {
                            //this code reads previous code length 3-6 times (2 extra bits to read)
                            read_length = read_forward(2);
                            read_length += 0x3;
                            uint8_t prevcode_length = distance_code_vec[distance_code_vec.size()-1];
                            distance_code_vec.insert(distance_code_vec.end(), read_length, prevcode_length);
                            codes_distance_total -= read_length;
                        } else if(code_index==17)
                        {
                            //this code copies 0 code 3-10 times (3 extra bits to read)
                            read_length = read_forward(3);
                            read_length += 0x3;
                            distance_code_vec.insert(distance_code_vec.end(), read_length, 0);
                            codes_distance_total -= read_length;
                        } else
                        {
                            //this code copies 0 code 11-138 times (7 extra bits to read)
                            read_length = read_forward(7);
                            read_length += 0xb;
                            distance_code_vec.insert(distance_code_vec.end(), read_length, 0);
                            codes_distance_total -= read_length;
                        }

                        currcode_length = 0;
                        reading_bits = 0;
                    }
                }

                std::vector<unsigned> distance_code_length_vec = distance_code_vec;

                uint8_t max_distance_code_length;
                distance_code_vec = lengths_to_prefix<unsigned>(distance_code_vec, UINT_MAX, &max_distance_code_length);

                size_t cv_size = codewords_code_vec.size();

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

                size_t dcv_size = distance_code_length_vec.size();

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
                    reading_bits_l |= (input_data[i]>>bit_offset)&0x1;

                    ++bit_offset;
                    if(((bit_offset)&0x8)>>3)
                    {
                        ++i;
                        bit_offset &= 0x7;
                    }
                    ++currcode_length_l;

                    int_fast16_t code_index = cv_size;

                    uint_fast8_t c_len_lookup_length = codewords_length_code_size[currcode_length_l];
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
                            deflated_data.push_back(static_cast<uint8_t>(code_index));
                        } else
                        {
                            if(code_index==256)
                                break;

                            //i am addicted to copy pasting
                            //FUCK YOU THIS CODE IS GARBAGE
                            read_length = code_index-0xfe;
                            int extra_length_bits = ((code_index-0x105)/4);

                            if(extra_length_bits!=0)
                            {
                                if(extra_length_bits==6)
                                {
                                    extra_length_bits = 0;
                                    read_length = 258;
                                } else
                                {
                                    read_length = code_index-0xfe;
                                    read_length = read_length<0 ? 0 : read_length;

                                    int mover_val = code_index-0x10a;

                                    int loop_range = mover_val/4+1;
                                    for(int mli = 0; mli < loop_range; ++mli)
                                    {
                                        read_length += (mover_val+1-(4*mli))*(1<<mli);
                                    }
                                }
                            }

                            for(int ext = 0; ext < extra_length_bits; ++ext)
                            {
                                read_length += ((input_data[i]>>bit_offset)&0x1)<<ext;
                                shift_offset();
                            }

                            uint8_t c_inner_bits_length = 0;
                            read_distance = 0;
                            for(;; read_distance <<= 1)
                            {
                                read_distance |= (input_data[i]>>bit_offset)&0x1;
                                shift_offset();
                                ++c_inner_bits_length;

                                int distance_index = dcv_size;

                                size_t c_len_lookup_length = distance_length_code_lookup[c_inner_bits_length].size();
                                for(int dci = 0; dci < c_len_lookup_length; ++dci)
                                {
                                    if(distance_code_vec[distance_length_code_lookup[c_inner_bits_length][dci]]==read_distance)
                                    {
                                        distance_index = distance_length_code_lookup[c_inner_bits_length][dci];
                                        break;
                                    }
                                }

                                if(distance_index!=dcv_size)
                                {
                                    read_distance = distance_index+1;
                                    break;
                                }
                            }

                            int extra_distance_bits = ((read_distance-0x3)/2);

                            int mover_val = read_distance-0x6;

                            int loop_range = mover_val/2+1;
                            for(int mli = 0; mli < loop_range; ++mli)
                            {
                                read_distance += (mover_val+1-(2*mli))*(1<<mli);
                            }

                            for(int ext = 0; ext < extra_distance_bits; ++ext)
                            {
                                read_distance += ((input_data[i]>>bit_offset)&0x1)<<ext;
                                shift_offset();
                            }

                            deflated_data.reserve(read_length);
                            unsigned deflate_data_end = deflated_data.size();
                            for(int rb = 0; rb < read_length; ++rb)
                            {
                                deflated_data.emplace_back(deflated_data[deflate_data_end-read_distance+rb]);
                            }
                        }

                        currcode_length_l = 0;
                        reading_bits_l = 0;
                    }
                }
             } else
             {
                while(true)
                {
                    uint8_t check_bits = 0;
                    for(int d = 0; d < 7; ++d)
                    {
                        check_bits |= (input_data[i]>>bit_offset)&0x1;
                        check_bits <<= 1;
                        shift_offset();
                    }

                    int out_bits;
                    if(check_bits>0xc8)
                    {
                        //its 144-255
                        unsigned out_bits;
                        check_bits |= (input_data[i]>>bit_offset)&0x1;
                        shift_offset();

                        out_bits = static_cast<unsigned>(check_bits);

                        out_bits <<= 1;
                        out_bits |= (input_data[i]>>bit_offset)&0x1;
                        shift_offset();

                        out_bits -= 0x100;

                        deflated_data.push_back(out_bits);
                        continue;
                    } else if(check_bits>0xbf)
                    {
                        //its 280-287
                        check_bits |= (input_data[i]>>bit_offset)&0x1;
                        out_bits = check_bits + 0x58;

                        shift_offset();
                    } else if(check_bits>0x2e)
                    {
                        //its 0-143
                        check_bits |= (input_data[i]>>bit_offset)&0x1;
                        check_bits -= 0x30;

                        shift_offset();

                        deflated_data.push_back(check_bits);
                        continue;
                    } else
                    {
                        //its 256-279
                        check_bits >>= 1;
                        if(check_bits==0)
                        {
                            break;
                        } else
                        {
                            out_bits = check_bits + 0x100;
                        }
                    }

                    read_length = out_bits-0xfe;
                    int clamped_bits = (out_bits-0x105);
                    clamped_bits = clamped_bits<0 ? 0 : clamped_bits;
                    uint8_t extra_length_bits = static_cast<uint8_t>(clamped_bits/4);

                    if(extra_length_bits==6)
                    {
                        extra_length_bits = 0;
                        read_length = 258;
                    } else if(extra_length_bits!=0)
                    {
                        read_length = out_bits-0xfe;
                        read_length = read_length<0 ? 0 : read_length;

                        int mover_val = out_bits-0x10a;

                        int loop_range = mover_val/4+1;
                        for(int mli = 0; mli < loop_range; ++mli)
                        {
                            read_length += (mover_val+1-(4*mli))*(1<<mli);
                        }
                    }

                    for(int ext = 0; ext < extra_length_bits; ++ext)
                    {
                        read_length += ((input_data[i]>>bit_offset)&0x1)<<ext;
                        shift_offset();
                    }

                    read_distance = 0;
                    for(int d = 0; d < 5; ++d)
                    {
                        read_distance <<= 1;
                        read_distance |= (input_data[i]>>bit_offset)&0x1;
                        shift_offset();
                    }

                    clamped_bits = (read_distance-0x2);
                    clamped_bits = clamped_bits<0 ? 0 : clamped_bits;
                    uint8_t extra_distance_bits = static_cast<uint8_t>(clamped_bits/2);

                    int mover_val = read_distance-0x5;

                    int loop_range = mover_val/2+1;
                    for(int mli = 0; mli < loop_range; ++mli)
                    {
                        read_distance += (mover_val+1-(2*mli))*(1<<mli);
                    }

                    read_distance++;

                    for(int ext = 0; ext < extra_distance_bits; ++ext)
                    {
                        read_distance += ((input_data[i]>>bit_offset)&0x1)<<ext;
                        shift_offset();
                    }

                    deflated_data.reserve(read_length);
                    unsigned deflate_data_end = deflated_data.size();
                    for(int rb = 0; rb < read_length; ++rb)
                    {
                        deflated_data.emplace_back(deflated_data[deflate_data_end-read_distance+rb]);
                    }
                }
            }
        }
    }

    return std::move(deflated_data);
}

uint8_t yandere_image::sub_positive(int lval, int rval)
{
    return lval + rval - 256;
}

bool yandere_image::png_read(bool do_checksums)
{
    std::ifstream input_stream(_image_path, std::ios::binary);

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

    while(input_stream.good())
    {
        //this is definitely not proper c++
        //THIS ISNT EVEN PROPER PROGRAMMING
        char chunk_length_str[4] = {};
        input_stream.read(chunk_length_str, 4);
        unsigned chunk_length;
        chars_to_number<unsigned>(chunk_length_str, &chunk_length);

        char chunk_type[4] = {};
        input_stream.read(chunk_type, 4);

        char* chunk_data = static_cast<char*>(malloc(chunk_length)); //char is always 1 byte long
        input_stream.read(chunk_data, chunk_length);

        char chunkChecksum[4] = {};
        input_stream.read(chunkChecksum, 4);

        if(do_checksums)
        {
            //too lazy
        }

        if(strncmp(chunk_type, "IHDR", 4)==0)
        {
            char image_width_str[4] = {};
            char image_height_str[4] = {};
            for(int i = 0; i < 4; i++)
            {
                image_width_str[i] = chunk_data[i];
                image_height_str[i] = chunk_data[i+4];
            }
            chars_to_number<unsigned>(image_width_str, &temp_width);
            chars_to_number<unsigned>(image_height_str, &temp_height);

            bit_depth = static_cast<uint8_t>(chunk_data[8]);
            uint8_t colorType = static_cast<uint8_t>(chunk_data[9]);

            pallete_used = colorType&0x1;
            rgb_used = (colorType>>1)&0x1;
            alpha_channel = (colorType>>2)&0x1;

            values_per_pixel = (2*rgb_used)+alpha_channel+1;

            bpp = std::ceil((values_per_pixel*bit_depth)/8.0f);
            vals_per_byte = 8/bit_depth;

            interlacing = static_cast<bool>(chunk_data[12]);
        } else if(strncmp(chunk_type, "IDAT", 4)==0)
        {
            deflate_stream.reserve(chunk_length);
            for(int i = 0; i < chunk_length; i++)
            {
                deflate_stream.push_back(chunk_data[i]);
            }
        } else if(strncmp(chunk_type, "PLTE", 4)==0)
        {
            if(chunk_length%3!=0)
            {
                free(chunk_data);
                input_stream.close();
                return false;
            }
            pallete_vector.reserve(chunk_length);
            for(int i = 0; i < chunk_length; i++)
            {
                pallete_vector.emplace_back(chunk_data[i]);
            }
        } else if(strncmp(chunk_type, "IEND", 4)==0)
        {
            std::vector<uint8_t> image_data = yan_deflate(deflate_stream);
            if(image_data.size()!=0)
            {
                temp_data.reserve(std::ceil(temp_width*temp_height*values_per_pixel/static_cast<float>(vals_per_byte)));

                unsigned stream_pos = 0;
                uint8_t read_bits = 0;

                for(int y = 0; y < temp_height; ++y)
                {
                    uint8_t filter_type = image_data[stream_pos++];

                    if(pallete_used)
                    {
                        for(int x = 0; x < temp_width; ++x, ++stream_pos)
                        {
                            unsigned pallete_pixel = image_data[stream_pos]*3;
                            temp_data.emplace_back(pallete_vector[pallete_pixel]);
                            temp_data.emplace_back(pallete_vector[pallete_pixel+1]);
                            temp_data.emplace_back(pallete_vector[pallete_pixel+2]);
                        }
                    } else
                    {
                        for(int x = 0; x < temp_width*values_per_pixel; ++x, ++stream_pos)
                        {
                            uint8_t pixel_data;

                            switch(filter_type)
                            {
                                case 0:
                                    pixel_data = image_data[stream_pos];
                                break;

                                case 1:
                                {
                                    if(x<bpp)
                                    {
                                        pixel_data = image_data[stream_pos];
                                    } else
                                    {
                                        pixel_data = sub_positive(image_data[stream_pos], image_data[stream_pos-bpp]);
                                        image_data[stream_pos] = pixel_data;
                                    }
                                    break;
                                }
                                case 2:
                                {
                                    if(y==0)
                                    {
                                        pixel_data = image_data[stream_pos];
                                    } else
                                    {
                                        unsigned above_index = stream_pos-(1+(temp_width*values_per_pixel)/vals_per_byte); //the +1 is to skip the filter type for current line
                                        pixel_data = sub_positive(image_data[stream_pos], image_data[above_index]);
                                        image_data[stream_pos] = pixel_data;
                                    }
                                    break;
                                }
                                case 3:
                                {
                                    unsigned above_index = stream_pos-(1+(temp_width*values_per_pixel)/vals_per_byte);
                                    uint8_t up_pixel = y==0 ? 0 : image_data[above_index];
                                    uint8_t left_pixel = x<bpp ? 0 : image_data[stream_pos-bpp];

                                    uint8_t right_side_pixel = (left_pixel+up_pixel)/2;
                                    pixel_data = sub_positive(image_data[stream_pos], right_side_pixel);
                                    image_data[stream_pos] = pixel_data;
                                    break;
                                }
                                case 4:
                                {
                                    unsigned above_index = stream_pos-(1+(temp_width*values_per_pixel)/vals_per_byte);
                                    uint8_t up_pixel = y==0 ? 0 : image_data[above_index];
                                    uint8_t left_pixel = x<bpp ? 0 : image_data[stream_pos-bpp];
                                    uint8_t up_left_pixel = ((y==0) | (x<bpp)) ? 0 : image_data[above_index-bpp];

                                    uint8_t right_side_pixel;

                                    int initial_est = left_pixel + up_pixel - up_left_pixel;
                                    int distance_left = std::abs(initial_est - left_pixel);
                                    int distance_up = std::abs(initial_est - up_pixel);
                                    int distance_up_left = std::abs(initial_est - up_left_pixel);

                                    if((distance_left<=distance_up)&&(distance_left<=distance_up_left))
                                    {
                                        right_side_pixel = left_pixel;
                                    } else if(distance_up<=distance_up_left)
                                    {
                                        right_side_pixel = up_pixel;
                                    } else
                                    {
                                        right_side_pixel = up_left_pixel;
                                    }

                                    pixel_data = sub_positive(image_data[stream_pos], right_side_pixel);
                                    image_data[stream_pos] = pixel_data;
                                    break;
                                }
                            }

                            temp_data.emplace_back(pixel_data);
                        }
                    }
                }
            }
            

            image = temp_data;
            width = temp_width;
            height = temp_height;

            free(chunk_data);
            break;
        }/* else
        {
            char newChunk[5] = {chunk_type[0], chunk_type[1], chunk_type[2], chunk_type[3], '\0'};
            printf("%s\n", newChunk);
        }*/

        free(chunk_data);
    }

    input_stream.close();

    return true;
}

std::vector<uint8_t> yandere_image::yan_inflate(std::vector<uint8_t>& input_data)
{
	std::vector<uint8_t> inflated_data;
	
	uint8_t compression_method = 8; //the only one that exists atm
	uint8_t compression_info = 7; //max window size, not even gonna try calculating it properly
	
	uint8_t cmf_data = (compression_info<<4)|(compression_method&0x0f);
	inflated_data.push_back(cmf_data);
	
	uint8_t check_val = 0; //has to be that (cmf_data*256 + flg_data) % 31 == 0
	uint8_t dict_set = 0; //i dont know how enabling it helps me in any way
	uint8_t compression_level = 0; //i clearly used the least compressed one there is because its made by me
	
	uint8_t flg_data = ((compression_level&0x3)<<6)|((dict_set&0x1)<<5)|(check_val&0x1f);
	
	uint8_t crc_remainder = (static_cast<int>(cmf_data)*256+static_cast<int>(flg_data))%31;
	//make it mod 31 == 0
	if(crc_remainder!=0)
	{
		flg_data |= (31-crc_remainder)&0x1f;
	}
	inflated_data.push_back(flg_data);
	
	int input_data_size = input_data.size();
	bool last_block = false;
	
	bool last_found = false;
	std::vector<uint8_t>::iterator last_found_iter;
	std::vector<uint8_t> search_vec;
	
	std::vector<uint8_t>::iterator found_iter;
	
	const int max_distance = 32768;
	auto first_iter = inflated_data.end();
	
	int last_inflated_data_size = 0;
	
	//this is really unreadable, soz
	uint8_t bit_offset = 0;
	for(int i = 0; !last_block;)
	{	
		//0 = no compression
		//1 = static huffman tree
		//2 = dynamic huffman tree
		uint8_t encoding_method = 0;
		
		int block_size = 0xff;
		switch(encoding_method)
		{
			case 0:
			{
				block_size = 0xff; //maximum size for uncompressed data block
			break;
			}
			case 1:
			{
				block_size = input_data_size-i; //just encode all of it idk
			break;
			}
		}
		
		int next_block_edge = i+block_size;
		if(next_block_edge >= input_data_size)
		{
			last_block = true;
		}
		int max_bound = std::min(next_block_edge, input_data_size);
		
		uint8_t bit_info = ((encoding_method&0x3)<<1)|(static_cast<uint8_t>(last_block));
		
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
				
				for(; i < max_bound; ++i)
				{		
					inflated_data.emplace_back(input_data[i]);
				}
			break;
			}
			case 1:
			{
				//todo (never)
			}
			break;
		}
	}
	
	int adler_sum1 = 1;
	int adler_sum2 = 0;
			
	//how inefficient ;-;
	//dont care
	for(int s = 0; s < input_data_size; ++s)
	{
		adler_sum1 += (input_data[s] % 65521);
		adler_sum2 += (adler_sum1 % 65521);
	}
			
	inflated_data.push_back((adler_sum2&0xff00)>>8);
	inflated_data.push_back(adler_sum2&0xff);
			
	inflated_data.push_back((adler_sum1&0xff00)>>8);
	inflated_data.push_back(adler_sum1&0xff);

	return std::move(inflated_data);
}

std::vector<uint32_t> yandere_image::crc_table_gen()
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

void yandere_image::png_save(std::string save_path)
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

    std::string width_string = std::to_string(width);
    image_header.insert(image_header.end(), width_string.begin(), width_string.end());

    std::string height_string = std::to_string(height);
    image_header.insert(image_header.end(), height_string.begin(), height_string.end());
    int image_size = image.size();

	std::vector<std::array<char,4>> encode_chunks;
	encode_chunks.push_back({'I', 'H', 'D', 'R'});
	encode_chunks.push_back({'I', 'D', 'A', 'T'});
	encode_chunks.push_back({'I', 'E', 'N', 'D'});
	
	int chunks_amount = encode_chunks.size();
	
	for(int i = 0; i < chunks_amount; ++i)
	{
		std::vector<char> data_chunk;
		
		std::string chunk_name = "";
		uint32_t chunk_length = 0;
		
		data_chunk.reserve(4);
		for(int en = 0; en < 4; ++en)
		{
			data_chunk.emplace_back(encode_chunks[i][en]);
			chunk_name += encode_chunks[i][en];
		}
		
		if(chunk_name=="IDAT")
		{
			std::vector<uint8_t> image_bytes;
			image_bytes.reserve(height*width*bpp+height);
			
			for(int y = 0; y < height; ++y)
			{
				uint8_t line_filter = 0;
				image_bytes.emplace_back(line_filter);
			
				for(int x = 0; x < width; ++x)
				{
					for(int bppi = 0; bppi < bpp; ++bppi)
					{
						image_bytes.emplace_back(image[y*width*bpp+x*bpp+bppi]);
					}
				}
			}
			
			std::vector<uint8_t> inflated_bytes = yan_inflate(image_bytes);
			
			data_chunk.insert(data_chunk.end(), inflated_bytes.begin(), inflated_bytes.end()); //no point trying to do this efficiently, have to convert from uint8_t to char
			
			chunk_length = inflated_bytes.size();
		} else if(chunk_name=="IHDR")
		{
			//4 bytes of width
			data_chunk.push_back((width&0xff000000)>>24);
			data_chunk.push_back((width&0xff0000)>>16);
			data_chunk.push_back((width&0xff00)>>8);
			data_chunk.push_back(width&0xff);
			
			//4 bytes of height
			data_chunk.push_back((height&0xff000000)>>24);
			data_chunk.push_back((height&0xff0000)>>16);
			data_chunk.push_back((height&0xff00)>>8);
			data_chunk.push_back(height&0xff);
			
			//bits per pixel, always 8 (bla bla bla not efficient)
			data_chunk.push_back(8);
			
			//color type
			//0100 bit adds alpha
			//0010 bit adds color
			//0001 bit means color from pallete
			if(bpp==1)
			{
				//grayscale
				data_chunk.push_back(0);
			} else if(bpp==2)
			{
				//grayscale with alpha
				data_chunk.push_back(4);
			} else if(bpp==3)
			{
				//fullcolor
				data_chunk.push_back(2);
			} else if(bpp==4)
			{
				//fullcolor with alpha
				data_chunk.push_back(6);
			} else
			{
				return;
			}
			
			//stuff that doesnt change, like png using deflate algorithm for compression
			data_chunk.push_back(0);
			data_chunk.push_back(0);
			
			//interlacing off
			data_chunk.push_back(0);
			
			chunk_length = 13;
		}
		
		//crc stuff
		std::vector<uint32_t> crc_table = crc_table_gen();
		
		uint32_t chunk_crc = 0xffffffff;
		
		int crc_length = chunk_length + 4; //4 letters of the chunk name
		
		for(int b = 0; b < crc_length; ++b)
		{
			chunk_crc = (crc_table[(chunk_crc^data_chunk[b])&0xff]^(chunk_crc>>8));
		}
		
		chunk_crc ^= 0xffffffff;
		//crc stuff over
		
		data_chunk.insert(data_chunk.begin(), chunk_length&0xff);
		data_chunk.insert(data_chunk.begin(), (chunk_length&0xff00)>>8);
		data_chunk.insert(data_chunk.begin(), (chunk_length&0xff0000)>>16);
		data_chunk.insert(data_chunk.begin(), (chunk_length&0xff000000)>>24);
		
		
		data_chunk.push_back((chunk_crc&0xff000000)>>24);
		data_chunk.push_back((chunk_crc&0xff0000)>>16);
		data_chunk.push_back((chunk_crc&0xff00)>>8);
		data_chunk.push_back(chunk_crc&0xff);

		out_stream.write(data_chunk.data(), data_chunk.size());
    }

    out_stream.close();
}

bool yandere_image::pgm_read()
{
    //im not parsing comments because NO
    std::ifstream read_stream(_image_path, std::ios::binary);

    char data_type[2];
    read_stream >> data_type;

    if(strncmp(data_type, "P2", 2)==0||strncmp(data_type, "P5", 2)==0)
    {
        std::string width_string;
        read_stream >> width_string;
        width = std::stoul(width_string);

        std::string height_string;
        read_stream >> height_string;
        height = std::stoul(height_string);

        int image_size = width*height;
        image.reserve(width*height);

        std::string max_val_string;
        read_stream >> max_val_string;
        float max_val = std::stof(max_val_string);

        if(strncmp(data_type, "P2", 2)==0)
        {
            //ascii
            for(int i = 0; i < image_size; ++i)
            {
                std::string color_string;
                read_stream >> color_string;

                image.emplace_back(std::round(255*(std::stoi(color_string)/max_val)));
            }
        } else
        {
            //binary
            char color_byte = 0;
            for(int i = 0; i < image_size; ++i)
            {
                read_stream.get(color_byte);

                image.emplace_back(std::round(255*(color_byte/max_val)));
            }
        }
    } else
    {

        read_stream.close();
        return false;
    }

    bpp = 1;

    read_stream.close();
    return true;
}

void yandere_image::pgm_save(std::string save_path)
{
    if(bpp!=1)
    {
        return;
    }

    std::ofstream out_stream(save_path, std::ios::binary);

    std::vector<char> image_header;
    //magic number for the binary grayscale format
    image_header.push_back('P');
    image_header.push_back('5');

    image_header.push_back('\n');

    std::string width_string = std::to_string(width);
    image_header.insert(image_header.end(), width_string.begin(), width_string.end());

    image_header.push_back('\n');

    std::string height_string = std::to_string(height);
    image_header.insert(image_header.end(), height_string.begin(), height_string.end());

    image_header.push_back('\n');

    //max value for a color which is 255
    image_header.push_back('2');
    image_header.push_back('5');
    image_header.push_back('5');

    image_header.push_back('\n');

    out_stream.write(image_header.data(), image_header.size());


    int image_size = image.size();

    std::vector<char> image_data;
    //each color value is separated by a whitespace (therefore twice the bytes)
    image_data.reserve(image_size);

    for(int i = 0; i < image_size; ++i)
    {
        image_data.emplace_back(static_cast<char>(image[i]));
    }

    out_stream.write(image_data.data(), image_data.size());

    out_stream.close();
}

void yandere_image::ppm_save(std::string save_path)
{
    if(bpp!=3)
    {
        return;
    }

    std::ofstream out_stream(save_path, std::ios::binary);

    std::vector<char> image_header;
    //magic number for the binary rgb format
    image_header.push_back('P');
    image_header.push_back('6');

    image_header.push_back('\n');

    std::string width_string = std::to_string(width);
    image_header.insert(image_header.end(), width_string.begin(), width_string.end());

    image_header.push_back('\n');

    std::string height_string = std::to_string(height);
    image_header.insert(image_header.end(), height_string.begin(), height_string.end());

    image_header.push_back('\n');

    //max value for a color which is 255
    image_header.push_back('2');
    image_header.push_back('5');
    image_header.push_back('5');

    image_header.push_back('\n');

    out_stream.write(image_header.data(), image_header.size());


    int image_size = image.size();

    std::vector<char> image_data;
    //each color value is separated by a whitespace (therefore twice the bytes)
    image_data.reserve(image_size);

	for(int y = 0; y < height; ++y)
	{
    	for(int x = 0; x < width; ++x)
    	{
    		for(uint8_t bppi = 0; bppi < bpp; ++bppi)
    		{
        		image_data.emplace_back(static_cast<char>(image[y*width*bpp+x*bpp+bppi]));
        	}
    	}
    }

    out_stream.write(image_data.data(), image_data.size());

    out_stream.close();
}

void yandere_image::grayscale()
{
    if(bpp==1)
    {
        return;
    }

    std::vector<uint8_t> grayscale_image;
    grayscale_image.reserve(width*height);

    for(unsigned y = 0; y<height; ++y)
    {
        for(unsigned x = 0; x<width; ++x)
        {
            unsigned avg_pixel = 0;

            for(uint8_t bppi = 0; bppi < bpp; ++bppi)
            {
                avg_pixel += image[y*width*bpp+x*bpp+bppi];
            }
            avg_pixel /= bpp;
            grayscale_image.emplace_back(avg_pixel);
        }
    }

    bpp = 1;
    image = std::move(grayscale_image);
}

void yandere_image::bpp_resize(uint8_t set_bpp, uint8_t extra_channel)
{
    if(bpp==set_bpp)
    {
        return;
    }

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
                    recolored_image.emplace_back(image[y*width*bpp+x*bpp+bppi]);
                } else
                {
                    recolored_image.emplace_back(extra_channel);
                }
            }
        }
    }

    bpp = set_bpp;
    image = std::move(recolored_image);
}

void yandere_image::resize(unsigned set_width, unsigned set_height, resize_type type)
{
    if(set_width==width&&set_height==height)
    {
        return;
    }

    std::vector<uint8_t> resized_image;
    resized_image.reserve(set_width*set_height*bpp);

    float resize_ratio_width = static_cast<float>(set_width)/width;
    float resize_ratio_height = static_cast<float>(set_height)/height;

    float current_pixel_width = 1/resize_ratio_width;
    float current_pixel_height = 1/resize_ratio_height;

    float current_pixel_widthHalf = current_pixel_width/2.0f;
    float current_pixel_heightHalf = current_pixel_height/2.0f;

    float pixel_area = resize_ratio_width*resize_ratio_height;

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
                    float ratio_x = x/static_cast<float>(set_width);
                    float ratio_y = y/static_cast<float>(set_height);

                    float calc_x = std::round(width*ratio_x);
                    float calc_y = std::round(height*ratio_y);

                    unsigned pixel_start_index = calc_y*width*bpp+calc_x*bpp;
                    for(uint8_t bppi = 0; bppi < bpp; ++bppi)
                    {
                        res_pixel[bppi] = image[pixel_start_index+bppi];
                    }

                    break;
                }

                case resize_type::area_sample:
                {
                    float left_border = (x/resize_ratio_width-current_pixel_widthHalf);
                    left_border = left_border < 0 ? 0 : left_border;

                    float top_border = (y/resize_ratio_height-current_pixel_heightHalf);
                    top_border = top_border < 0 ? 0 : top_border;

                    float right_border = (x/resize_ratio_width+current_pixel_widthHalf);
                    right_border = right_border > width ? width : right_border;

                    float bottom_border = (y/resize_ratio_height+current_pixel_heightHalf);
                    bottom_border = bottom_border > height ? height : bottom_border;

                    unsigned lowest_x = static_cast<unsigned>(left_border);
                    unsigned lowest_y = static_cast<unsigned>(top_border);
                    unsigned highest_x = std::ceil(right_border);
                    unsigned highest_y = std::ceil(bottom_border);

                    float pixels_avgd = 0;

                    for(unsigned oy = lowest_y; oy < highest_y; ++oy)
                    {
                        for(unsigned ox = lowest_x; ox < highest_x; ++ox)
                        {
                            float pixelInsideAreaRatio;

                            if(oy!=lowest_y&&ox!=lowest_x&&oy!=(highest_y-1)&&ox!=(highest_x-1))
                            {
                                //pixel fully inside the area
                                pixelInsideAreaRatio = 1;
                            } else
                            {
                                //pixel partially outside the area

                                float scaled_x = x/resize_ratio_width;
                                float scaled_y = y/resize_ratio_height;

                                float overlap_width;
                                float overlap_height;

                                if(ox!=lowest_x&&ox!=(highest_x-1))
                                {
                                    if(scaled_x<=ox)
                                    {
                                        overlap_width = scaled_x+current_pixel_widthHalf-ox;
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
                                        overlap_height = scaled_y+current_pixel_heightHalf-oy;
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

                                pixelInsideAreaRatio = overlap_width*overlap_height;
                            }

                            pixels_avgd += pixelInsideAreaRatio;

                            for(uint8_t bppi = 0; bppi < bpp; ++bppi)
                            {
                                avg_pixel[bppi] += image[oy*width*bpp+ox*bpp+bppi]*pixelInsideAreaRatio;
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

    image = std::move(resized_image);
}

void yandere_image::flip()
{
    std::vector<uint8_t> flipped_data;
    flipped_data.reserve(image.size());

    for(unsigned y = height; y>0; --y)
    {
        for(unsigned x = 0; x<width; ++x)
        {
            for(uint8_t bppi = 0; bppi<bpp; ++bppi)
            {
                flipped_data.emplace_back(image[(y-1)*width*bpp+x*bpp+bppi]);
            }
        }
    }

    image = std::move(flipped_data);
}

bool yandere_image::save_to_file(std::string save_path)
{
    std::filesystem::path save_filepath(save_path);

    if(save_filepath.extension()==".pgm")
    {
        pgm_save(save_path);

        return true;
    } else if(save_filepath.extension()==".ppm")
    {
    	ppm_save(save_path);
    	
    	return true;
    } else if(save_filepath.extension()==".png")
    {
    	png_save(save_path);
    	
    	return true;
    }

    return false;
}

bool yandere_image::read(std::string load_path)
{
	std::filesystem::path iPath(load_path);
	_image_path = load_path;

    if(iPath.extension()==".png")
    {
        png_read();
        return true;
    } else if(iPath.extension()==".pgm")
    {
        pgm_read();
        return true;
    }
    
    return false;
}

unsigned yandere_image::pixel_color_pos(unsigned x, unsigned y, uint8_t color)
{
	return y*width*bpp+x*bpp+color;
}

uint8_t yandere_image::pixel_color(unsigned x, unsigned y, uint8_t color)
{
	return image[y*width*bpp+x*bpp+color];
}

bool yandere_image::can_parse(std::string extension)
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
