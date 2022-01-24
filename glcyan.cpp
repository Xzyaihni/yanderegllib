#include "glcyan.h"

#include <iostream>
#include <fstream>
#include <cmath>
#include <filesystem>
#include <algorithm>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

namespace YandereConst
{
	const int circleLOD = 10;
};

YandereCamera::YandereCamera(YanPosition position, YanPosition lookPosition) : _projectionMatrix(glm::mat4(1.0f))
{
	_cameraPosition = glm::vec3(position.x, position.y, position.z);
	_cameraDirection = glm::normalize(glm::vec3(lookPosition.x, lookPosition.y, lookPosition.z)-_cameraPosition);

	calculate_view();
}

void YandereCamera::create_projection(float fov, float aspectRatio, std::array<float, 2> planes)
{
	_projectionMatrixExists = true;

	_projectionMatrix = glm::perspective(glm::radians(fov), aspectRatio, planes[0], planes[1]);
	clipClose = planes[0];
	clipFar = planes[1];
	
	calculate_planes();
}

void YandereCamera::create_projection(std::array<float, 4> orthoBox, std::array<float, 2> planes)
{
	_projectionMatrixExists = true;

	_projectionMatrix = glm::ortho(orthoBox[0], orthoBox[1], orthoBox[2], orthoBox[3], planes[0], planes[1]);
	clipClose = planes[0];
	clipFar = planes[1];
	
	calculate_planes();
}

void YandereCamera::set_position(YanPosition position)
{
	_cameraPosition = glm::vec3(position.x, position.y, position.z);

	calculate_view();
}

void YandereCamera::set_position_x(float x)
{
	_cameraPosition.x = x;

	calculate_view();
}

void YandereCamera::set_position_y(float y)
{
	_cameraPosition.y = y;

	calculate_view();
}

void YandereCamera::set_position_z(float z)
{
	_cameraPosition.z = z;

	calculate_view();
}

void YandereCamera::set_rotation(float yaw, float pitch)
{
	_cameraDirection.x = std::cos(yaw) * std::cos(pitch);
	_cameraDirection.y = std::sin(pitch);
	_cameraDirection.z = std::sin(yaw) * std::cos(pitch);

	calculate_view();
}

void YandereCamera::set_direction(std::array<float, 3> direction)
{
	_cameraDirection = glm::vec3(direction[0], direction[1], direction[2]);

	calculate_view();
}

void YandereCamera::look_at(std::array<float, 3> lookPosition)
{
	_cameraDirection = glm::normalize(glm::vec3(lookPosition[0], lookPosition[1], lookPosition[2])-_cameraPosition);

	calculate_view();
}

void YandereCamera::translate_position(YanPosition transPos)
{
	_cameraPosition += transPos.x*_cameraRight;
	_cameraPosition += transPos.y*_cameraUp;
	_cameraPosition += transPos.z*_cameraForward;

	calculate_view();
}

YanPosition YandereCamera::position()
{
	return {_cameraPosition.x, _cameraPosition.y, _cameraPosition.z};
}

int YandereCamera::x_point_side(YanPosition point)
{
	if(_planesArr[Side::left].a*point.x+_planesArr[Side::left].b*point.y+_planesArr[Side::left].c*point.z+_planesArr[Side::left].d>0
	&& _planesArr[Side::right].a*point.x+_planesArr[Side::right].b*point.y+_planesArr[Side::right].c*point.z+_planesArr[Side::right].d>0)
	{
		return 0;
	} else
	{
		return _planesArr[Side::left].a*point.x+_planesArr[Side::left].b*point.y+_planesArr[Side::left].c*point.z+_planesArr[Side::left].d>0 ? 1 : -1;
	}
}

int YandereCamera::y_point_side(YanPosition point)
{
	if(_planesArr[Side::down].a*point.x+_planesArr[Side::down].b*point.y+_planesArr[Side::down].c*point.z+_planesArr[Side::down].d>0
	&& _planesArr[Side::up].a*point.x+_planesArr[Side::up].b*point.y+_planesArr[Side::up].c*point.z+_planesArr[Side::up].d>0)
	{
		return 0;
	} else
	{
		return _planesArr[Side::down].a*point.x+_planesArr[Side::down].b*point.y+_planesArr[Side::down].c*point.z+_planesArr[Side::down].d>0 ? 1 : -1;
	}
}

int YandereCamera::z_point_side(YanPosition point)
{
	if(_planesArr[Side::back].a*point.x+_planesArr[Side::back].b*point.y+_planesArr[Side::back].c*point.z+_planesArr[Side::back].d>0
	&& _planesArr[Side::forward].a*point.x+_planesArr[Side::forward].b*point.y+_planesArr[Side::forward].c*point.z+_planesArr[Side::forward].d>0)
	{
		return 0;
	} else
	{
		return _planesArr[Side::back].a*point.x+_planesArr[Side::back].b*point.y+_planesArr[Side::back].c*point.z+_planesArr[Side::back].d>0 ? 1 : -1;
	}
}

bool YandereCamera::cube_in_frustum(YanPosition middle, float size)
{	
	for(int i = 0; i < 6; ++i)
	{
		if(_planesArr[i].a * (middle.x-size) + _planesArr[i].b * (middle.y-size) + _planesArr[i].c * (middle.z-size) > -_planesArr[i].d)
			continue;
			
		if(_planesArr[i].a * (middle.x+size) + _planesArr[i].b * (middle.y-size) + _planesArr[i].c * (middle.z-size) > -_planesArr[i].d)
			continue;
			
		if(_planesArr[i].a * (middle.x-size) + _planesArr[i].b * (middle.y-size) + _planesArr[i].c * (middle.z+size) > -_planesArr[i].d)
			continue;
			
		if(_planesArr[i].a * (middle.x+size) + _planesArr[i].b * (middle.y-size) + _planesArr[i].c * (middle.z+size) > -_planesArr[i].d)
			continue;
			
		if(_planesArr[i].a * (middle.x-size) + _planesArr[i].b * (middle.y+size) + _planesArr[i].c * (middle.z-size) > -_planesArr[i].d)
			continue;
			
		if(_planesArr[i].a * (middle.x+size) + _planesArr[i].b * (middle.y+size) + _planesArr[i].c * (middle.z-size) > -_planesArr[i].d)
			continue;
			
		if(_planesArr[i].a * (middle.x-size) + _planesArr[i].b * (middle.y+size) + _planesArr[i].c * (middle.z+size) > -_planesArr[i].d)
			continue;
			
		if(_planesArr[i].a * (middle.x+size) + _planesArr[i].b * (middle.y+size) + _planesArr[i].c * (middle.z+size) > -_planesArr[i].d)
			continue;
		
		return false;
	}
	
	return true;
}


glm::mat4* YandereCamera::proj_view_matrix_ptr()
{
	return &_projViewMatrix;
}

glm::mat4* YandereCamera::view_matrix_ptr()
{
	assert(_viewMatrixExists);
	return &_viewMatrix;
}

glm::mat4* YandereCamera::projection_matrix_ptr()
{
	assert(_projectionMatrixExists);
	return &_projectionMatrix;
}

void YandereCamera::calculate_view()
{
	_viewMatrixExists = true;

	_cameraForward = glm::normalize(_cameraDirection);
	_cameraRight = glm::normalize(glm::cross(glm::vec3(0, 1, 0), _cameraDirection));
	_cameraUp = glm::cross(_cameraDirection, _cameraRight);

	_viewMatrix = glm::lookAt(_cameraPosition, _cameraPosition + _cameraForward, _cameraUp);
	
	calculate_planes();
}

void YandereCamera::calculate_planes()
{
	_projViewMatrix =  _projectionMatrix * _viewMatrix;
	
	_planesArr[Side::left] = {_projViewMatrix[0][3]+_projViewMatrix[0][0],
				  _projViewMatrix[1][3]+_projViewMatrix[1][0],
				  _projViewMatrix[2][3]+_projViewMatrix[2][0],
				  _projViewMatrix[3][3]+_projViewMatrix[3][0]};
				  
	_planesArr[Side::right] = {_projViewMatrix[0][3]-_projViewMatrix[0][0],
				   _projViewMatrix[1][3]-_projViewMatrix[1][0],
				   _projViewMatrix[2][3]-_projViewMatrix[2][0],
				   _projViewMatrix[3][3]-_projViewMatrix[3][0]};
				  
	_planesArr[Side::down] = {_projViewMatrix[0][3]+_projViewMatrix[0][1],
				  _projViewMatrix[1][3]+_projViewMatrix[1][1],
				  _projViewMatrix[2][3]+_projViewMatrix[2][1],
				  _projViewMatrix[3][3]+_projViewMatrix[3][1]};
				  
	_planesArr[Side::up] = {_projViewMatrix[0][3]-_projViewMatrix[0][1],
				_projViewMatrix[1][3]-_projViewMatrix[1][1],
				_projViewMatrix[2][3]-_projViewMatrix[2][1],
				_projViewMatrix[3][3]-_projViewMatrix[3][1]};
				  
	_planesArr[Side::back] = {_projViewMatrix[0][3]+_projViewMatrix[0][2],
				  _projViewMatrix[1][3]+_projViewMatrix[1][2],
				  _projViewMatrix[2][3]+_projViewMatrix[2][2],
				  _projViewMatrix[3][3]+_projViewMatrix[3][2]};
				  
	_planesArr[Side::forward] = {_projViewMatrix[0][3]-_projViewMatrix[0][2],
					 _projViewMatrix[1][3]-_projViewMatrix[1][2],
					 _projViewMatrix[2][3]-_projViewMatrix[2][2],
					 _projViewMatrix[3][3]-_projViewMatrix[3][2]};
}

//------------------------------------------------------------------------------------------------------------------
YandereModel::YandereModel(std::string stringModelPath)
{
	vertices.clear();
	indices.clear();

	if(stringModelPath.find("!d")==0)
	{
		std::string defaultModel = stringModelPath.substr(2);
		if(defaultModel == "CIRCLE")
		{
			float verticesCount = YandereConst::circleLOD*M_PI;
			float circumference = M_PI*2;

			vertices.reserve(circumference*verticesCount*(5/3)+5);

			for(int i = 0; i <= circumference*verticesCount; i+=3)
			{
				float scaleVar = (i/(float)verticesCount);
				vertices.push_back(std::sin(scaleVar));
				vertices.push_back(std::cos(scaleVar));
				vertices.push_back(0.0f);

				//texture uvs
				vertices.push_back(std::sin(scaleVar));
				vertices.push_back(std::cos(scaleVar));
			}

			indices.reserve(circumference*YandereConst::circleLOD+YandereConst::circleLOD);

			for(int i = 0; i <= circumference*YandereConst::circleLOD+YandereConst::circleLOD; i++)
			{
				indices.push_back(0);
				indices.push_back(i+2);
				indices.push_back(i+1);
			}
		} else if(defaultModel == "TRIANGLE")
		{
			vertices = {
			-1.0f, -1.0f, 0.0f,   0.0f, 0.0f,
			0.0f, 1.0f, 0.0f,	 0.5f, 1.0f,
			1.0f, -1.0f, 0.0f,	1.0f, 0.0f
			};

			indices = {
			0, 2, 1,
			};
		} else if(defaultModel == "SQUARE")
		{
			vertices = {
			-1, 1, 0,	0, 1,
			1, 1, 0,	 1, 1,
			1, -1, 0,	1, 0,
			-1, -1, 0,   0, 0,
			};

			indices = {
			0, 2, 1,
			2, 0, 3,
			};
		} else if(defaultModel == "CUBE")
		{
			vertices = {
			-1, 1, 1,	0, 1,
			1, -1, 1,	1, 0,
			-1, -1, 1,   0, 0,
			1, 1, 1,	 1, 1,
			-1, -1, 1,   0, 1,
			1, -1, 1,	1, 1,
			-1, -1, -1,  0, 0,
			1, -1, -1,   1, 0,
			-1, 1, 1,	0, 0,
			1, 1, 1,	 1, 0,
			-1, 1, -1,   0, 1,
			1, 1, -1,	1, 1,
			-1, 1, 1,	1, 1,
			-1, -1, 1,   1, 0,
			1, 1, 1,	 0, 1,
			1, -1, 1,	0, 0,
			};

			indices = {
			0, 2, 1,
			0, 1, 3,
			4, 7, 5,
			4, 6, 7,
			8, 9, 10,
			9, 11, 10,
			10, 7, 6,
			10, 11, 7,
			10, 13, 12,
			10, 6, 13,
			14, 7, 11,
			14, 15, 7,
			};
		} else if(defaultModel == "PYRAMID")
		{
			vertices = {
			0.0f, -1.0f, -1.0f,   0.0f, 0.0f,
			0.0f, 1.0f, 0.0f,	 0.5f, 0.5f,
			1.0f, -1.0f, 0.64f,   0.0f, 1.0f,
			-1.0f, -1.0f, 0.64f,  1.0f, 0.0f,
			};

			indices = {
			0, 2, 1,
			3, 1, 2,
			3, 0, 1,
			0, 3, 2,
			};
		} else
		{
			vertices = {};
			indices = {};
		}
	} else
	{
		std::filesystem::path modelPath(stringModelPath);

		std::string extension = modelPath.filename().extension().string();

		if(!parseModel(stringModelPath, extension))
		{
			throw std::runtime_error(std::string("error parsing: ") + modelPath.filename().string());
		}
	}
}

void YandereModel::set_current(unsigned vertexArrayBuffer, unsigned arrayBuffer, unsigned elementArrayBuffer)
{
	glBindBuffer(GL_ARRAY_BUFFER, vertexArrayBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertices.size(), vertices.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementArrayBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned) * indices.size(), indices.data(), GL_STATIC_DRAW);

	glBindVertexArray(arrayBuffer);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
}

bool YandereModel::parseModel(std::string modelPath, std::string fileFormat)
{
	if(fileFormat==".obj")
	{
		std::ifstream modelStream(modelPath);
		std::string currModelLine;

		std::vector<float> currVerts;
		std::vector<unsigned> vertIndices;
		std::vector<float> currUVs;
		std::vector<unsigned> uvIndices;

		std::vector<float> currNormals;
		std::vector<unsigned> normalIndices;

		std::vector<std::string> savedParams;
		std::vector<unsigned> repeatedVerts;

		while(std::getline(modelStream, currModelLine))
		{
			if(currModelLine.find("#")!=-1)
			{
				currModelLine = currModelLine.substr(0, currModelLine.find("#"));
			}

			if(currModelLine.length()==0)
			{
				continue;
			}

			if(currModelLine.find("vt")==0)
			{
				std::stringstream texuvStream(currModelLine.substr(3));

				float x, y;

				texuvStream >> x >> y;

				currUVs.reserve(3);
				currUVs.push_back(x);
				currUVs.push_back(y);
			} else if(currModelLine.find("vn")==0)
			{
				std::stringstream normalStream(currModelLine.substr(3));

				float x, y, z;

				normalStream >> x >> y >> z;

				currNormals.reserve(3);
				currNormals.push_back(x);
				currNormals.push_back(y);
				currNormals.push_back(z);
			} else if(currModelLine.find("v")==0)
			{
				std::stringstream vertexStream(currModelLine.substr(2));

				float x, y, z;

				vertexStream >> x >> y >> z;

				currVerts.reserve(3);
				currVerts.push_back(x);
				currVerts.push_back(y);
				currVerts.push_back(z);
			} else if(currModelLine.find("f")==0)
			{
				std::vector<std::string> params = stringSplit(currModelLine.substr(2), " ");

				size_t paramsSize = params.size();

				if(params[paramsSize-1].length()==0)
				{
					params.pop_back();
				}

				std::vector<unsigned> order = {0, 1, 2};

				if(paramsSize>3)
				{
					for(int i = 3; i < paramsSize; i++)
					{
						order.push_back(0);
						order.push_back(i-1);
						order.push_back(i);
					}
				}

				for(const auto& p : order)
				{
					auto findIndex = std::find(savedParams.begin(), savedParams.end(), params[p]);

					if(findIndex!=savedParams.end())
					{
						repeatedVerts.push_back((unsigned)(vertIndices.size()-std::distance(savedParams.begin(), findIndex)));
					} else
					{
						repeatedVerts.push_back(0);
					}

					savedParams.push_back(params[p]);

					std::vector<std::string> faceVals = stringSplit(params[p], "/");

					vertIndices.push_back((unsigned)(std::stoi(faceVals[0]))-1);

					if(faceVals.size()>=2 && faceVals[1].length()!=0)
					{
						uvIndices.push_back((unsigned)(std::stoi(faceVals[1])-1));
					} else
					{
						uvIndices.push_back(UINT_MAX);
					}

					if(faceVals.size()==3)
					{
						normalIndices.push_back((unsigned)(std::stoi(faceVals[2])-1));
					} else
					{
						normalIndices.push_back(UINT_MAX);
					}
				}
			} else
			{
				//std::cout << currModelLine << std::endl;
			}
		}

		size_t uvsSize = currUVs.size();
		size_t uvIndicesSize = uvIndices.size();

		vertices.clear();
		indices.clear();

		int indexingOffset = 0;

		for(int v = 0; v < vertIndices.size(); v++)
		{
			if(repeatedVerts[v]==0)
			{
				vertices.push_back(currVerts[vertIndices[v]*3]);
				vertices.push_back(currVerts[vertIndices[v]*3+1]);
				vertices.push_back(currVerts[vertIndices[v]*3+2]);

				indices.push_back(v-indexingOffset);

				if(uvIndices[v]!=UINT_MAX)
				{
					vertices.push_back(currUVs[uvIndices[v]*2]);
					vertices.push_back(currUVs[uvIndices[v]*2+1]);
				} else
				{
					vertices.push_back(0);
					vertices.push_back(0);
				}
			} else
			{
				indices.push_back(indices[v-repeatedVerts[v]]);
				indexingOffset++;
			}
		}

		modelStream.close();

		return true;
	} else
	{
		return false;
	}
}

std::vector<float>* YandereModel::getVerticesPtr()
{
	return &vertices;
}

std::vector<unsigned>* YandereModel::getIndicesPtr()
{
	return &indices;
}

//------------------------------------------------------------------------------------------------------------------
YandereTexture::YandereTexture(std::string stringImagePath) : _empty(false)
{
	if(stringImagePath.find("!d")==0)
	{
		std::string defaultTexture = stringImagePath.substr(2);
		if(defaultTexture == "SOLID")
		{
			_image = YandereImage();
			_image.width = 1;
			_image.height = 1;
			_textureType = GL_RGBA;
			_image.image = {255, 255, 255, 255};
		}
	} else
	{
		std::filesystem::path imagePath(stringImagePath);

		std::string extension = imagePath.filename().extension().string();

		if(!parse_image(stringImagePath, extension))
		{
			std::cout << "error parsing: " << imagePath.filename() << std::endl;
		}
	}
}

YandereTexture::YandereTexture(YandereImage image) : _image(image), _empty(false)
{
	_textureType = set_texture_type(image.bpp);
	_image.flip();
}

void YandereTexture::set_current(unsigned textureBuffer)
{
	assert(!_empty);

	glBindTexture(GL_TEXTURE_2D, textureBuffer);

	switch(_textureType)
	{
		case GL_RED:
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			break;
		case GL_RG:
			glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
			break;
		case GL_RGB:
			glPixelStorei(GL_UNPACK_ALIGNMENT, 3);
			break;
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, _textureType, _image.width, _image.height, 0, _textureType, GL_UNSIGNED_BYTE, _image.image.data());
	glGenerateMipmap(GL_TEXTURE_2D);
}

int YandereTexture::width()
{
	return _image.width;
}

int YandereTexture::height()
{
	return _image.height;
}

bool YandereTexture::parse_image(std::string imagePath, std::string fileFormat)
{
	if(YandereImage::can_parse(fileFormat))
	{
		_image = YandereImage(imagePath);
		_image.flip();

		_textureType = set_texture_type(_image.bpp);

		return true;
	}

	return false;
}

unsigned YandereTexture::set_texture_type(uint8_t bpp)
{
	switch(bpp)
	{
		case 4:
			return GL_RGBA;
		case 3:
			return GL_RGB;
		case 2:
			return GL_RG;
		case 1:
		default:
			return GL_RED;
	}
}

//----------------------------------------------------------------------------------------------------------------

YandereShader::YandereShader(std::string shaderText) : _shaderText(shaderText)
{
}

std::string& YandereShader::text()
{
	return _shaderText;
}



YandereShaderProgram::YandereShaderProgram(unsigned programID) : _programID(programID)
{
	matrix_setup();
}

unsigned YandereShaderProgram::add_num(std::string name)
{
	return add_any(_shaderProps, 0, 1, name);
}

unsigned YandereShaderProgram::add_vec2(std::string name)
{
	return add_any(_shaderPropsVec2, YVec2{}, 2, name);
}

unsigned YandereShaderProgram::add_vec3(std::string name)
{
	return add_any(_shaderPropsVec3, YVec3{}, 3, name);
}

unsigned YandereShaderProgram::add_vec4(std::string name)
{
	return add_any(_shaderPropsVec4, YVec4{}, 4, name);
}


void YandereShaderProgram::apply_uniforms()
{
	std::array<int, 4> vecIndexCounts = {0, 0, 0, 0};

	for(Prop& prop : _propsVec)
	{
		switch(prop.length)
		{
			case 4:
			{
				YVec4 v4Prop = _shaderPropsVec4[vecIndexCounts[3]];
				glUniform4f(prop.location, v4Prop.x, v4Prop.y, v4Prop.z, v4Prop.w);
				break;
			}
			
			case 3:
			{
				YVec3 v3Prop = _shaderPropsVec3[vecIndexCounts[2]];
				glUniform3f(prop.location, v3Prop.x, v3Prop.y, v3Prop.z);
				break;
			}
				
			case 2:
			{
				YVec2 v2Prop = _shaderPropsVec2[vecIndexCounts[1]];
				glUniform2f(prop.location, v2Prop.x, v2Prop.y);
				break;
			}
		
			default:
				glUniform1f(prop.location, _shaderProps[vecIndexCounts[0]]);
				break;
		}
		
		++vecIndexCounts[prop.length-1];
	}
}


template<typename T, typename V>
unsigned YandereShaderProgram::add_any(T& type, V vType, int index, std::string name)
{
	unsigned propLocation = glGetUniformLocation(_programID, name.c_str());
	assert(propLocation!=-1);
	
	_propsVec.push_back(Prop{index, propLocation});
	
	type.push_back(vType);
	
	return (type.size()-1)*4;
}


unsigned YandereShaderProgram::program() const
{
	return _programID;
}


void YandereShaderProgram::matrix_setup()
{
	glUseProgram(_programID);

	_viewMat = glGetUniformLocation(_programID, "viewMat");
	_projectionMat = glGetUniformLocation(_programID, "projectionMat");
}

unsigned YandereShaderProgram::view_mat() const
{
	return _viewMat;
}

unsigned YandereShaderProgram::projection_mat() const
{
	return _projectionMat;
}

//----------------------------------------------------------------------------------------------------------------

YandereInitializer::YandereInitializer()
{
	load_default_models();
	_textureMap["!dSOLID"] = YandereTexture("!dSOLID");
}

YandereInitializer::~YandereInitializer()
{
	if(_glewInitialized)
	{
		if(glIsBuffer(_vertexBufferObjectID))
		{
			glDeleteBuffers(1, &_vertexBufferObjectID);
		}
		if(glIsBuffer(_elementObjectBufferID))
		{
			glDeleteBuffers(1, &_elementObjectBufferID);
		}
		if(glIsVertexArray(_vertexArrayObjectID))
		{
			glDeleteVertexArrays(1, &_vertexArrayObjectID);
		}
		for(const auto& shader : _shaderProgramMap)
		{
			if(glIsProgram(shader.second.program()))
			{
				glDeleteProgram(shader.second.program());
			}
		}
		if(glIsBuffer(_textureBufferObjectID))
		{
			glDeleteBuffers(1, &_textureBufferObjectID);
		}
	}
	
	//c libraries :/
	for(auto& [name, face] : _fontsMap)
	{
		FT_Done_Face(face);
	}
	
	if(_fontInit)
	{
		FT_Done_FreeType(_ftLib);
	}
}

void YandereInitializer::set_draw_model(std::string modelName)
{
	if(modelName!=_currentModelUsed)
	{
		_currentModelSize = _modelMap[modelName].getIndicesPtr()->size();
		_currentModelUsed = modelName;

		assert(_modelMap.count(modelName)!=0);

		_modelMap[modelName].set_current(_vertexBufferObjectID, _vertexArrayObjectID, _elementObjectBufferID);
	}
}

void YandereInitializer::set_draw_texture(std::string textureName)
{
	if(textureName!=_currentTextureUsed)
	{
		assert(_textureMap.count(textureName)!=0);
		_textureMap[textureName].set_current(_textureBufferObjectID);
	}
}

void YandereInitializer::set_draw_camera(YandereCamera* camera)
{
	_mainCamera = camera;
	update_matrix_pointers(_mainCamera);
}

glm::mat4 YandereInitializer::calculate_matrix(YanTransforms transforms)
{
	glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(transforms.position.x, transforms.position.y, transforms.position.z));
	modelMatrix = glm::rotate(modelMatrix, transforms.rotation, glm::vec3(transforms.axis.x, transforms.axis.y, transforms.axis.z));
	modelMatrix = glm::scale(modelMatrix, glm::vec3(transforms.scale.x, transforms.scale.y, transforms.scale.z));
	return modelMatrix;
}

void YandereInitializer::config_GL_buffers(bool stencil, bool antialiasing, bool culling)
{
	int success;

	if(antialiasing)
	{
		glEnable(GL_MULTISAMPLE);
	} else
	{
		glDisable(GL_MULTISAMPLE);
	}

	if(culling)
	{
		glEnable(GL_CULL_FACE);
		glFrontFace(GL_CCW);
	} else
	{
		glDisable(GL_CULL_FACE);
	}

	if(stencil)
	{
		glEnable(GL_STENCIL_TEST);
	} else
	{
		glDisable(GL_STENCIL_TEST);
	}
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_DEPTH_TEST);

	glGenVertexArrays(1, &_vertexArrayObjectID);

	glGenBuffers(1, &_vertexBufferObjectID);

	glGenBuffers(1, &_elementObjectBufferID);

	glGenBuffers(1, &_textureBufferObjectID);

	set_draw_model("!dTRIANGLE");
}

void YandereInitializer::update_matrix_pointers(YandereCamera* camera)
{
	_viewMatrix = camera->view_matrix_ptr();
	_projectionMatrix = camera->projection_matrix_ptr();
}

void YandereInitializer::load_shaders_from(std::string shadersFolder)
{
	std::filesystem::path shadersPath(shadersFolder);

	if(!std::filesystem::exists(shadersPath))
	{
		throw std::runtime_error("shader folder not found");
	}

	for(const auto& file : std::filesystem::directory_iterator(shadersPath))
	{
		std::ifstream shaderSrcFile(file.path().string());
		std::string shaderStr;

		std::string shaderFilename = file.path().filename().string();

		if(shaderSrcFile.good())
		{
			shaderStr = std::string(std::istreambuf_iterator(shaderSrcFile), {});

			shaderSrcFile.close();
		} else
		{
			std::cout << "error reading a shader file" << std::endl;
		}

		_shaderMap[shaderFilename] = YandereShader(shaderStr);
	}
}

void YandereInitializer::load_models_from(std::string modelsFolder)
{
	std::filesystem::path modelsPath(modelsFolder);

	if(!std::filesystem::exists(modelsPath))
	{
		throw std::runtime_error("models folder not found");
	}

	for(const auto& file : std::filesystem::directory_iterator(modelsPath))
	{
		std::string modelFilename = file.path().filename().stem().string();

		_modelMap[modelFilename] = std::move(YandereModel(file.path().string()));
	}
}

void YandereInitializer::load_default_models()
{
	std::string dName;
	std::vector<std::string> defaultNames = {"!dCIRCLE", "!dSQUARE", "!dTRIANGLE", "!dPYRAMID", "!dCUBE"};
	for(const auto& dName : defaultNames)
	{
		_modelMap[dName] = std::move(YandereModel(dName));
	}
}

void YandereInitializer::load_textures_from(std::string texturesFolder)
{
	std::filesystem::path texturesPath(texturesFolder);

	if(!std::filesystem::exists(texturesPath))
	{
		throw std::runtime_error("textures folder not found");
	}

	for(const auto& file : std::filesystem::directory_iterator(texturesPath))
	{
		std::string textureFilename = file.path().filename().stem().string();

		_textureMap[textureFilename] = std::move(YandereTexture(file.path().string()));
	}
}

void YandereInitializer::load_font(std::string fPath)
{
	std::filesystem::path fontPath(fPath);
	std::string fontString = fontPath.string();
	
	if(!std::filesystem::exists(fontPath))
	{
		throw std::runtime_error("font file not found");
	}
	
	if(!_fontInit)
	{	
		if(FT_Error err = FT_Init_FreeType(&_ftLib); err!=FT_Err_Ok)
		{
			throw std::runtime_error("[ERROR_"+std::to_string(err)+"] couldnt load freetype lib");
		}
		_fontInit = true;
	}
	
	FT_Face face;
	if(FT_Error err = FT_New_Face(_ftLib, fontString.c_str(), 0, &face); err!=FT_Err_Ok)
	{
		throw std::runtime_error("[ERROR_"+std::to_string(err)+"] loading font at: "+fontPath.string());
	}
	
	_fontsMap[fontPath.filename().stem().string()] = std::move(face);
}

unsigned YandereInitializer::create_shader_program(std::vector<std::string> shaderIDArr)
{
	assert(!_shaderMap.empty());

	int success;

	const char* vertexShaderSrc = nullptr;
	const char* fragmentShaderSrc = nullptr;
	const char* geometryShaderSrc = nullptr;

	for(const auto& shader : shaderIDArr)
	{
		std::string currExtension = std::filesystem::path(shader).extension().string();
	
		if(_shaderMap.count(shader)==0)
			throw std::runtime_error("shader file doesnt exist");
	
		if(currExtension==".fragment")
		{
			fragmentShaderSrc = _shaderMap[shader].text().c_str();
		} else if(currExtension==".vertex")
		{
			vertexShaderSrc = _shaderMap[shader].text().c_str();
		} else if(currExtension==".geometry")
		{
			geometryShaderSrc = _shaderMap[shader].text().c_str();
		} else
		{
			throw std::runtime_error("shader file isnt a .fragment, .vertex or .geometry");
		}
	}

	unsigned vertexShaderID;
	unsigned fragmentShaderID;
	unsigned geometryShaderID;

	unsigned currProgram = glCreateProgram();

	if(vertexShaderSrc!=nullptr)
	{
		vertexShaderID = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertexShaderID, 1, &vertexShaderSrc, NULL);
		glCompileShader(vertexShaderID);

		glGetShaderiv(vertexShaderID, GL_COMPILE_STATUS, &success);
		if(!success) output_error(vertexShaderID);

		glAttachShader(currProgram, vertexShaderID);
	}
	
	if(fragmentShaderSrc!=nullptr)
	{
		fragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragmentShaderID, 1, &fragmentShaderSrc, NULL);
		glCompileShader(fragmentShaderID);

		glGetShaderiv(fragmentShaderID, GL_COMPILE_STATUS, &success);
		if(!success) output_error(fragmentShaderID);

		glAttachShader(currProgram, fragmentShaderID);
	}

	if(geometryShaderSrc!=nullptr)
	{
		geometryShaderID = glCreateShader(GL_GEOMETRY_SHADER);
		glShaderSource(geometryShaderID, 1, &fragmentShaderSrc, NULL);
		glCompileShader(geometryShaderID);

		glGetShaderiv(geometryShaderID, GL_COMPILE_STATUS, &success);
		if(!success) output_error(geometryShaderID);

		glAttachShader(currProgram, geometryShaderID);
	}

	glLinkProgram(currProgram);

	glGetProgramiv(currProgram, GL_LINK_STATUS, &success);
	if(!success) output_error(currProgram, true);
	
	_shaderProgramMap[_lastProgramID] = YandereShaderProgram(currProgram);

	if(vertexShaderSrc!=nullptr) glDeleteShader(vertexShaderID);
	if(fragmentShaderSrc!=nullptr) glDeleteShader(fragmentShaderID);
	if(geometryShaderSrc!=nullptr) glDeleteShader(geometryShaderID);
	
	return _lastProgramID++;
}

YandereShaderProgram* YandereInitializer::shader_program_ptr(unsigned programID)
{
	return &_shaderProgramMap[programID];
}


void YandereInitializer::set_shader_program(unsigned programID)
{
	assert(_shaderProgramMap.count(programID)!=0);
	
	glUseProgram(_shaderProgramMap[programID].program());
	
	_storedShaderID = programID;
}

void YandereInitializer::apply_uniforms()
{
	YandereShaderProgram* programPtr = &_shaderProgramMap[_storedShaderID];

	glUniformMatrix4fv(programPtr->view_mat(), 1, GL_FALSE, glm::value_ptr(*_viewMatrix));
	glUniformMatrix4fv(programPtr->projection_mat(), 1, GL_FALSE, glm::value_ptr(*_projectionMatrix));

	programPtr->apply_uniforms();
}

YandereText YandereInitializer::create_text(std::string text, std::string fontName, int size, float x, float y)
{	
	if(!_glewInitialized)
	{
		throw std::runtime_error("creating text before initializing yandereinitializer");
	}

	int width, height;
	
	std::vector<std::vector<uint8_t>> letterPixels;
	std::vector<LetterData> letters;
	
	int maxWidth = 0;
	height = 0;
	
	FT_Face currFace = _fontsMap[fontName];
	
	FT_Set_Pixel_Sizes(currFace, 0, size);
	
	const int loadChars = 128;
	letters.reserve(loadChars);
	letterPixels.reserve(loadChars);
	
	for(int i = 0; i < loadChars; ++i)
	{
		FT_Load_Char(currFace, i, FT_LOAD_RENDER);
		
		if(currFace->glyph->bitmap.width>maxWidth)
		{
			maxWidth = currFace->glyph->bitmap.width;
		}
		if(currFace->glyph->bitmap.rows>height)
		{
			height = currFace->glyph->bitmap.rows;
		}
		
		letters.emplace_back(LetterData{static_cast<float>(currFace->glyph->bitmap.width), 
		static_cast<float>(currFace->glyph->bitmap.rows),
		static_cast<float>(currFace->glyph->bitmap_left),
		static_cast<float>(currFace->glyph->bitmap_top), 
		static_cast<float>(currFace->glyph->advance.x)});
		
		std::vector<uint8_t> lPixels;
		
		int pixelsAmount = currFace->glyph->bitmap.width * currFace->glyph->bitmap.rows;
		
		uint8_t* bitmapArr = currFace->glyph->bitmap.buffer;
		
		lPixels.reserve(pixelsAmount);
		for(int l = 0; l < pixelsAmount; ++l)
		{
			lPixels.emplace_back(bitmapArr[l]);
		}
		
		letterPixels.emplace_back(lPixels);
	}
	
	width = maxWidth*loadChars;
	
	std::vector<uint8_t> texData(width*height, 0);
	
	for(int c = 0; c < loadChars; ++c)
	{
		int xStart = maxWidth*c;
		
		int letterWidth = letters[c].width;
		int letterTotal = letterWidth*letters[c].height;
		for(int l = 0; l < letterTotal; ++l)
		{
			int letterX = l%letterWidth;
			int letterY = l/letterWidth;
			texData[xStart+letterX+letterY*width] = letterPixels[c][l];
		}
		
		std::string letterModelName = "!fg"+std::to_string(c);
		YandereModel letterModel;
		
		float xMin = xStart/static_cast<float>(width);
		
		float xMax = xMin+letters[c].width/static_cast<float>(width);
		float yMax = 1.0f-letters[c].height/static_cast<float>(height);
		
		letterModel.vertices = {
		-1, 1, 0,	 xMin, 1,
		1, 1, 0,	  xMax, 1,
		1, -1, 0,	 xMax, yMax,
		-1, -1, 0,	xMin, yMax};
		
		letterModel.indices = {
		0, 2, 1,
		2, 0, 3};
		
		_modelMap[letterModelName] = letterModel;
		
		letters[c].originY *= 2;
	}

	std::string textureName = "!f" + fontName;

	YandereImage img;
	img.width = width;
	img.height = height;
	img.bpp = 1;
	img.image = texData;
	
	YandereTexture fontTexture = YandereTexture(img);
	
	_textureMap[textureName] = std::move(fontTexture);
	
	YandereText outText = YandereText(this, size, letters, textureName);
	outText.set_position(x, y);
	outText.change_text(text);

	return std::move(outText);
}

void YandereInitializer::output_error(unsigned objectID, bool isProgram)
{
	char errorInfo[512];
	if(isProgram)
	{
		glGetProgramInfoLog(objectID, 512, NULL, errorInfo);
	} else
	{
		glGetShaderInfoLog(objectID, 512, NULL, errorInfo);
	}
	
	std::stringstream sstream;
	
	sstream << "LOAD ERROR: " << errorInfo;
	
	throw std::runtime_error(sstream.str());
}

void YandereInitializer::do_glew_init(bool stencil, bool antialiasing, bool culling)
{
	GLenum err = glewInit();
	if(err!=GLEW_OK)
	{
		//it gives me an unknown error for no reason so im ignoring it
		if(err==4)
		{
			glewGetErrorString(err);
		} else
		{
			throw std::runtime_error(reinterpret_cast<const char*>(glewGetErrorString(err)));
		}
		
		#ifdef YANDEBUG
		glDebugMessageCallback(YandereInitializer::debug_callback, NULL);
		#endif
	}

	_glewInitialized = true;

	config_GL_buffers(stencil, antialiasing, culling);
}

bool YandereInitializer::glew_initialized()
{
	return _glewInitialized;
}

void GLAPIENTRY YandereInitializer::debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
	std::cout << "opengl error " << type << ": " << message << std::endl;
}

//---------------------------------------------------------------------------------------------------------------

YandereObject::YandereObject() : _usedModel(""), _usedTexture("")
{
}

YandereObject::YandereObject(YandereInitializer* yanIniter, std::string usedModel, std::string usedTexture, YanTransforms transform, YanColor color, YanBorder border)
 : _transform(transform), _color(color), _border(border), _yanInitializer(yanIniter), _usedModel(usedModel), _usedTexture(usedTexture)
{
	assert(_yanInitializer->glew_initialized());
	yGL_setup(border.enabled);

	mats_update();
}

void YandereObject::yGL_setup(bool borders)
{
	if(borders)
	{
		glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	}
	
	
	glGenBuffers(1, &_vertexBufferObject);;
}

void YandereObject::set_position(YanPosition position)
{
	_transform.position = position;
	
	mats_update();
}

void YandereObject::translate(YanPosition positionDelta)
{
	_transform.position.x += positionDelta.x;
	_transform.position.y += positionDelta.y;
	_transform.position.z += positionDelta.z;
		
	mats_update();
}

void YandereObject::set_scale(YanScale scale)
{
	_transform.scale = scale;
		
	mats_update();
}

void YandereObject::set_rotation(float rotation)
{
	_transform.rotation = rotation;

	mats_update();
}

void YandereObject::rotate(float rotationDelta)
{
	_transform.rotation += rotationDelta;

	mats_update();
}

void YandereObject::set_rotationAxis(YanPosition axis)
{
	_transform.axis = axis;

	mats_update();
}

void YandereObject::set_color(YanColor color)
{
	_color = color;

	_objData.color = _color;
}

void YandereObject::set_border(YanBorder border)
{
	_border = border;
}

void YandereObject::draw_update()
{
	assert(_usedModel!="" && _usedTexture!="");
	_yanInitializer->set_draw_model(_usedModel);
	_yanInitializer->set_draw_texture(_usedTexture);

	assert(_yanInitializer->_mainCamera!=nullptr);

	size_t bufferSize = sizeof(float)*(sizeof(YandereObject::ObjectData)/sizeof(float));

	glBindBuffer(GL_ARRAY_BUFFER, _vertexBufferObject);
	glBufferData(GL_ARRAY_BUFFER, bufferSize, &_objData, GL_STATIC_DRAW);

	glBindVertexArray(_yanInitializer->_vertexArrayObjectID);

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, bufferSize, reinterpret_cast<void*>(0));

	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, bufferSize, reinterpret_cast<void*>((sizeof(float)*4)));

	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, bufferSize, reinterpret_cast<void*>((2*sizeof(float)*4)));

	glEnableVertexAttribArray(5);
	glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, bufferSize, reinterpret_cast<void*>((3*sizeof(float)*4)));

	glEnableVertexAttribArray(6);
	glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, bufferSize, reinterpret_cast<void*>((4*sizeof(float)*4)));

	glVertexAttribDivisor(2, 1);
	glVertexAttribDivisor(3, 1);
	glVertexAttribDivisor(4, 1);
	glVertexAttribDivisor(5, 1);
	glVertexAttribDivisor(6, 1);

	_yanInitializer->apply_uniforms();

	if(_border.enabled)
	{
		glClear(GL_STENCIL_BUFFER_BIT);
		glStencilFunc(GL_ALWAYS, 1, 0xFF);
		glStencilMask(0xFF);
	}

	glDrawElements(GL_TRIANGLES, _yanInitializer->_currentModelSize, GL_UNSIGNED_INT, 0);

	if(_border.enabled)
	{
		//outlines are done by rescaling the model from the center which won't work with non centered models
		//fixable with scaling by normals (dont care about it)

		glDisable(GL_DEPTH_TEST);

		glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
		glStencilMask(0x00);

		//it assumes the default flat shading shader is 0 whatever, shouldnt have used enums for this
		unsigned restoreShader = _yanInitializer->_storedShaderID;
		_yanInitializer->set_shader_program(0);

		glBufferData(GL_ARRAY_BUFFER, bufferSize, &_scaledObjData, GL_STATIC_DRAW);

		_yanInitializer->apply_uniforms();

		glDrawElements(GL_TRIANGLES, _yanInitializer->_currentModelSize, GL_UNSIGNED_INT, 0);

		_yanInitializer->set_shader_program(restoreShader);

		glStencilMask(0xFF);
		glStencilFunc(GL_ALWAYS, 1, 0xFF);

		glEnable(GL_DEPTH_TEST);
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void YandereObject::mats_update()
{
	_scaledTransform = _transform;
	_scaledTransform.scale.x *= _border.width;
	_scaledTransform.scale.y *= _border.width;
	_scaledTransform.scale.z *= _border.width;

	glm::mat4 tempMat = _yanInitializer->calculate_matrix(_transform);

	_objData.color = _color;

	for(int m = 0; m<4; ++m)
	{
		for(int v = 0; v<4; ++v)
		{
			_objData.matrix[m*4+v] = tempMat[m][v];
		}
	}

	if(_border.enabled)
	{
		tempMat = _yanInitializer->calculate_matrix(_scaledTransform);

		_scaledObjData.color  = _border.color;

		for(int m = 0; m<4; ++m)
		{
			for(int v = 0; v<4; ++v)
			{
				_scaledObjData.matrix[m*4+v] = tempMat[m][v];
			}
		}
	}
}

//---------------------------------------------------------------------------------------------------------------

YandereObjects::YandereObjects() : _transformsSize(0)
{
}

YandereObjects::YandereObjects(YandereInitializer* yanIniter, std::string usedModel, std::string usedTexture, std::vector<YanTransforms> transformsInstanced, size_t tsize, std::vector<YanColor> colors, YanBorder border)
 : _transforms(transformsInstanced), _transformsSize(tsize), _colors(colors), _border(border), _yanInitializer(yanIniter), _usedModel(usedModel), _usedTexture(usedTexture)
{
	assert(_yanInitializer->glew_initialized());
	yGL_setup(border.enabled);

	_singleColor=!(colors.size()>1);

	if(transformsInstanced.size()!=0)
	{
		mats_update();
	}
}

bool YandereObjects::empty()
{
	return (_transformsSize==0);
}

void YandereObjects::yGL_setup(bool borders)
{
	_instancedData.reserve(_transformsSize);
	_scaledInstancedData.reserve(_transformsSize);
	
	if(borders)
	{
		glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	}
	
	
	glGenBuffers(1, &_instanceVertexBufferObject);;
}

void YandereObjects::set_positions(std::vector<YanPosition> positions)
{
	assert(positions.size()==_transformsSize);
	for(int i = 0; i < _transformsSize; i++)
	{
		_transforms[i].position = positions[i];
	}
	mats_update();
}

void YandereObjects::set_position(YanPosition position)
{
	for(int i = 0; i < _transformsSize; i++)
	{
		_transforms[i].position = position;
	}
	mats_update();
}

void YandereObjects::translate(std::vector<YanPosition> positionDelta)
{
	assert(positionDelta.size()==_transformsSize);
	for(int i = 0; i < _transformsSize; i++)
	{
		_transforms[i].position.x += positionDelta[i].x;
		_transforms[i].position.y += positionDelta[i].y;
		_transforms[i].position.z += positionDelta[i].z;
	}
	mats_update();
}

void YandereObjects::translate(YanPosition positionDelta)
{
	for(int i = 0; i < _transformsSize; i++)
	{
		_transforms[i].position.x += positionDelta.x;
		_transforms[i].position.y += positionDelta.y;
		_transforms[i].position.z += positionDelta.z;
	}
	mats_update();
}

void YandereObjects::set_scales(std::vector<YanScale> scales)
{
	assert(scales.size()==_transformsSize);
	for(int i = 0; i < _transformsSize; i++)
	{
		_transforms[i].scale = scales[i];
	}
	mats_update();
}

void YandereObjects::set_rotations(std::vector<float> rotations)
{
	assert(rotations.size()==_transformsSize);
	for(int i = 0; i < _transformsSize; i++)
	{
		_transforms[i].rotation = rotations[i];
	}
	mats_update();
}

void YandereObjects::rotate(std::vector<float> rotationDeltas)
{
	assert(rotationDeltas.size()==_transformsSize);
	for(int i = 0; i < _transformsSize; i++)
	{
		_transforms[i].rotation += rotationDeltas[i];
	}
	mats_update();
}

void YandereObjects::set_rotationAxes(std::vector<YanPosition> axes)
{
	assert(axes.size()==_transformsSize);
	for(int i = 0; i < _transformsSize; i++)
	{
		_transforms[i].axis = axes[i];
	}
	mats_update();
}

void YandereObjects::set_colors(std::vector<YanColor> colors)
{
	assert(colors.size()==_colors.size());

	_colors = colors;
	
	for(size_t i = 0; i < _transformsSize; i++)
	{
		if(!_singleColor)
		{
			_instancedData[i].color = _colors[i];
		} else
		{
			_instancedData[i].color = _colors[0];
		}
	}
}

void YandereObjects::set_border(YanBorder border)
{
	_border = border;
}

void YandereObjects::draw_update()
{
	assert(!empty());

	_yanInitializer->set_draw_model(_usedModel);
	_yanInitializer->set_draw_texture(_usedTexture);
	
	assert(_yanInitializer->_mainCamera!=nullptr);

	size_t bufferSize = sizeof(float)*(sizeof(YandereObjects::ObjectData)/sizeof(float));

	glBindBuffer(GL_ARRAY_BUFFER, _instanceVertexBufferObject);
	glBufferData(GL_ARRAY_BUFFER, bufferSize * _transformsSize, _instancedData.data(), GL_STATIC_DRAW);

	glBindVertexArray(_yanInitializer->_vertexArrayObjectID);

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, bufferSize, reinterpret_cast<void*>(0));

	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, bufferSize, reinterpret_cast<void*>((sizeof(float)*4)));

	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, bufferSize, reinterpret_cast<void*>((2*sizeof(float)*4)));

	glEnableVertexAttribArray(5);
	glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, bufferSize, reinterpret_cast<void*>((3*sizeof(float)*4)));

	glEnableVertexAttribArray(6);
	glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, bufferSize, reinterpret_cast<void*>((4*sizeof(float)*4)));

	glVertexAttribDivisor(2, 1);
	glVertexAttribDivisor(3, 1);
	glVertexAttribDivisor(4, 1);
	glVertexAttribDivisor(5, 1);
	glVertexAttribDivisor(6, 1);

	_yanInitializer->apply_uniforms();

	if(_border.enabled)
	{
		glClear(GL_STENCIL_BUFFER_BIT);
		glStencilFunc(GL_ALWAYS, 1, 0xFF);
		glStencilMask(0xFF);
	}

	glDrawElementsInstanced(GL_TRIANGLES, _yanInitializer->_currentModelSize, GL_UNSIGNED_INT, 0, _transformsSize);

	if(_border.enabled)
	{
		//outlines are done by rescaling the model from the center which won't work with non centered models
		//fixable with scaling by normals (dont care about it)

		glDisable(GL_DEPTH_TEST);

		glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
		glStencilMask(0x00);

		//it assumes the default flat shading shader is 0 whatever, shouldnt have used enums for this
		unsigned restoreShader = _yanInitializer->_storedShaderID;
		_yanInitializer->set_shader_program(0);

		glBufferData(GL_ARRAY_BUFFER, bufferSize * _transformsSize, _scaledInstancedData.data(), GL_STATIC_DRAW);

		_yanInitializer->apply_uniforms();

		glDrawElementsInstanced(GL_TRIANGLES, _yanInitializer->_currentModelSize, GL_UNSIGNED_INT, 0, _transformsSize);

		_yanInitializer->set_shader_program(restoreShader);

		glStencilMask(0xFF);
		glStencilFunc(GL_ALWAYS, 1, 0xFF);

		glEnable(GL_DEPTH_TEST);
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void YandereObjects::mats_update()
{
	_scaledTransforms = _transforms;

	std::transform(_scaledTransforms.begin(), _scaledTransforms.end(), _scaledTransforms.begin(), [this](YanTransforms t)->YanTransforms {
	YanTransforms temp = t;
	temp.scale.x *= _border.width;
	temp.scale.y *= _border.width;
	temp.scale.z *= _border.width;
	return temp;});

	for(size_t i = 0; i < _transformsSize; i++)
	{
		glm::mat4 tempMat = _yanInitializer->calculate_matrix(_transforms[i]);

		if(!_singleColor)
		{
			_instancedData[i].color = _colors[i];
		} else
		{
			_instancedData[i].color = _colors[0];
		}

		for(int m = 0; m<4; ++m)
		{
			for(int v = 0; v<4; ++v)
			{
				_instancedData[i].matrix[m*4+v] = tempMat[m][v];
			}
		}

		if(_border.enabled)
		{
			tempMat = _yanInitializer->calculate_matrix(_scaledTransforms[i]);

			_scaledInstancedData[i].color  = _border.color;

			for(int m = 0; m<4; ++m)
			{
				for(int v = 0; v<4; ++v)
				{
					_scaledInstancedData[i].matrix[m*4+v] = tempMat[m][v];
				}
			}
		}
	}
}

//-------------------------------------------------------------------------------------------------------------------
YandereLines::YandereLines() : YandereObjects()
{
}

YandereLines::YandereLines(YandereInitializer* yanIniter, std::vector<std::array<YanPosition, 2>> points, size_t tsize, std::vector<float> widths, std::vector<YanColor> colors, YanBorder border)
 : _points(points), _widths(widths), YandereObjects(yanIniter, "!dSQUARE", "!dSOLID", {}, tsize, colors, border)
{
	calculate_variables();
}

void YandereLines::set_positions(std::vector<std::array<YanPosition, 2>> points)
{
	_points = points;

	calculate_variables();
}

void YandereLines::set_rotations(std::vector<float> rotations)
{
	for(size_t i = 0; i < _transformsSize; i++)
	{
		_transforms[i].rotation = rotations[i];
	}

	calculate_variables();
}

void YandereLines::set_widths(std::vector<float> widths)
{
	_widths = widths;

	for(size_t i = 0; i < _transformsSize; i++)
	{
		_transforms[i].scale.y = _widths[i];
	}

	mats_update();
}

void YandereLines::calculate_variables()
{
	float lineWidth, lineHeight, lineLength;
	float angleOffsetX, angleOffsetY;

	_transforms.clear();

	if(_transforms.size()!=_transformsSize)
	{
		_transforms.reserve(_transformsSize);
	}

	for(size_t i = 0; i < _transformsSize; i++)
	{
		lineWidth = (_points[i][1].x-_points[i][0].x);
		lineHeight = (_points[i][1].y-_points[i][0].y);

		lineLength = std::sqrt((lineWidth*lineWidth)+(lineHeight*lineHeight))/2;

		float rot = std::atan2(lineHeight, lineWidth);

		angleOffsetX = (1-std::cos(rot))*lineLength;
		angleOffsetY = (std::sin(rot))*lineLength;

		YanPosition pos = {_points[i][0].x+lineLength-angleOffsetX, _points[i][0].y+angleOffsetY, _points[i][0].z};

		_transforms.push_back({pos, lineLength, _widths[i], 1, rot, 0, 0, 1});
	}

	mats_update();
}

YandereText::YandereText()
{
}

YandereText::YandereText(YandereInitializer* yanInitializer, int size, std::vector<LetterData> letters, std::string textureName) : 
_yanInitializer(yanInitializer), _size(size), _letters(letters), _textureName(textureName)
{
}

void YandereText::draw_update()
{
	for(auto& obj : _letterObjs)
	{
		obj.draw_update();
	}
}

void YandereText::change_text(std::string newText)
{
	_letterObjs.clear();

	if(newText.length()==0)
	{
		return;
	}

	float lastX = x;
	
	_textHeight = 0;
	_textWidth = 0;

	_letterObjs.reserve(newText.length());
	for(int i = 0; i < newText.length(); ++i)
	{
		int letterIndex = newText[i];
	
		LetterData letter = _letters[letterIndex];
		
		if(letter.height>_textHeight)
		{
			_textHeight = letter.height;
		}
		
		YanPosition letterPos = YanPosition{
		lastX + letter.width,
		y - letter.height + letter.originY
		};
		
		YanTransforms letterTransform = YanTransforms{{letterPos.x, letterPos.y, letterPos.z},
		{letter.width, letter.height, 1}};
	

		lastX += letter.width+letter.originX+letter.hDist/64.0f;
		_textWidth += letter.width+letter.hDist/64.0f;
		if(i==newText.length()-1)
		{
			_textWidth -= letter.hDist/64.0f;
		}
		
		std::string letterMesh = "!fg"+std::to_string(letterIndex);
		
		YandereObject letterObj = YandereObject(_yanInitializer, letterMesh, _textureName, letterTransform);
		
		_letterObjs.emplace_back(std::move(letterObj));
	}
}

void YandereText::set_position(float x, float y)
{
	float xDiff = x-(this->x);
	float yDiff = y-(this->y);

	this->x = x;
	this->y = y;
	
	for(auto& obj : _letterObjs)
	{
		obj.translate({xDiff, yDiff, 0});
	}
}

std::array<float, 2> YandereText::getPosition()
{
	return {x, y};
}

float YandereText::text_width()
{
	return _textWidth;
}

float YandereText::text_height()
{
	return _textHeight*2;
}
