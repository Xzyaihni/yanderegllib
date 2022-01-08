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

YandereCamera::YandereCamera()
{
}

YandereCamera::YandereCamera(YanPosition position, YanPosition lookPosition) : _projectionMatrix(glm::mat4(1.0f))
{
	_cameraPosition = glm::vec3(position.x, position.y, position.z);
	_cameraDirection = glm::normalize(glm::vec3(lookPosition.x, lookPosition.y, lookPosition.z)-_cameraPosition);

	calculateView();
}

void YandereCamera::createProjection(float fov, float aspectRatio, std::array<float, 2> planes)
{
	_projectionMatrixExists = true;

	_projectionMatrix = glm::perspective(glm::radians(fov), aspectRatio, planes[0], planes[1]);
	clipClose = planes[0];
	clipFar = planes[1];
	
	calculatePlanes();
}

void YandereCamera::createProjection(std::array<float, 4> orthoBox, std::array<float, 2> planes)
{
	_projectionMatrixExists = true;

	_projectionMatrix = glm::ortho(orthoBox[0], orthoBox[1], orthoBox[2], orthoBox[3], planes[0], planes[1]);
	clipClose = planes[0];
	clipFar = planes[1];
	
	calculatePlanes();
}

void YandereCamera::setPosition(YanPosition position)
{
	_cameraPosition = glm::vec3(position.x, position.y, position.z);

	calculateView();
}

void YandereCamera::setPositionX(float x)
{
	_cameraPosition.x = x;

	calculateView();
}

void YandereCamera::setPositionY(float y)
{
	_cameraPosition.y = y;

	calculateView();
}

void YandereCamera::setPositionZ(float z)
{
	_cameraPosition.z = z;

	calculateView();
}

void YandereCamera::setRotation(float yaw, float pitch)
{
	_cameraDirection.x = std::cos(yaw) * std::cos(pitch);
	_cameraDirection.y = std::sin(pitch);
	_cameraDirection.z = std::sin(yaw) * std::cos(pitch);

	calculateView();
}

void YandereCamera::setDirection(std::array<float, 3> direction)
{
	_cameraDirection = glm::vec3(direction[0], direction[1], direction[2]);

	calculateView();
}

void YandereCamera::lookAt(std::array<float, 3> lookPosition)
{
	_cameraDirection = glm::normalize(glm::vec3(lookPosition[0], lookPosition[1], lookPosition[2])-_cameraPosition);

	calculateView();
}

void YandereCamera::translatePosition(YanPosition transPos)
{
	_cameraPosition += transPos.x*_cameraRight;
	_cameraPosition += transPos.y*_cameraUp;
	_cameraPosition += transPos.z*_cameraForward;

	calculateView();
}

YanPosition YandereCamera::position()
{
	return {_cameraPosition.x, _cameraPosition.y, _cameraPosition.z};
}

int YandereCamera::xPointSide(YanPosition point)
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

int YandereCamera::yPointSide(YanPosition point)
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

int YandereCamera::zPointSide(YanPosition point)
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

bool YandereCamera::cubeInFrustum(YanPosition middle, float size)
{	
	for(int i = 0; i < 6; ++i)
	{
		if(_planesArr[i].a * (middle.x-size) + _planesArr[i].b * (middle.y-size) + _planesArr[i].c * (middle.z-size) + _planesArr[i].d > 0)
			continue;
			
		if(_planesArr[i].a * (middle.x+size) + _planesArr[i].b * (middle.y-size) + _planesArr[i].c * (middle.z-size) + _planesArr[i].d > 0)
			continue;
			
		if(_planesArr[i].a * (middle.x-size) + _planesArr[i].b * (middle.y-size) + _planesArr[i].c * (middle.z+size) + _planesArr[i].d > 0)
			continue;
			
		if(_planesArr[i].a * (middle.x+size) + _planesArr[i].b * (middle.y-size) + _planesArr[i].c * (middle.z+size) + _planesArr[i].d > 0)
			continue;
			
		if(_planesArr[i].a * (middle.x-size) + _planesArr[i].b * (middle.y+size) + _planesArr[i].c * (middle.z-size) + _planesArr[i].d > 0)
			continue;
			
		if(_planesArr[i].a * (middle.x+size) + _planesArr[i].b * (middle.y+size) + _planesArr[i].c * (middle.z-size) + _planesArr[i].d > 0)
			continue;
			
		if(_planesArr[i].a * (middle.x-size) + _planesArr[i].b * (middle.y+size) + _planesArr[i].c * (middle.z+size) + _planesArr[i].d > 0)
			continue;
			
		if(_planesArr[i].a * (middle.x+size) + _planesArr[i].b * (middle.y+size) + _planesArr[i].c * (middle.z+size) + _planesArr[i].d > 0)
			continue;
		
		return false;
	}
	
	return true;
}


glm::mat4* YandereCamera::projViewMatrixPtr()
{
	return &_projViewMatrix;
}

glm::mat4* YandereCamera::viewMatrixPtr()
{
	assert(_viewMatrixExists);
	return &_viewMatrix;
}

glm::mat4* YandereCamera::projectionMatrixPtr()
{
	assert(_projectionMatrixExists);
	return &_projectionMatrix;
}

void YandereCamera::calculateView()
{
	_viewMatrixExists = true;

	_cameraForward = glm::normalize(_cameraDirection);
	_cameraRight = glm::normalize(glm::cross(glm::vec3(0, 1, 0), _cameraDirection));
	_cameraUp = glm::cross(_cameraDirection, _cameraRight);

	_viewMatrix = glm::lookAt(_cameraPosition, _cameraPosition + _cameraForward, _cameraUp);
	
	calculatePlanes();
}

void YandereCamera::calculatePlanes()
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
YandereModel::YandereModel()
{
}

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

		std::string extension = modelPath.filename().extension();

		if(!parseModel(stringModelPath, extension))
		{
			throw std::runtime_error(std::string("error parsing: ") + std::string(modelPath.filename()));
		}
	}
}

void YandereModel::setCurrent(unsigned vertexArrayBuffer, unsigned arrayBuffer, unsigned elementArrayBuffer)
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

YandereTexture::YandereTexture()
{
}

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

		std::string extension = imagePath.filename().extension();

		if(!parseImage(stringImagePath, extension))
		{
			std::cout << "error parsing: " << imagePath.filename() << std::endl;
		}
	}
}

YandereTexture::YandereTexture(YandereImage image) : _image(image), _empty(false)
{
	_textureType = setTextureType(image.bpp);
	_image.flip();
}

void YandereTexture::setCurrent(unsigned textureBuffer)
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

bool YandereTexture::parseImage(std::string imagePath, std::string fileFormat)
{
	if(YandereImage::canParse(fileFormat))
	{
		_image = YandereImage(imagePath);
		_image.flip();

		_textureType = setTextureType(_image.bpp);

		return true;
	}

	return false;
}

unsigned YandereTexture::setTextureType(uint8_t bpp)
{
	switch(_image.bpp)
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

YandereInitializer::YandereInitializer()
{
	loadDefaultModels();
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
			if(glIsProgram(shader.second))
			{
				glDeleteProgram(shader.second);
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

void YandereInitializer::setDrawModel(std::string modelName)
{
	if(modelName!=_currentModelUsed)
	{
		_currentModelSize = _modelMap[modelName].getIndicesPtr()->size();
		_currentModelUsed = modelName;

		assert(_modelMap.count(modelName)!=0);

		_modelMap[modelName].setCurrent(_vertexBufferObjectID, _vertexArrayObjectID, _elementObjectBufferID);
	}
}

void YandereInitializer::setDrawTexture(std::string textureName)
{
	if(textureName!=_currentTextureUsed)
	{
		assert(_textureMap.count(textureName)!=0);
		_textureMap[textureName].setCurrent(_textureBufferObjectID);
	}
}

void YandereInitializer::setDrawCamera(YandereCamera* camera)
{
	_mainCamera = camera;
	updateMatrixPointers(_mainCamera);
}

glm::mat4 YandereInitializer::calculateMatrix(YanTransforms transforms)
{
	glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(transforms.position.x, transforms.position.y, transforms.position.z));
	modelMatrix = glm::rotate(modelMatrix, transforms.rotation, glm::vec3(transforms.axis.x, transforms.axis.y, transforms.axis.z));
	modelMatrix = glm::scale(modelMatrix, glm::vec3(transforms.scale.x, transforms.scale.y, transforms.scale.z));
	return modelMatrix;
}

void YandereInitializer::configGLBuffers(bool stencil, bool antialiasing, bool culling)
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

	setDrawModel("!dTRIANGLE");
}

void YandereInitializer::updateMatrixPointers(YandereCamera* camera)
{
	_viewMatrix = camera->viewMatrixPtr();
	_projectionMatrix = camera->projectionMatrixPtr();
}

void YandereInitializer::loadShadersFrom(std::string shadersFolder)
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

		std::string shaderFilename = file.path().filename();

		if(shaderSrcFile.good())
		{
			shaderStr = std::string(std::istreambuf_iterator(shaderSrcFile), {});

			shaderSrcFile.close();
		} else
		{
			std::cout << "error reading a shader file" << std::endl;
		}

		assignShader(shaderStr, shaderFilename);
	}
}

void YandereInitializer::loadModelsFrom(std::string modelsFolder)
{
	std::filesystem::path modelsPath(modelsFolder);

	if(!std::filesystem::exists(modelsPath))
	{
		throw std::runtime_error("models folder not found");
	}

	for(const auto& file : std::filesystem::directory_iterator(modelsPath))
	{
		std::string modelFilename = file.path().filename().stem();

		_modelMap[modelFilename] = std::move(YandereModel(file.path()));
	}
}

void YandereInitializer::loadDefaultModels()
{
	std::string dName;
	std::vector<std::string> defaultNames = {"!dCIRCLE", "!dSQUARE", "!dTRIANGLE", "!dPYRAMID", "!dCUBE"};
	for(const auto& dName : defaultNames)
	{
		_modelMap[dName] = std::move(YandereModel(dName));
	}
}

void YandereInitializer::loadTexturesFrom(std::string texturesFolder)
{
	std::filesystem::path texturesPath(texturesFolder);

	if(!std::filesystem::exists(texturesPath))
	{
		throw std::runtime_error("textures folder not found");
	}

	for(const auto& file : std::filesystem::directory_iterator(texturesPath))
	{
		std::string textureFilename = file.path().filename().stem();

		_textureMap[textureFilename] = std::move(YandereTexture(file.path()));
	}
}

void YandereInitializer::loadFont(std::string fPath)
{
	std::filesystem::path fontPath(fPath);
	
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
	if(FT_Error err = FT_New_Face(_ftLib, fontPath.c_str(), 0, &face); err!=FT_Err_Ok)
	{
		throw std::runtime_error("[ERROR_"+std::to_string(err)+"] loading font at: "+fontPath.string());
	}
	
	_fontsMap[fontPath.filename().stem()] = std::move(face);
}

void YandereInitializer::createShaderProgram(unsigned programID, std::vector<std::string> shaderIDArr)
{
	assert(!_shaderMap.empty());

	int success;

	const char* vertexShaderSrc = NULL;
	const char* fragmentShaderSrc = NULL;
	const char* geometryShaderSrc = NULL;

	for(const auto& shader : shaderIDArr)
	{
		std::string currExtension = std::filesystem::path(shader).extension();
	
		if(_shaderMap.count(shader)==0)
			throw std::runtime_error("shader file doesnt exist");
	
		if(currExtension==".fragment")
		{
			fragmentShaderSrc = _shaderMap[shader].c_str();
		} else if(currExtension==".vertex")
		{
			vertexShaderSrc = _shaderMap[shader].c_str();
		} else if(currExtension==".geometry")
		{
			geometryShaderSrc = _shaderMap[shader].c_str();
		} else
		{
			throw std::runtime_error("shader file isnt a .fragment, .vertex or .geometry");
		}
	}

	unsigned vertexShaderID;
	unsigned fragmentShaderID;
	unsigned geometryShaderID;

	_shaderProgramMap[programID] = glCreateProgram();

	if(vertexShaderSrc!=NULL)
	{
		vertexShaderID = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertexShaderID, 1, &vertexShaderSrc, NULL);
		glCompileShader(vertexShaderID);

		glGetShaderiv(vertexShaderID, GL_COMPILE_STATUS, &success);
		if(!success) outputError(vertexShaderID);

		glAttachShader(_shaderProgramMap[programID], vertexShaderID);
	}
	
	if(fragmentShaderSrc!=NULL)
	{
		fragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragmentShaderID, 1, &fragmentShaderSrc, NULL);
		glCompileShader(fragmentShaderID);

		glGetShaderiv(fragmentShaderID, GL_COMPILE_STATUS, &success);
		if(!success) outputError(fragmentShaderID);

		glAttachShader(_shaderProgramMap[programID], fragmentShaderID);
	}

	if(geometryShaderSrc!=NULL)
	{
		geometryShaderID = glCreateShader(GL_GEOMETRY_SHADER);
		glShaderSource(geometryShaderID, 1, &fragmentShaderSrc, NULL);
		glCompileShader(geometryShaderID);

		glGetShaderiv(geometryShaderID, GL_COMPILE_STATUS, &success);
		if(!success) outputError(geometryShaderID);

		glAttachShader(_shaderProgramMap[programID], geometryShaderID);
	}

	glLinkProgram(_shaderProgramMap[programID]);

	glGetProgramiv(_shaderProgramMap[programID], GL_LINK_STATUS, &success);
	if(!success) outputError(_shaderProgramMap[programID], true);

	if(vertexShaderSrc!=NULL) glDeleteShader(vertexShaderID);
	if(fragmentShaderSrc!=NULL) glDeleteShader(fragmentShaderID);
	if(geometryShaderSrc!=NULL) glDeleteShader(geometryShaderID);
}

void YandereInitializer::switchShaderProgram(unsigned programID)
{
	assert(_shaderProgramMap.count(programID)!=0);
	unsigned _shaderProgramID = _shaderProgramMap[programID];

	glUseProgram(_shaderProgramID);

	_shaderViewLocation = glGetUniformLocation(_shaderProgramID, "viewMat");
	_shaderProjectionLocation = glGetUniformLocation(_shaderProgramID, "projectionMat");

	//this is horribly inefficient but i really dont want to figure out how to design this better
}

YandereText YandereInitializer::createText(std::string text, std::string fontName, int size, float x, float y)
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
		
		letters[c].width /= size;
		letters[c].height /= size;
		letters[c].originX /= size;
		letters[c].originY /= size/2.0f;
		letters[c].hDist /= size;
	}

	std::string textureName = "!f" + fontName;

	YandereImage img;
	img.width = width;
	img.height = height;
	img.bpp = 1;
	img.image = texData;
	
	YandereTexture fontTexture = YandereTexture(img);
	
	_textureMap[textureName] = std::move(fontTexture);
	
	YandereText outText = YandereText(this, letters, textureName);
	outText.setPosition(x, y);
	outText.changeText(text);

	return std::move(outText);
}

void YandereInitializer::assignShader(std::string shaderStr, std::string shaderID)
{
	_shaderMap[shaderID] = shaderStr;
}

void YandereInitializer::outputError(unsigned objectID, bool isProgram)
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

void YandereInitializer::doGlewInit(bool stencil, bool antialiasing, bool culling)
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
		glDebugMessageCallback(YandereInitializer::debugCallback, NULL);
		#endif
	}

	_glewInitialized = true;

	configGLBuffers(stencil, antialiasing, culling);
}

bool YandereInitializer::glewInitialized()
{
	return _glewInitialized;
}

void GLAPIENTRY YandereInitializer::debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
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
	assert(_yanInitializer->glewInitialized());
	yGLSetupCalls(border.enabled);

	matsUpdate();
}

void YandereObject::yGLSetupCalls(bool borders)
{
	if(borders)
	{
		glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	}
	
	
	glGenBuffers(1, &_vertexBufferObject);;
}

void YandereObject::setPosition(YanPosition position)
{
	_transform.position = position;
	
	matsUpdate();
}

void YandereObject::translate(YanPosition positionDelta)
{
	_transform.position.x += positionDelta.x;
	_transform.position.y += positionDelta.y;
	_transform.position.z += positionDelta.z;
		
	matsUpdate();
}

void YandereObject::setScale(YanScale scale)
{
	_transform.scale = scale;
		
	matsUpdate();
}

void YandereObject::setRotation(float rotation)
{
	_transform.rotation = rotation;

	matsUpdate();
}

void YandereObject::rotate(float rotationDelta)
{
	_transform.rotation += rotationDelta;

	matsUpdate();
}

void YandereObject::setRotationAxis(YanPosition axis)
{
	_transform.axis = axis;

	matsUpdate();
}

void YandereObject::setColor(YanColor color)
{
	_color = color;

	_objData.color = _color;
}

void YandereObject::setBorder(YanBorder border)
{
	_border = border;
}

void YandereObject::drawUpdate()
{
	assert(_usedModel!="" && _usedTexture!="");
	_yanInitializer->setDrawModel(_usedModel);
	_yanInitializer->setDrawTexture(_usedTexture);

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

	assert(_yanInitializer->_shaderViewLocation!=-1);
	assert(_yanInitializer->_shaderProjectionLocation!=-1);
	glUniformMatrix4fv(_yanInitializer->_shaderViewLocation, 1, GL_FALSE, glm::value_ptr(*_yanInitializer->_viewMatrix));
	glUniformMatrix4fv(_yanInitializer->_shaderProjectionLocation, 1, GL_FALSE, glm::value_ptr(*_yanInitializer->_projectionMatrix));

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
		_yanInitializer->switchShaderProgram(0);

		glBufferData(GL_ARRAY_BUFFER, bufferSize, &_scaledObjData, GL_STATIC_DRAW);

		glUniformMatrix4fv(_yanInitializer->_shaderViewLocation, 1, GL_FALSE, glm::value_ptr(*_yanInitializer->_viewMatrix));
		glUniformMatrix4fv(_yanInitializer->_shaderProjectionLocation, 1, GL_FALSE, glm::value_ptr(*_yanInitializer->_projectionMatrix));

		glDrawElements(GL_TRIANGLES, _yanInitializer->_currentModelSize, GL_UNSIGNED_INT, 0);

		_yanInitializer->switchShaderProgram(restoreShader);

		glStencilMask(0xFF);
		glStencilFunc(GL_ALWAYS, 1, 0xFF);

		glEnable(GL_DEPTH_TEST);
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void YandereObject::matsUpdate()
{
	_scaledTransform = _transform;
	_scaledTransform.scale.x *= _border.width;
	_scaledTransform.scale.y *= _border.width;
	_scaledTransform.scale.z *= _border.width;

	glm::mat4 tempMat = _yanInitializer->calculateMatrix(_transform);

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
		tempMat = _yanInitializer->calculateMatrix(_scaledTransform);

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
	assert(_yanInitializer->glewInitialized());
	yGLSetupCalls(border.enabled);

	_singleColor=!(colors.size()>1);

	if(transformsInstanced.size()!=0)
	{
		matsUpdate();
	}
}

bool YandereObjects::isEmpty()
{
	return (_transformsSize==0);
}

void YandereObjects::yGLSetupCalls(bool borders)
{
	_instancedData.reserve(_transformsSize);
	_scaledInstancedData.reserve(_transformsSize);
	
	if(borders)
	{
		glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	}
	
	
	glGenBuffers(1, &_instanceVertexBufferObject);;
}

void YandereObjects::setPositions(std::vector<YanPosition> positions)
{
	assert(positions.size()==_transformsSize);
	for(int i = 0; i < _transformsSize; i++)
	{
		_transforms[i].position = positions[i];
	}
	matsUpdate();
}

void YandereObjects::setPosition(YanPosition position)
{
	for(int i = 0; i < _transformsSize; i++)
	{
		_transforms[i].position = position;
	}
	matsUpdate();
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
	matsUpdate();
}

void YandereObjects::translate(YanPosition positionDelta)
{
	for(int i = 0; i < _transformsSize; i++)
	{
		_transforms[i].position.x += positionDelta.x;
		_transforms[i].position.y += positionDelta.y;
		_transforms[i].position.z += positionDelta.z;
	}
	matsUpdate();
}

void YandereObjects::setScales(std::vector<YanScale> scales)
{
	assert(scales.size()==_transformsSize);
	for(int i = 0; i < _transformsSize; i++)
	{
		_transforms[i].scale = scales[i];
	}
	matsUpdate();
}

void YandereObjects::setRotations(std::vector<float> rotations)
{
	assert(rotations.size()==_transformsSize);
	for(int i = 0; i < _transformsSize; i++)
	{
		_transforms[i].rotation = rotations[i];
	}
	matsUpdate();
}

void YandereObjects::rotate(std::vector<float> rotationDeltas)
{
	assert(rotationDeltas.size()==_transformsSize);
	for(int i = 0; i < _transformsSize; i++)
	{
		_transforms[i].rotation += rotationDeltas[i];
	}
	matsUpdate();
}

void YandereObjects::setRotationAxes(std::vector<YanPosition> axes)
{
	assert(axes.size()==_transformsSize);
	for(int i = 0; i < _transformsSize; i++)
	{
		_transforms[i].axis = axes[i];
	}
	matsUpdate();
}

void YandereObjects::setColors(std::vector<YanColor> colors)
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

void YandereObjects::setBorder(YanBorder border)
{
	_border = border;
}

void YandereObjects::drawUpdate()
{
	assert(!isEmpty());

	_yanInitializer->setDrawModel(_usedModel);
	_yanInitializer->setDrawTexture(_usedTexture);
	
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

	assert(_yanInitializer->_shaderViewLocation!=-1);
	assert(_yanInitializer->_shaderProjectionLocation!=-1);
	glUniformMatrix4fv(_yanInitializer->_shaderViewLocation, 1, GL_FALSE, glm::value_ptr(*_yanInitializer->_viewMatrix));
	glUniformMatrix4fv(_yanInitializer->_shaderProjectionLocation, 1, GL_FALSE, glm::value_ptr(*_yanInitializer->_projectionMatrix));

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
		_yanInitializer->switchShaderProgram(0);

		glBufferData(GL_ARRAY_BUFFER, bufferSize * _transformsSize, _scaledInstancedData.data(), GL_STATIC_DRAW);

		glUniformMatrix4fv(_yanInitializer->_shaderViewLocation, 1, GL_FALSE, glm::value_ptr(*_yanInitializer->_viewMatrix));
		glUniformMatrix4fv(_yanInitializer->_shaderProjectionLocation, 1, GL_FALSE, glm::value_ptr(*_yanInitializer->_projectionMatrix));

		glDrawElementsInstanced(GL_TRIANGLES, _yanInitializer->_currentModelSize, GL_UNSIGNED_INT, 0, _transformsSize);

		_yanInitializer->switchShaderProgram(restoreShader);

		glStencilMask(0xFF);
		glStencilFunc(GL_ALWAYS, 1, 0xFF);

		glEnable(GL_DEPTH_TEST);
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void YandereObjects::matsUpdate()
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
		glm::mat4 tempMat = _yanInitializer->calculateMatrix(_transforms[i]);

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
			tempMat = _yanInitializer->calculateMatrix(_scaledTransforms[i]);

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
	calculateVariables();
}

void YandereLines::setPositions(std::vector<std::array<YanPosition, 2>> points)
{
	_points = points;

	calculateVariables();
}

void YandereLines::setRotations(std::vector<float> rotations)
{
	for(size_t i = 0; i < _transformsSize; i++)
	{
		_transforms[i].rotation = rotations[i];
	}

	calculateVariables();
}

void YandereLines::setWidths(std::vector<float> widths)
{
	_widths = widths;

	for(size_t i = 0; i < _transformsSize; i++)
	{
		_transforms[i].scale.y = _widths[i];
	}

	matsUpdate();
}

void YandereLines::calculateVariables()
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

	matsUpdate();
}

YandereText::YandereText()
{
}

YandereText::YandereText(YandereInitializer* yanInitializer, std::vector<LetterData> letters, std::string textureName) : 
_yanInitializer(yanInitializer), _letters(letters), _textureName(textureName)
{
}

void YandereText::drawUpdate()
{
	for(auto& obj : _letterObjs)
	{
		obj.drawUpdate();
	}
}

void YandereText::changeText(std::string newText)
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
		
		YanTransforms letterTransform = YanTransforms{letterPos,
		letter.width,
		letter.height, 1};
	

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

void YandereText::setPosition(float x, float y)
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

float YandereText::textWidth()
{
	return _textWidth;
}

float YandereText::textHeight()
{
	return _textHeight;
}
