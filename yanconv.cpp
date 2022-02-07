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

//#define YANTIMINGS

std::vector<std::string> stringSplit(std::string splitStr, std::string delim)
{
    std::vector<std::string> retVec;

    int currDelimPos = splitStr.find(delim);
    while(currDelimPos!=-1)
    {
        retVec.push_back(splitStr.substr(0, currDelimPos));

        splitStr = splitStr.substr(currDelimPos+1);
        currDelimPos = splitStr.find(delim);
    }

    retVec.push_back(splitStr.substr(0));

    return retVec;
}

template <typename T>
inline void cAToNumber(char* val, T* returnVal)
{
    T calcVal;

    *returnVal = 0;
    memcpy(&calcVal, val, sizeof(T));

    uint8_t orderByte = 1;
    T byteMask = 0xff;
    bool rightMove = true;

    for(int i = 0; i < sizeof(T); ++i)
    {
        if(rightMove)
        {
            *returnVal |= ((calcVal >> 8*(sizeof(T)-orderByte)) & byteMask);
        } else
        {
            *returnVal |= ((calcVal << 8*(sizeof(T)-orderByte)) & byteMask);
        }
        orderByte += rightMove ? 2 : -2;
        rightMove = rightMove && (orderByte < sizeof(T));
        orderByte = orderByte > sizeof(T) ? sizeof(T)-1 : orderByte;
        byteMask = byteMask << 8;
    }
}

template <typename T>
inline std::vector<T> lengthsToPrefix(std::vector<T> lengthsArr, T maxVal, uint8_t* highestLength)
{
    //is this fast or optimized? i dont know
    //am i done with trying? yes
    size_t lArrSize = lengthsArr.size(); //this could be not big enough if the max length in the vector is bigger than its size
    std::vector<uint8_t> countsArr(lArrSize, 0);

    uint8_t maxBitLength = 0;
    for(const auto& currLength : lengthsArr)
    {
        countsArr[currLength]++;
        if(maxBitLength<currLength) maxBitLength = currLength;
    }

    if(highestLength!=NULL) *highestLength = maxBitLength;

    int code = 0;
    countsArr[0] = 0;
    std::vector<int> codeLengths(maxBitLength+1, 0);
    for(int i = 1; i <= maxBitLength; ++i)
    {
        code = (code+countsArr[i-1])<<1;
        codeLengths[i] = code;
    }

    std::vector<T> finalArr(lArrSize, maxVal);
    for(int i = 0; i < lArrSize; ++i)
    {
        int currLength = lengthsArr[i];
        if(currLength!=0)
        {
            finalArr[i] = codeLengths[currLength];
            codeLengths[currLength]++;
        }
    }

    return finalArr;
}

YandereImage::YandereImage() : image({})
{
}

YandereImage::YandereImage(std::string imagePath) : _imagePath(imagePath)
{
    if(!read(imagePath))
    {
    	throw std::runtime_error("cant read " + imagePath);
    }
}

std::vector<uint8_t> YandereImage::yan_deflate(std::vector<uint8_t>& inputData)
{
    std::vector<uint8_t> deflatedData;

    uint8_t compressionInfo = (inputData[0]>>4);
    uint8_t compressionMethod = (inputData[0]&0x0f);

    if(compressionMethod!=8 | compressionInfo>7)
    {
        //all deflate streams have 8 compression method (at the moment)
        return std::move(deflatedData);
    }

    uint8_t compressionLevel = (inputData[1]>>6);
    bool presetDict = ((inputData[1]>>5)&0x1);

    char checksumStr[4] = {};
    checksumStr[0] = static_cast<char>(inputData[0]);
    checksumStr[1] = static_cast<char>(inputData[1]);
    checksumStr[2] = 0;
    checksumStr[3] = 0;

    unsigned checksumNum;
    cAToNumber<unsigned>(checksumStr, &checksumNum);

    if(checksumNum%31!=0)
    {
        //the checksum is just the 2 bytes being divisible by 31
        return std::move(deflatedData);
    }

    bool lastBlock = false;
    uint8_t compressionType;
    
    uint_fast8_t bitOffset = 0;
	int i = 2;
	
	auto shiftOffsets = [&bitOffset, &i](uint_fast8_t num) mutable { bitOffset+=num; if(bitOffset>7) { ++i; bitOffset &= 0x7;} };
	auto shiftOffset = [&bitOffset, &i]() mutable {++bitOffset; i += ((bitOffset)&0x8)>>3; bitOffset &= 0x7;};
	auto readForward = [&bitOffset, &i, &inputData, &shiftOffsets](uint_fast8_t num) mutable -> unsigned {auto retVal = ((((inputData[i]>>bitOffset)&((2<<(num-1))-1)) | ((inputData[i+1]<<(8-bitOffset))&((2<<(num-1))-1)))); shiftOffsets(num); return retVal;};
    //2 first bytes are zlib stuff so start reading DEFLATE blocks at 2 offset
    //int might be too small for large data (gigabytes range)
    //but my implementation isnt even fast enough to read that much in a realistic amount of time
    while(!lastBlock)
    {
        lastBlock = ((inputData[i]>>bitOffset)&0x1);
        shiftOffset();

        compressionType = ((inputData[i]>>bitOffset)&0x1);
        shiftOffset();
        compressionType |= ((inputData[i]>>bitOffset)&0x1)<<1;
        shiftOffset();

        //00 - no compression
        //01 - compressed with fixed huffman codes
        //10 - compressed with dynamic huffman codes
        //11 - error
        if(compressionType==0)
        {
            if(bitOffset>0) ++i;
            bitOffset = 0; //skip the partial bytes
            char dataLengthStr[4] = {};
            dataLengthStr[0] = static_cast<char>(inputData[i]);
            dataLengthStr[1] = static_cast<char>(inputData[i+1]);
            dataLengthStr[2] = 0;
            dataLengthStr[3] = 0;
            i+=4; //skip the one's complement of length
            unsigned dataLength;
            memcpy(&dataLength, dataLengthStr, 4);

            deflatedData.reserve(dataLength);
            for(int d = 0; d < dataLength; ++d)
            {
                deflatedData.push_back(inputData[i++]);
            }
        } else
        {
            int readLength = 0;
            int readDistance = 0;

            if(compressionType==3)
            {
                return std::move(deflatedData);
            } else if(compressionType==2)
            {
                unsigned codesLengthTotal = static_cast<unsigned>(readForward(5));
                codesLengthTotal += 257;

                unsigned codesDistanceTotal = static_cast<unsigned>(readForward(5));
                ++codesDistanceTotal;

                unsigned codeLength = static_cast<unsigned>(readForward(4));
                codeLength += 4;

                std::vector<unsigned> lengthCodesBin;
                lengthCodesBin.reserve(19);
                for(int lc = 0; lc < 19; ++lc)
                {
                    uint8_t lengthCode;
                    if(lc<codeLength)
                    {
                        lengthCode = readForward(3);
                    } else
                    {
                        lengthCode = 0;
                    }

                    lengthCodesBin.push_back(lengthCode);
                }

                //reorder the codes
                lengthCodesBin = {lengthCodesBin[3], lengthCodesBin[17], lengthCodesBin[15], lengthCodesBin[13], lengthCodesBin[11], lengthCodesBin[9], lengthCodesBin[7], lengthCodesBin[5], lengthCodesBin[4], lengthCodesBin[6], lengthCodesBin[8], lengthCodesBin[10], lengthCodesBin[12], lengthCodesBin[14], lengthCodesBin[16], lengthCodesBin[18], lengthCodesBin[0], lengthCodesBin[1], lengthCodesBin[2]};

                std::vector<unsigned> lengthCodesVec = lengthCodesBin;
                lengthCodesBin = lengthsToPrefix<unsigned>(lengthCodesBin, UINT_MAX, NULL);

                size_t lciSize = lengthCodesBin.size();

                std::vector<unsigned> codewordsVec;
                uint_fast16_t readingBits = 0;
                uint_fast8_t currCodeLength = 0;
                for(;codesLengthTotal!=0; readingBits <<= 1)
                {
                    readingBits |= (inputData[i]>>bitOffset)&0x1;
                    shiftOffset();
                    currCodeLength++;

                    int codeIndex = lciSize;
                    for(int lci = 0; lci < lciSize; ++lci)
                    {
                        if(lengthCodesBin[lci]==readingBits)
                        {
                            codeIndex = lci;
                            break;
                        }
                    }

                    if(codeIndex<19 && lengthCodesVec[codeIndex]==currCodeLength)
                    {
                        readLength = 0;
                        if(codeIndex<16)
                        {
                            //code length 0-15
                            codewordsVec.push_back(codeIndex);
                            --codesLengthTotal;
                        } else if(codeIndex==16)
                        {
                            //this code reads previous code length 3-6 times (2 extra bits to read)
                            readLength = readForward(2);
                            readLength += 0x3;
                            uint8_t prevCodeLength = codewordsVec[codewordsVec.size()-1];
                            codewordsVec.insert(codewordsVec.end(), readLength, prevCodeLength);
                            codesLengthTotal -= readLength;
                        } else if(codeIndex==17)
                        {
                            //this code copies 0 code 3-10 times (3 extra bits to read)
                            readLength = readForward(3);
                            readLength += 0x3;
                            codewordsVec.insert(codewordsVec.end(), readLength, 0);
                            codesLengthTotal -= readLength;
                        } else
                        {
                            //this code copies 0 code 11-138 times (7 extra bits to read)
                            readLength = readForward(7);
                            readLength += 0xb;
                            codewordsVec.insert(codewordsVec.end(), readLength, 0);
                            codesLengthTotal -= readLength;
                        }

                        currCodeLength = 0;
                        readingBits = 0;
                    }
                }

                std::vector<unsigned> codewordsCodeVec = codewordsVec;
                uint8_t highestCodewordsCodeLength;
                codewordsCodeVec = lengthsToPrefix<unsigned>(codewordsCodeVec, UINT_MAX, &highestCodewordsCodeLength);

                //copy pasting :(
                std::vector<unsigned> distanceCodeVec;
                readingBits = 0;
                currCodeLength = 0;
                for(;codesDistanceTotal!=0; readingBits <<= 1)
                {
                    readingBits |= (inputData[i]>>bitOffset)&0x1;
                    shiftOffset();
                    ++currCodeLength;

                    int codeIndex = lciSize;
                    for(int lci = 0; lci < lciSize; ++lci)
                    {
                        if(lengthCodesBin[lci]==readingBits)
                        {
                            codeIndex = lci;
                            break;
                        }
                    }

                    if(codeIndex<19 && lengthCodesVec[codeIndex]==currCodeLength)
                    {
                        readLength = 0;
                        if(codeIndex<16)
                        {
                            //code length 0-15
                            distanceCodeVec.push_back(codeIndex);
                            --codesDistanceTotal;
                        } else if(codeIndex==16)
                        {
                            //this code reads previous code length 3-6 times (2 extra bits to read)
                            readLength = readForward(2);
                            readLength += 0x3;
                            uint8_t prevCodeLength = distanceCodeVec[distanceCodeVec.size()-1];
                            distanceCodeVec.insert(distanceCodeVec.end(), readLength, prevCodeLength);
                            codesDistanceTotal -= readLength;
                        } else if(codeIndex==17)
                        {
                            //this code copies 0 code 3-10 times (3 extra bits to read)
                            readLength = readForward(3);
                            readLength += 0x3;
                            distanceCodeVec.insert(distanceCodeVec.end(), readLength, 0);
                            codesDistanceTotal -= readLength;
                        } else
                        {
                            //this code copies 0 code 11-138 times (7 extra bits to read)
                            readLength = readForward(7);
                            readLength += 0xb;
                            distanceCodeVec.insert(distanceCodeVec.end(), readLength, 0);
                            codesDistanceTotal -= readLength;
                        }

                        currCodeLength = 0;
                        readingBits = 0;
                    }
                }

                std::vector<unsigned> distanceCodeLengthVec = distanceCodeVec;

                uint8_t highestDistanceCodeLength;
                distanceCodeVec = lengthsToPrefix<unsigned>(distanceCodeVec, UINT_MAX, &highestDistanceCodeLength);

                size_t cVSize = codewordsCodeVec.size();

                std::vector<uint_fast8_t> codewordsLengthCodeSize(highestCodewordsCodeLength+1);
                std::vector<std::vector<uint_fast16_t>> codewordsLengthCodeLookup(highestCodewordsCodeLength+1);

                codewordsLengthCodeSize[0] = 0;
                codewordsLengthCodeLookup[0] = std::vector<uint_fast16_t>();
                for(int clci = 1; clci <= highestCodewordsCodeLength; ++clci)
                {
                    std::vector<uint_fast16_t> innerVec;
                    for(int clcii = 0; clcii < cVSize; ++clcii)
                    {
                        if(codewordsVec[clcii]==clci)
                        {
                            innerVec.push_back(clcii);
                        }
                    }

                    codewordsLengthCodeSize[clci] = static_cast<uint_fast8_t>(innerVec.size());
                    codewordsLengthCodeLookup[clci] = innerVec;
                }

                size_t dcVSize = distanceCodeLengthVec.size();

                std::vector<std::vector<uint_fast16_t>> distanceLengthCodeLookup(highestDistanceCodeLength+1);

                distanceLengthCodeLookup[0] = std::vector<uint_fast16_t>();
                for(int dlci = 1; dlci <= highestDistanceCodeLength; ++dlci)
                {
                    std::vector<uint_fast16_t> innerVec;
                    for(int dlcii = 0; dlcii < dcVSize; ++dlcii)
                    {
                        if(distanceCodeLengthVec[dlcii]==dlci)
                        {
                            innerVec.push_back(dlcii);
                        }
                    }

                    distanceLengthCodeLookup[dlci] = innerVec;
                }

                uint_fast8_t currCodeLengthL = 0;
                uint_fast16_t readingBitsL = 0;

                #ifdef YANTIMINGS
                auto startTime = std::chrono::high_resolution_clock::now();
                long long unsigned codeDurations = 0;
                long long unsigned codeDCounter = 0;
                long long unsigned opDurations = 0;
                long long unsigned opDCounter = 0;
                #endif

                for(;; readingBitsL <<= 1)
                {
                    readingBitsL |= (inputData[i]>>bitOffset)&0x1;

                    ++bitOffset;
                    if(((bitOffset)&0x8)>>3)
                    {
                        ++i;
                        bitOffset &= 0x7;
                    }
                    ++currCodeLengthL;

                    int_fast16_t codeIndex = cVSize;

                    uint_fast8_t currLenLookupLength = codewordsLengthCodeSize[currCodeLengthL];
                    for(uint_fast8_t cci = 0; cci < currLenLookupLength; ++cci)
                    {
                        if(codewordsCodeVec[codewordsLengthCodeLookup[currCodeLengthL][cci]]==readingBitsL)
                        {
                            codeIndex = codewordsLengthCodeLookup[currCodeLengthL][cci];
                            break;
                        }
                    }

                    if(codeIndex!=cVSize)
                    {
                        #ifdef YANTIMINGS
                        auto timeTaken = std::chrono::high_resolution_clock::now() - startTime;
                        codeDurations += (std::chrono::duration_cast<std::chrono::nanoseconds>(timeTaken).count());
                        ++codeDCounter;
                        #endif

                        if(codeIndex<256)
                        {
                            deflatedData.push_back(static_cast<uint8_t>(codeIndex));
                        } else
                        {
                            if(codeIndex==256)
                            {
                                #ifdef YANTIMINGS
                                printf("(avg) one code: %u ns, (avg) one opcalc: %u ns\n", codeDurations/codeDCounter, opDurations/opDCounter);
                                #endif
                                break;
                            }

                            #ifdef YANTIMINGS
                            auto startTime2 = std::chrono::high_resolution_clock::now();
                            #endif
                            //i am addicted to copy pasting
                            readLength = codeIndex-0xfe;
                            int extraLengthBits = ((codeIndex-0x105)/4);

                            if(extraLengthBits!=0)
                            {
                                if(extraLengthBits==6)
                                {
                                    extraLengthBits = 0;
                                    readLength = 258;
                                } else
                                {
                                    readLength = codeIndex-0xfe;
                                    readLength = readLength<0 ? 0 : readLength;

                                    int moverVal = codeIndex-0x10a;

                                    int loopRange = moverVal/4+1;
                                    for(int mli = 0; mli < loopRange; ++mli)
                                    {
                                        readLength += (moverVal+1-(4*mli))*(1<<mli);
                                    }
                                }
                            }

                            for(int ext = 0; ext < extraLengthBits; ++ext)
                            {
                                readLength += ((inputData[i]>>bitOffset)&0x1)<<ext;
                                shiftOffset();
                            }

                            uint8_t currInnerBitsLength = 0;
                            readDistance = 0;
                            for(;; readDistance <<= 1)
                            {
                                readDistance |= (inputData[i]>>bitOffset)&0x1;
                                shiftOffset();
                                ++currInnerBitsLength;

                                int distanceIndex = dcVSize;

                                size_t currLenLookupLength = distanceLengthCodeLookup[currInnerBitsLength].size();
                                for(int dci = 0; dci < currLenLookupLength; ++dci)
                                {
                                    if(distanceCodeVec[distanceLengthCodeLookup[currInnerBitsLength][dci]]==readDistance)
                                    {
                                        distanceIndex = distanceLengthCodeLookup[currInnerBitsLength][dci];
                                        break;
                                    }
                                }

                                if(distanceIndex!=dcVSize)
                                {
                                    readDistance = distanceIndex+1;
                                    break;
                                }
                            }

                            int extraDistanceBits = ((readDistance-0x3)/2);

                            int moverVal = readDistance-0x6;

                            int loopRange = moverVal/2+1;
                            for(int mli = 0; mli < loopRange; ++mli)
                            {
                                readDistance += (moverVal+1-(2*mli))*(1<<mli);
                            }

                            for(int ext = 0; ext < extraDistanceBits; ++ext)
                            {
                                readDistance += ((inputData[i]>>bitOffset)&0x1)<<ext;
                                shiftOffset();
                            }

                            #ifdef YANTIMINGS
                            auto timeTaken2 = std::chrono::high_resolution_clock::now() - startTime2;
                            opDurations += (std::chrono::duration_cast<std::chrono::nanoseconds>(timeTaken2).count());
                            ++opDCounter;
                            #endif

                            deflatedData.reserve(readLength);
                            unsigned deflDataEnd = deflatedData.size();
                            for(int rb = 0; rb < readLength; ++rb)
                            {
                                deflatedData.emplace_back(deflatedData[deflDataEnd-readDistance+rb]);
                            }
                        }

                        currCodeLengthL = 0;
                        readingBitsL = 0;

                        #ifdef YANTIMINGS
                        startTime = std::chrono::high_resolution_clock::now();
                        #endif
                    }
                }
             } else
             {
                while(true)
                {
                    uint8_t checkBits = 0;
                    for(int d = 0; d < 7; ++d)
                    {
                        checkBits |= (inputData[i]>>bitOffset)&0x1;
                        checkBits <<= 1;
                        shiftOffset();
                    }

                    int outBits;
                    if(checkBits>0xc8)
                    {
                        //its 144-255
                        unsigned outBits;
                        checkBits |= (inputData[i]>>bitOffset)&0x1;
                        shiftOffset();

                        outBits = static_cast<unsigned>(checkBits);

                        outBits <<= 1;
                        outBits |= (inputData[i]>>bitOffset)&0x1;
                        shiftOffset();

                        outBits -= 0x100;

                        deflatedData.push_back(outBits);
                        continue;
                    } else if(checkBits>0xbf)
                    {
                        //its 280-287
                        checkBits |= (inputData[i]>>bitOffset)&0x1;
                        outBits = checkBits + 0x58;

                        shiftOffset();
                    } else if(checkBits>0x2e)
                    {
                        //its 0-143
                        checkBits |= (inputData[i]>>bitOffset)&0x1;
                        checkBits -= 0x30;

                        shiftOffset();

                        deflatedData.push_back(checkBits);
                        continue;
                    } else
                    {
                        //its 256-279
                        checkBits >>= 1;
                        if(checkBits==0)
                        {
                            break;
                        } else
                        {
                            outBits = checkBits + 0x100;
                        }
                    }

                    readLength = outBits-0xfe;
                    int clampedBits = (outBits-0x105);
                    clampedBits = clampedBits<0 ? 0 : clampedBits;
                    uint8_t extraLengthBits = static_cast<uint8_t>(clampedBits/4);

                    if(extraLengthBits==6)
                    {
                        extraLengthBits = 0;
                        readLength = 258;
                    } else if(extraLengthBits!=0)
                    {
                        readLength = outBits-0xfe;
                        readLength = readLength<0 ? 0 : readLength;

                        int moverVal = outBits-0x10a;

                        int loopRange = moverVal/4+1;
                        for(int mli = 0; mli < loopRange; ++mli)
                        {
                            readLength += (moverVal+1-(4*mli))*(1<<mli);
                        }
                    }

                    for(int ext = 0; ext < extraLengthBits; ++ext)
                    {
                        readLength += ((inputData[i]>>bitOffset)&0x1)<<ext;
                        shiftOffset();
                    }

                    readDistance = 0;
                    for(int d = 0; d < 5; ++d)
                    {
                        readDistance <<= 1;
                        readDistance |= (inputData[i]>>bitOffset)&0x1;
                        shiftOffset();
                    }

                    clampedBits = (readDistance-0x2);
                    clampedBits = clampedBits<0 ? 0 : clampedBits;
                    uint8_t extraDistanceBits = static_cast<uint8_t>(clampedBits/2);

                    int moverVal = readDistance-0x5;

                    int loopRange = moverVal/2+1;
                    for(int mli = 0; mli < loopRange; ++mli)
                    {
                        readDistance += (moverVal+1-(2*mli))*(1<<mli);
                    }

                    readDistance++;

                    for(int ext = 0; ext < extraDistanceBits; ++ext)
                    {
                        readDistance += ((inputData[i]>>bitOffset)&0x1)<<ext;
                        shiftOffset();
                    }

                    deflatedData.reserve(readLength);
                    unsigned deflDataEnd = deflatedData.size();
                    for(int rb = 0; rb < readLength; ++rb)
                    {
                        deflatedData.emplace_back(deflatedData[deflDataEnd-readDistance+rb]);
                    }
                }
            }
        }
    }

    return std::move(deflatedData);
}

uint8_t YandereImage::sub_positive(int lVar, int rVar)
{
    return lVar + rVar - 256;
}

bool YandereImage::png_read(bool checkChecksums)
{
    std::ifstream inputStream(_imagePath, std::ios::binary);

    //PNG files begin with 89 50 4e 47 0d 0a 1a 0a
    inputStream.seekg(8, std::ios_base::beg);

    uint8_t bitDepth;
    uint8_t valuesPerPixel;

    bool palleteUsed;
    bool rgbUsed;
    bool alphaChannel;

    uint8_t valsPerByte;

    bool interlacing;

    std::vector<uint8_t> deflateStream;

    std::vector<uint8_t> palleteVector;

    std::vector<uint8_t> tempData;
    unsigned tempWidth = 0;
    unsigned tempHeight = 0;

    while(inputStream.good())
    {
        //this is definitely not proper c++
        char chunkLengthStr[4] = {};
        inputStream.read(chunkLengthStr, 4);
        unsigned chunkLength;
        cAToNumber<unsigned>(chunkLengthStr, &chunkLength);

        char chunkType[4] = {};
        inputStream.read(chunkType, 4);

        char* chunkData = (char*)malloc(chunkLength); //char is always 1 byte long
        inputStream.read(chunkData, chunkLength);

        char chunkChecksum[4] = {};
        inputStream.read(chunkChecksum, 4);

        if(checkChecksums)
        {
            //too lazy
        }

        if(strncmp(chunkType, "IHDR", 4)==0)
        {
            char imageWidthStr[4] = {};
            char imageHeightStr[4] = {};
            for(int i = 0; i < 4; i++)
            {
                imageWidthStr[i] = chunkData[i];
                imageHeightStr[i] = chunkData[i+4];
            }
            cAToNumber<unsigned>(imageWidthStr, &tempWidth);
            cAToNumber<unsigned>(imageHeightStr, &tempHeight);

            bitDepth = (uint8_t)chunkData[8];
            uint8_t colorType = (uint8_t)chunkData[9];

            palleteUsed = colorType&0x1;
            rgbUsed = (colorType>>1)&0x1;
            alphaChannel = (colorType>>2)&0x1;

            valuesPerPixel = (2*rgbUsed)+alphaChannel+1;

            bpp = std::ceil((valuesPerPixel*bitDepth)/8.0f);
            valsPerByte = 8/bitDepth;

            interlacing = (bool)chunkData[12];
        } else if(strncmp(chunkType, "IDAT", 4)==0)
        {
            deflateStream.reserve(chunkLength);
            for(int i = 0; i < chunkLength; i++)
            {
                deflateStream.push_back(chunkData[i]);
            }
        } else if(strncmp(chunkType, "PLTE", 4)==0)
        {
            if(chunkLength%3!=0)
            {
                free(chunkData);
                inputStream.close();
                return false;
            }
            palleteVector.reserve(chunkLength);
            for(int i = 0; i < chunkLength; i++)
            {
                palleteVector.emplace_back(chunkData[i]);
            }
        } else if(strncmp(chunkType, "IEND", 4)==0)
        {
            #ifdef YANTIMINGS
            printf("deflate started\n");
            auto startTime = std::chrono::high_resolution_clock::now();
            #endif

            std::vector<uint8_t> imageData = yan_deflate(deflateStream);

            #ifdef YANTIMINGS
            auto timeTaken = std::chrono::high_resolution_clock::now() - startTime;
            printf("deflate took: %i ms\n", (std::chrono::duration_cast<std::chrono::milliseconds>(timeTaken).count()));
            #endif
            
            if(imageData.size()!=0)
            {
                tempData.reserve(std::ceil(tempWidth*tempHeight*valuesPerPixel/(float)valsPerByte));

                unsigned streamPos = 0;
                uint8_t readBits = 0;

                for(int y = 0; y < tempHeight; ++y)
                {
                    uint8_t filterType = imageData[streamPos++];

                    if(palleteUsed)
                    {
                        for(int x = 0; x < tempWidth; ++x, ++streamPos)
                        {
                            unsigned palletePixel = imageData[streamPos]*3;
                            tempData.emplace_back(palleteVector[palletePixel]);
                            tempData.emplace_back(palleteVector[palletePixel+1]);
                            tempData.emplace_back(palleteVector[palletePixel+2]);
                        }
                    } else
                    {
                        for(int x = 0; x < tempWidth*valuesPerPixel; ++x, ++streamPos)
                        {
                            uint8_t pixelData;

                            switch(filterType)
                            {
                                case 0:
                                    pixelData = imageData[streamPos];
                                break;

                                case 1:
                                {
                                    if(x<bpp)
                                    {
                                        pixelData = imageData[streamPos];
                                    } else
                                    {
                                        pixelData = sub_positive(imageData[streamPos], imageData[streamPos-bpp]);
                                        imageData[streamPos] = pixelData;
                                    }
                                    break;
                                }
                                case 2:
                                {
                                    if(y==0)
                                    {
                                        pixelData = imageData[streamPos];
                                    } else
                                    {
                                        unsigned aboveIndex = streamPos-(1+(tempWidth*valuesPerPixel)/valsPerByte); //the +1 is to skip the filter type for current line
                                        pixelData = sub_positive(imageData[streamPos], imageData[aboveIndex]);
                                        imageData[streamPos] = pixelData;
                                    }
                                    break;
                                }
                                case 3:
                                {
                                    unsigned aboveIndex = streamPos-(1+(tempWidth*valuesPerPixel)/valsPerByte);
                                    uint8_t upPixel = y==0 ? 0 : imageData[aboveIndex];
                                    uint8_t leftPixel = x<bpp ? 0 : imageData[streamPos-bpp];

                                    uint8_t rightSidePixel = (leftPixel+upPixel)/2;
                                    pixelData = sub_positive(imageData[streamPos], rightSidePixel);
                                    imageData[streamPos] = pixelData;
                                    break;
                                }
                                case 4:
                                {
                                    unsigned aboveIndex = streamPos-(1+(tempWidth*valuesPerPixel)/valsPerByte);
                                    uint8_t upPixel = y==0 ? 0 : imageData[aboveIndex];
                                    uint8_t leftPixel = x<bpp ? 0 : imageData[streamPos-bpp];
                                    uint8_t upLeftPixel = ((y==0) | (x<bpp)) ? 0 : imageData[aboveIndex-bpp];

                                    uint8_t rightSidePixel;

                                    int initialEst = leftPixel + upPixel - upLeftPixel;
                                    int distanceLeft = std::abs(initialEst - leftPixel);
                                    int distanceUp = std::abs(initialEst - upPixel);
                                    int distanceUpLeft = std::abs(initialEst - upLeftPixel);

                                    if((distanceLeft<=distanceUp)&&(distanceLeft<=distanceUpLeft))
                                    {
                                        rightSidePixel = leftPixel;
                                    } else if(distanceUp<=distanceUpLeft)
                                    {
                                        rightSidePixel = upPixel;
                                    } else
                                    {
                                        rightSidePixel = upLeftPixel;
                                    }

                                    pixelData = sub_positive(imageData[streamPos], rightSidePixel);
                                    imageData[streamPos] = pixelData;
                                    break;
                                }
                            }

                            tempData.emplace_back(pixelData);
                        }
                    }
                }
            }
            

            image = tempData;
            width = tempWidth;
            height = tempHeight;

            free(chunkData);
            break;
        }/* else
        {
            char newChunk[5] = {chunkType[0], chunkType[1], chunkType[2], chunkType[3], '\0'};
            printf("%s\n", newChunk);
        }*/

        free(chunkData);
    }

    inputStream.close();

    return true;
}

std::vector<uint8_t> YandereImage::yan_inflate(std::vector<uint8_t>& inputData)
{
	std::vector<uint8_t> inflatedData;
	
	uint8_t compressionMethod = 8; //the only one that exists atm
	uint8_t compressionInfo = 7; //max window size, not even gonna try calculating it properly
	
	uint8_t cmfData = (compressionInfo<<4)|(compressionMethod&0x0f);
	inflatedData.push_back(cmfData);
	
	uint8_t checkVal = 0; //has to be that (cmfData*256 + flgData) % 31 == 0
	uint8_t dictSet = 0; //i dont know how enabling it helps me in any way
	uint8_t compressionLevel = 0; //i clearly used the least compressed one there is because its made by me
	
	uint8_t flgData = ((compressionLevel&0x3)<<6)|((dictSet&0x1)<<5)|(checkVal&0x1f);
	
	uint8_t crcRemainder = (static_cast<int>(cmfData)*256+static_cast<int>(flgData))%31;
	//make it mod 31 == 0
	if(crcRemainder!=0)
	{
		flgData |= (31-crcRemainder)&0x1f;
	}
	inflatedData.push_back(flgData);
	
	int inputDataSize = inputData.size();
	bool lastBlock = false;
	
	bool lastFound = false;
	std::vector<uint8_t>::iterator lastFoundIter;
	std::vector<uint8_t> searchVec;
	
	std::vector<uint8_t>::iterator foundIter;
	
	int maxDistance = 32768;
	auto firstIter = inflatedData.end();
	
	int lastInflatedDataSize = 0;
	
	//this is really unreadable, soz
	uint8_t bitOffset = 0;
	for(int i = 0; !lastBlock;)
	{	
		//0 = no compression
		//1 = static huffman tree
		//2 = dynamic huffman tree
		uint8_t encodingMethod = 0;
		
		int blockSize = 0xff;
		switch(encodingMethod)
		{
			case 0:
			{
				blockSize = 0xff; //maximum size for uncompressed data block
			break;
			}
			case 1:
			{
				blockSize = inputDataSize-i; //just encode all of it idk
			break;
			}
		}
		
		int nextBlockEdge = i+blockSize;
		if(nextBlockEdge >= inputDataSize)
		{
			lastBlock = true;
		}
		int maxBound = std::min(nextBlockEdge, inputDataSize);
		
		uint8_t bitInfo = ((encodingMethod&0x3)<<1)|(static_cast<uint8_t>(lastBlock));
		
		switch(encodingMethod)
		{
			case 0:
			{	
				inflatedData.push_back(bitInfo);
			
				//block length
				inflatedData.push_back(blockSize&0xff);
				inflatedData.push_back((blockSize&0xff00)>>8);
				
				//ones complement of block length
				inflatedData.push_back((blockSize&0xff)^0xff);
				inflatedData.push_back(((blockSize&0xff00)>>8)^0xff);
				
				inflatedData.reserve(blockSize);
				
				for(; i < maxBound; ++i)
				{		
					inflatedData.emplace_back(inputData[i]);
				}
			break;
			}
			case 1:
			{
				uint32_t bitBuffer = 0;
			
				auto reverseBits = [](uint16_t bitsToRev, uint8_t amount)->uint16_t { uint16_t reversedBits = bitsToRev; for(uint8_t i = 0; i < (amount-1); ++i) { reversedBits <<= 1; bitsToRev >>= 1; reversedBits |= bitsToRev&0x1; }; reversedBits &= ((1<<amount)-1); return reversedBits; };
				auto checkOffsets = [&bitOffset, &bitBuffer, &inflatedData]() mutable { if(bitOffset>15) { inflatedData.push_back(bitBuffer&0xff); inflatedData.push_back((bitBuffer&0xff00)>>8); bitBuffer >>= 16; bitOffset -= 16; } };
				auto pushBits = [&bitOffset, &bitBuffer, &checkOffsets](uint16_t bitsToPush, uint8_t amount) mutable { bitBuffer |= (bitsToPush<<bitOffset); bitOffset += amount; checkOffsets(); };
				auto pushBitsReverse = [&pushBits, &reverseBits](uint16_t bitsToPush, uint8_t amount) { pushBits(reverseBits(bitsToPush, amount), amount); };
				
				pushBits(bitInfo, 3);
				
				lastInflatedDataSize = inflatedData.size();
			
				for(; i < maxBound; ++i)
				{
					uint16_t currentByte = inputData[i];
					
					if(inflatedData.size() > lastInflatedDataSize)
					{
						lastInflatedDataSize = inflatedData.size();
						
						auto lastIter = inflatedData.end();
						if(std::distance(firstIter, lastIter)>maxDistance)
						{
							firstIter = inflatedData.end() - maxDistance;
						}
					
						if(!lastFound)
						{
							foundIter = std::find(firstIter, lastIter, *(inflatedData.end()-1));
							if(foundIter != lastIter)
							{
								//found a repeating symbol, search for a sequence
								searchVec.push_back(*(inflatedData.end()-1));
								lastFound = true;
								lastFoundIter = foundIter;
							}
						} else
						{
							searchVec.push_back(*(inflatedData.end()-1));
							foundIter = std::search(lastFoundIter, lastIter, searchVec.begin(), searchVec.end());
							if(foundIter != lastIter)
							{
								//check if the repeating symbol list is greater than the max length
								if(std::distance(foundIter, lastIter)==258)
								{
									//replace the symbols with distance/length pair
									printf("yes\n");
								} else
								{
									searchVec.push_back(*(inflatedData.end()-1));
									lastFoundIter = foundIter;
								}
							} else
							{
								if(searchVec.size()>2)
								{
									//found a repeating symbol sequence bigger than 2 symbols, replace with distance/length pair
									//printf("true\n");
								}
								lastFound = false;
							}
						}
					}
					
					//insert the bits
					if(currentByte<144)
					{
						pushBitsReverse((currentByte+0x30), 8);
					} else
					{
						pushBitsReverse((currentByte+0x100), 9);
					}

				}
				
				if(bitOffset>8)
				{
					inflatedData.push_back((bitBuffer&0xff00)>>8);
				}
				inflatedData.push_back(bitBuffer&0xff);
				
				pushBits(0, 7);
				if(lastBlock)
				{
					inflatedData.shrink_to_fit();
				}
			break;
			}
		}
	}
	
	int adlerSum1 = 1;
	int adlerSum2 = 0;
			
	//how inefficient ;-;
	//dont care
	for(int s = 0; s < inputDataSize; ++s)
	{
		adlerSum1 += (inputData[s] % 65521);
		adlerSum2 += (adlerSum1 % 65521);
	}
			
	inflatedData.push_back((adlerSum2&0xff00)>>8);
	inflatedData.push_back(adlerSum2&0xff);
			
	inflatedData.push_back((adlerSum1&0xff00)>>8);
	inflatedData.push_back(adlerSum1&0xff);

	return std::move(inflatedData);
}

std::vector<uint32_t> YandereImage::crc_table_gen()
{
	std::vector<uint32_t> crcTable;
	crcTable.reserve(256);

	for(int i = 0; i < 256; ++i)
	{
		uint32_t tableNum = static_cast<uint32_t>(i);
		
		for(int l = 0; l < 8; ++l)
		{
			if(tableNum&0x1)
			{
				tableNum = 0xedb88320^(tableNum>>1);
			} else
			{
				tableNum >>= 1;
			}
		}
		
		crcTable.emplace_back(tableNum);
	}
	
	return crcTable;
}

void YandereImage::png_save(std::string savePath)
{
    std::ofstream outStream(savePath, std::ios::binary);

    std::vector<char> imageHeader;
    //magic numbers for png
    imageHeader.push_back(0x89);
    
    //PNG in ascii
    imageHeader.push_back('P');
    imageHeader.push_back('N');
    imageHeader.push_back('G');

    //line ending
    imageHeader.push_back(0x0d);
    imageHeader.push_back(0x0a);
    
    imageHeader.push_back(0x1a);
    imageHeader.push_back(0x0a);
    

    outStream.write(imageHeader.data(), imageHeader.size());

    std::string widthString = std::to_string(width);
    imageHeader.insert(imageHeader.end(), widthString.begin(), widthString.end());

    std::string heightString = std::to_string(height);
    imageHeader.insert(imageHeader.end(), heightString.begin(), heightString.end());
    int imageSize = image.size();

	std::vector<std::array<char,4>> encodeChunks;
	encodeChunks.push_back({'I', 'H', 'D', 'R'});
	encodeChunks.push_back({'I', 'D', 'A', 'T'});
	encodeChunks.push_back({'I', 'E', 'N', 'D'});
	
	int chunksAmount = encodeChunks.size();
	
	for(int i = 0; i < chunksAmount; ++i)
	{
		std::vector<char> dataChunk;
		
		std::string chunkName = "";
		uint32_t chunkLength = 0;
		
		dataChunk.reserve(4);
		for(int eN = 0; eN < 4; ++eN)
		{
			dataChunk.emplace_back(encodeChunks[i][eN]);
			chunkName += encodeChunks[i][eN];
		}
		
		if(chunkName=="IDAT")
		{
			std::vector<uint8_t> imageBytes;
			imageBytes.reserve(height*width*bpp+height);
			
			for(int y = 0; y < height; ++y)
			{
				uint8_t lineFilter = 0;
				imageBytes.emplace_back(lineFilter);
			
				for(int x = 0; x < width; ++x)
				{
					for(int bppi = 0; bppi < bpp; ++bppi)
					{
						imageBytes.emplace_back(image[y*width*bpp+x*bpp+bppi]);
					}
				}
			}
			
			std::vector<uint8_t> inflatedBytes = yan_inflate(imageBytes);
			
			dataChunk.insert(dataChunk.end(), inflatedBytes.begin(), inflatedBytes.end()); //no point trying to do this efficiently, have to convert from uint8_t to char
			
			chunkLength = inflatedBytes.size();
		} else if(chunkName=="IHDR")
		{
			//4 bytes of width
			dataChunk.push_back((width&0xff000000)>>24);
			dataChunk.push_back((width&0xff0000)>>16);
			dataChunk.push_back((width&0xff00)>>8);
			dataChunk.push_back(width&0xff);
			
			//4 bytes of height
			dataChunk.push_back((height&0xff000000)>>24);
			dataChunk.push_back((height&0xff0000)>>16);
			dataChunk.push_back((height&0xff00)>>8);
			dataChunk.push_back(height&0xff);
			
			//bits per pixel, always 8 (bla bla bla not efficient)
			dataChunk.push_back(8);
			
			//color type
			//0100 bit adds alpha
			//0010 bit adds color
			//0001 bit means color from pallete
			if(bpp==1)
			{
				//grayscale
				dataChunk.push_back(0);
			} else if(bpp==2)
			{
				//grayscale with alpha
				dataChunk.push_back(4);
			} else if(bpp==3)
			{
				//fullcolor
				dataChunk.push_back(2);
			} else if(bpp==4)
			{
				//fullcolor with alpha
				dataChunk.push_back(6);
			} else
			{
				return;
			}
			
			//stuff that doesnt change, like png using deflate algorithm for compression
			dataChunk.push_back(0);
			dataChunk.push_back(0);
			
			//interlacing off
			dataChunk.push_back(0);
			
			chunkLength = 13;
		}
		
		//crc stuff
		std::vector<uint32_t> crcTable = crc_table_gen();
		
		uint32_t chunkCRC = 0xffffffff;
		
		int crcLength = chunkLength + 4; //4 letters of the chunk name
		
		for(int b = 0; b < crcLength; ++b)
		{
			chunkCRC = (crcTable[(chunkCRC^dataChunk[b])&0xff]^(chunkCRC>>8));
		}
		
		chunkCRC ^= 0xffffffff;
		//crc stuff over
		
		dataChunk.insert(dataChunk.begin(), chunkLength&0xff);
		dataChunk.insert(dataChunk.begin(), (chunkLength&0xff00)>>8);
		dataChunk.insert(dataChunk.begin(), (chunkLength&0xff0000)>>16);
		dataChunk.insert(dataChunk.begin(), (chunkLength&0xff000000)>>24);
		
		
		dataChunk.push_back((chunkCRC&0xff000000)>>24);
		dataChunk.push_back((chunkCRC&0xff0000)>>16);
		dataChunk.push_back((chunkCRC&0xff00)>>8);
		dataChunk.push_back(chunkCRC&0xff);

		outStream.write(dataChunk.data(), dataChunk.size());
    }

    outStream.close();
}

bool YandereImage::pgm_read()
{
    //im not parsing comments because NO
    std::ifstream readStream(_imagePath, std::ios::binary);

    char dataType[2];
    readStream >> dataType;

    if(strncmp(dataType, "P2", 2)==0||strncmp(dataType, "P5", 2)==0)
    {
        std::string widthString;
        readStream >> widthString;
        width = std::stoul(widthString);

        std::string heightString;
        readStream >> heightString;
        height = std::stoul(heightString);

        int imageSize = width*height;
        image.reserve(width*height);

        std::string maxvalString;
        readStream >> maxvalString;
        float maxVal = std::stof(maxvalString);

        if(strncmp(dataType, "P2", 2)==0)
        {
            //ascii
            for(int i = 0; i < imageSize; ++i)
            {
                std::string colorString;
                readStream >> colorString;

                image.emplace_back(std::round(255*(std::stoi(colorString)/maxVal)));
            }
        } else
        {
            //binary
            char colorByte = 0;
            for(int i = 0; i < imageSize; ++i)
            {
                readStream.get(colorByte);

                image.emplace_back(std::round(255*(colorByte/maxVal)));
            }
        }
    } else
    {

        readStream.close();
        return false;
    }

    bpp = 1;

    readStream.close();
    return true;
}

void YandereImage::pgm_save(std::string savePath)
{
    if(bpp!=1)
    {
        return;
    }

    std::ofstream outStream(savePath, std::ios::binary);

    std::vector<char> imageHeader;
    //magic number for the binary grayscale format
    imageHeader.push_back('P');
    imageHeader.push_back('5');

    imageHeader.push_back('\n');

    std::string widthString = std::to_string(width);
    imageHeader.insert(imageHeader.end(), widthString.begin(), widthString.end());

    imageHeader.push_back('\n');

    std::string heightString = std::to_string(height);
    imageHeader.insert(imageHeader.end(), heightString.begin(), heightString.end());

    imageHeader.push_back('\n');

    //max value for a color which is 255
    imageHeader.push_back('2');
    imageHeader.push_back('5');
    imageHeader.push_back('5');

    imageHeader.push_back('\n');

    outStream.write(imageHeader.data(), imageHeader.size());


    int imageSize = image.size();

    std::vector<char> imageData;
    //each color value is separated by a whitespace (therefore twice the bytes)
    imageData.reserve(imageSize);

    for(int i = 0; i < imageSize; ++i)
    {
        imageData.emplace_back(static_cast<char>(image[i]));
    }

    outStream.write(imageData.data(), imageData.size());

    outStream.close();
}

void YandereImage::ppm_save(std::string savePath)
{
    if(bpp!=3)
    {
        return;
    }

    std::ofstream outStream(savePath, std::ios::binary);

    std::vector<char> imageHeader;
    //magic number for the binary rgb format
    imageHeader.push_back('P');
    imageHeader.push_back('6');

    imageHeader.push_back('\n');

    std::string widthString = std::to_string(width);
    imageHeader.insert(imageHeader.end(), widthString.begin(), widthString.end());

    imageHeader.push_back('\n');

    std::string heightString = std::to_string(height);
    imageHeader.insert(imageHeader.end(), heightString.begin(), heightString.end());

    imageHeader.push_back('\n');

    //max value for a color which is 255
    imageHeader.push_back('2');
    imageHeader.push_back('5');
    imageHeader.push_back('5');

    imageHeader.push_back('\n');

    outStream.write(imageHeader.data(), imageHeader.size());


    int imageSize = image.size();

    std::vector<char> imageData;
    //each color value is separated by a whitespace (therefore twice the bytes)
    imageData.reserve(imageSize);

	for(int y = 0; y < height; ++y)
	{
    	for(int x = 0; x < width; ++x)
    	{
    		for(uint8_t bppi = 0; bppi < bpp; ++bppi)
    		{
        		imageData.emplace_back(static_cast<char>(image[y*width*bpp+x*bpp+bppi]));
        	}
    	}
    }

    outStream.write(imageData.data(), imageData.size());

    outStream.close();
}

void YandereImage::grayscale()
{
    if(bpp==1)
    {
        return;
    }

    std::vector<uint8_t> grayscaleImage;
    grayscaleImage.reserve(width*height);

    for(unsigned y = 0; y<height; ++y)
    {
        for(unsigned x = 0; x<width; ++x)
        {
            unsigned avgPixel = 0;

            for(uint8_t bppi = 0; bppi < bpp; ++bppi)
            {
                avgPixel += image[y*width*bpp+x*bpp+bppi];
            }
            avgPixel /= bpp;
            grayscaleImage.emplace_back(avgPixel);
        }
    }

    bpp = 1;
    image = std::move(grayscaleImage);
}

void YandereImage::bpp_resize(uint8_t desiredBpp, uint8_t extraChannel)
{
    if(bpp==desiredBpp)
    {
        return;
    }

    std::vector<uint8_t> recoloredImage;
    recoloredImage.reserve(width*height*desiredBpp);

    for(unsigned y = 0; y<height; ++y)
    {
        for(unsigned x = 0; x<width; ++x)
        {
            for(uint8_t bppi = 0; bppi<desiredBpp; ++bppi)
            {
                if(bppi<bpp)
                {
                    recoloredImage.emplace_back(image[y*width*bpp+x*bpp+bppi]);
                } else
                {
                    recoloredImage.emplace_back(extraChannel);
                }
            }
        }
    }

    bpp = desiredBpp;
    image = std::move(recoloredImage);
}

void YandereImage::resize(unsigned desiredWidth, unsigned desiredHeight, ResizeType resizeType)
{
    if(desiredWidth==width&&desiredHeight==height)
    {
        return;
    }

    std::vector<uint8_t> resizedImage;
    resizedImage.reserve(desiredWidth*desiredHeight*bpp);

    float resizeRatioWidth = static_cast<float>(desiredWidth)/width;
    float resizeRatioHeight = static_cast<float>(desiredHeight)/height;

    float currentPixelWidth = 1/resizeRatioWidth;
    float currentPixelHeight = 1/resizeRatioHeight;

    float currentPixelWidthHalf = currentPixelWidth/2.0f;
    float currentPixelHeightHalf = currentPixelHeight/2.0f;

    float pixelArea = resizeRatioWidth*resizeRatioHeight;

    std::vector<uint8_t> resPixel(bpp);
    std::vector<unsigned> avgPixel(bpp);

    for(unsigned y = 0; y < desiredHeight; ++y)
    {
        for(unsigned x = 0; x < desiredWidth; ++x)
        {
            switch(resizeType)
            {
                case ResizeType::nearest_neighbor:
                {
                    float ratioX = x/static_cast<float>(desiredWidth);
                    float ratioY = y/static_cast<float>(desiredHeight);

                    float calcX = std::round(width*ratioX);
                    float calcY = std::round(height*ratioY);

                    unsigned pixelStartIndex = calcY*width*bpp+calcX*bpp;
                    for(uint8_t bppi = 0; bppi < bpp; ++bppi)
                    {
                        resPixel[bppi] = image[pixelStartIndex+bppi];
                    }

                    break;
                }

                case ResizeType::area_sample:
                {
                    float leftBorder = (x/resizeRatioWidth-currentPixelWidthHalf);
                    leftBorder = leftBorder < 0 ? 0 : leftBorder;

                    float topBorder = (y/resizeRatioHeight-currentPixelHeightHalf);
                    topBorder = topBorder < 0 ? 0 : topBorder;

                    float rightBorder = (x/resizeRatioWidth+currentPixelWidthHalf);
                    rightBorder = rightBorder > width ? width : rightBorder;

                    float bottomBorder = (y/resizeRatioHeight+currentPixelHeightHalf);
                    bottomBorder = bottomBorder > height ? height : bottomBorder;

                    unsigned lowestX = static_cast<unsigned>(leftBorder);
                    unsigned lowestY = static_cast<unsigned>(topBorder);
                    unsigned highestX = std::ceil(rightBorder);
                    unsigned highestY = std::ceil(bottomBorder);

                    float pixelsAvgd = 0;

                    for(unsigned oy = lowestY; oy < highestY; ++oy)
                    {
                        for(unsigned ox = lowestX; ox < highestX; ++ox)
                        {
                            float pixelInsideAreaRatio;

                            if(oy!=lowestY&&ox!=lowestX&&oy!=(highestY-1)&&ox!=(highestX-1))
                            {
                                //pixel fully inside the area
                                pixelInsideAreaRatio = 1;
                            } else
                            {
                                //pixel partially outside the area

                                float xScaled = x/resizeRatioWidth;
                                float yScaled = y/resizeRatioHeight;

                                float overlapWidth;
                                float overlapHeight;

                                if(ox!=lowestX&&ox!=(highestX-1))
                                {
                                    if(xScaled<=ox)
                                    {
                                        overlapWidth = xScaled+currentPixelWidthHalf-ox;
                                        overlapWidth = overlapWidth>1?1:overlapWidth;
                                    } else
                                    {
                                        overlapWidth = xScaled-(ox+1);
                                        overlapWidth = overlapWidth>1?1:overlapWidth;
                                    }
                                } else
                                {
                                    //this is just for a speedup
                                    overlapWidth = 1;
                                }

                                if(ox!=lowestX&&ox!=(highestX-1))
                                {
                                    if(yScaled<=oy)
                                    {
                                        overlapHeight = yScaled+currentPixelHeightHalf-oy;
                                        overlapHeight = overlapHeight>1?1:overlapHeight;
                                    } else
                                    {
                                        overlapHeight = yScaled-(oy+1);
                                        overlapHeight = overlapHeight>1?1:overlapHeight;
                                    }
                                } else
                                {
                                    //this is just for a speedup
                                    overlapHeight = 1;
                                }

                                pixelInsideAreaRatio = overlapWidth*overlapHeight;
                            }

                            pixelsAvgd += pixelInsideAreaRatio;

                            for(uint8_t bppi = 0; bppi < bpp; ++bppi)
                            {
                                avgPixel[bppi] += image[oy*width*bpp+ox*bpp+bppi]*pixelInsideAreaRatio;
                            }
                        }
                    }

                    for(uint8_t bppi = 0; bppi < bpp; ++bppi)
                    {
                        resPixel[bppi] = avgPixel[bppi]/pixelsAvgd;
                        avgPixel[bppi] = 0;
                    }

                    break;
                }
            }

            for(uint8_t bppi = 0; bppi < bpp; ++bppi)
            {
                resizedImage.emplace_back(resPixel[bppi]);
            }
        }
    }

    width = desiredWidth;
    height = desiredHeight;

    image = std::move(resizedImage);
}

void YandereImage::flip()
{
    std::vector<uint8_t> flippedData;
    flippedData.reserve(image.size());

    for(unsigned y = height; y>0; --y)
    {
        for(unsigned x = 0; x<width; ++x)
        {
            for(uint8_t bppi = 0; bppi<bpp; ++bppi)
            {
                flippedData.emplace_back(image[(y-1)*width*bpp+x*bpp+bppi]);
            }
        }
    }

    image = std::move(flippedData);
}

bool YandereImage::save_to_file(std::string savePath)
{
    std::filesystem::path saveFilepath(savePath);

    if(saveFilepath.extension()==".pgm")
    {
        pgm_save(savePath);

        return true;
    } else if(saveFilepath.extension()==".ppm")
    {
    	ppm_save(savePath);
    	
    	return true;
    } else if(saveFilepath.extension()==".png")
    {
    	png_save(savePath);
    	
    	return true;
    }

    return false;
}

bool YandereImage::read(std::string loadPath)
{
	std::filesystem::path iPath(loadPath);
	_imagePath = loadPath;

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

unsigned YandereImage::pixel_color_pos(unsigned x, unsigned y, uint8_t color)
{
	return y*width*bpp+x*bpp+color;
}

uint8_t YandereImage::pixel_color(unsigned x, unsigned y, uint8_t color)
{
	return image[y*width*bpp+x*bpp+color];
}

bool YandereImage::can_parse(std::string extension)
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
