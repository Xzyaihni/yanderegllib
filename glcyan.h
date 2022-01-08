#ifndef GLCYAN_H
#define GLCYAN_H

#include <GL/glew.h>
#include <GL/glu.h>
#include <GL/gl.h>

#include <glm/glm.hpp>

#include <vector>
#include <array>
#include <unordered_map>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "yanconv.h"

struct YanPosition
{
	float x = 0;
	float y = 0;
	float z = 0;
};

struct YanScale
{
	float x = 1;
	float y = 1;
	float z = 1;
};

struct YanTransforms
{
	YanPosition position = {};

	YanScale scale = {};

	float rotation = 0;

	YanPosition axis = {0, 0, 1};
};

struct YanColor
{
	float r = 1;
	float g = 1;
	float b = 1;
	float a = 1;
};

struct YanBorder
{
	bool enabled = false;
	YanColor color = {};
	float width = 1.1f;
};

struct LetterData
{
	float width;
	float height;
	float originX;
	float originY;
	float hDist;
};

class YandereCamera
{
private:
	struct Plane
	{
		float a;
		float b;
		float c;
		float d;
	};
		
	enum Side
	{
		left = 0,
		right,
		down,
		up,
		back,
		forward
	};

public:
	YandereCamera();
	YandereCamera(YanPosition position, YanPosition lookPosition);

	void createProjection(std::array<float, 4> orthoBox, std::array<float, 2> planes);
	void createProjection(float fov, float aspect, std::array<float, 2> planes);

	void setPosition(YanPosition);
	void setPositionX(float);
	void setPositionY(float);
	void setPositionZ(float);

	void setRotation(float, float);
	void setDirection(std::array<float, 3>);
	void lookAt(std::array<float, 3>);

	void translatePosition(YanPosition);

	YanPosition position();

	//returns -1 if the point is in the negative direction, 1 if positive, 0 if inside the camera planes
	int xPointSide(YanPosition point);
	int yPointSide(YanPosition point);
	int zPointSide(YanPosition point);
	
	//i could check for cuboid but im too lazy taking in 8 coordinate points
	bool cubeInFrustum(YanPosition middle, float size);

	glm::mat4* projViewMatrixPtr();
	glm::mat4* viewMatrixPtr();
	glm::mat4* projectionMatrixPtr();

	float clipFar;
	float clipClose;
	
private:
	void calculateView();
	
	void calculatePlanes();

	glm::vec3 _cameraPosition;
	glm::vec3 _cameraDirection;
	glm::vec3 _cameraUp;
	glm::vec3 _cameraRight;
	glm::vec3 _cameraForward;

	glm::mat4 _viewMatrix;
	glm::mat4 _projectionMatrix;
	
	glm::mat4 _projViewMatrix;
	
	std::array<Plane, 6> _planesArr;
	
	bool _projectionMatrixExists = false;
	bool _viewMatrixExists = false;
};

class YandereModel
{
public:
    YandereModel();
    YandereModel(std::string);

    void setCurrent(unsigned, unsigned, unsigned);

    std::vector<float>* getVerticesPtr();
    std::vector<unsigned>* getIndicesPtr();
    
    std::vector<float> vertices;
	std::vector<unsigned> indices;
	
private:
    bool parseModel(std::string, std::string);
};

class YandereTexture
{
public:
    YandereTexture();
    YandereTexture(std::string stringImagePath);
    YandereTexture(YandereImage image);

    void setCurrent(unsigned);
    
    int width();
    int height();
    
private:
    bool parseImage(std::string, std::string);

	unsigned setTextureType(uint8_t bpp);

    unsigned _textureType;

    YandereImage _image;

    bool _empty = true;
};

class YandereText;

class YandereInitializer
{
public:
	YandereInitializer();
	~YandereInitializer();

	void doGlewInit(bool stencil = true, bool antialiasing = true, bool culling = true);
	bool glewInitialized();

	void setDrawModel(std::string modelName);
	void setDrawTexture(std::string textureName);
	
	void setDrawCamera(YandereCamera* camera);

	void loadShadersFrom(std::string shadersFolder);
	void loadModelsFrom(std::string modelsFolder);
	void loadTexturesFrom(std::string texturesFolder);

	void loadFont(std::string fPath);

	void createShaderProgram(unsigned programID, std::vector<std::string> shaderIDArr);
	void switchShaderProgram(unsigned programID);

	YandereText createText(std::string text, std::string fontName, int size, float x, float y);

	glm::mat4 calculateMatrix(YanTransforms transforms);
	
	//probably not a good idea to use these directly
	std::unordered_map<std::string, YandereModel> _modelMap;
	std::unordered_map<std::string, YandereTexture> _textureMap;

private:
	void outputError(unsigned, bool = false);

	void configGLBuffers(bool s = true, bool a = true, bool c = true);

	void loadDefaultModels();

	void assignShader(std::string shaderStr, std::string shaderID);

	void updateMatrixPointers(YandereCamera* camera);

	static void debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam);

	bool _glewInitialized = false;

	YandereCamera* _mainCamera = nullptr;

	unsigned _storedShaderID = -1;

	glm::mat4* _viewMatrix = nullptr;
	glm::mat4* _projectionMatrix = nullptr;

    std::string _currentModelUsed;
    size_t _currentModelSize;

    std::string _currentTextureUsed;

	std::unordered_map<std::string, std::string> _shaderMap;
	std::unordered_map<unsigned, unsigned> _shaderProgramMap;
	
	std::unordered_map<std::string, FT_Face> _fontsMap;
	
	FT_Library _ftLib;
	bool _fontInit = false;

	unsigned _elementObjectBufferID = -1;
	unsigned _vertexArrayObjectID = -1;
	unsigned _vertexBufferObjectID = -1;

	unsigned _textureBufferObjectID = -1;

	unsigned _shaderViewLocation = -1;
	unsigned _shaderProjectionLocation = -1;

	friend class YandereObject;
	friend class YandereObjects;
};

class YandereObject
{
private:
	struct ObjectData
    {
        YanColor color;
        float matrix[16];
    };

public:
	YandereObject();

	YandereObject(YandereInitializer*, std::string usedModel, std::string usedTexture, YanTransforms transform, YanColor color = {}, YanBorder border = {});

	void setPosition(YanPosition position);
	void translate(YanPosition positionDelta);

	void setScale(YanScale scale);
	void setRotation(float rotation);
	void rotate(float rotation);

	void setRotationAxis(YanPosition axis);

	void setColor(YanColor color);
	void setBorder(YanBorder border);

	void drawUpdate();
	
protected:
	void matsUpdate();

	void yGLSetupCalls(bool);

	YanTransforms _transform;
	YanTransforms _scaledTransform;
	YanColor _color;
	YanBorder _border;

	unsigned _vertexBufferObject = 0;

	YandereInitializer* _yanInitializer;
	
private:
    std::string _usedModel;
    std::string _usedTexture;

	ObjectData _objData;
	ObjectData _scaledObjData;
};

class YandereObjects
{
private:
	struct ObjectData
    {
        YanColor color;
        float matrix[16];
    };
    
public:
	YandereObjects();

	YandereObjects(YandereInitializer*, std::string usedModel, std::string usedTexture, std::vector<YanTransforms> transforms, size_t s, std::vector<YanColor> colors = {{}}, YanBorder border = {});

	bool isEmpty();

	void setPositions(std::vector<YanPosition> positions);
	void setPosition(YanPosition position);
	void translate(std::vector<YanPosition> positionDelta);
	void translate(YanPosition positionDelta);

	void setScales(std::vector<YanScale> scales);
	void setRotations(std::vector<float> rotations);
	void rotate(std::vector<float> rotations);

	void setRotationAxes(std::vector<YanPosition> axes);

	void setColors(std::vector<YanColor> colors);
	void setBorder(YanBorder border);

	void drawUpdate();
	
protected:
	void matsUpdate();

	void yGLSetupCalls(bool);

	std::vector<YanTransforms> _transforms;
	std::vector<YanTransforms> _scaledTransforms;
	std::vector<YanColor> _colors;
	YanBorder _border;

	unsigned _instanceVertexBufferObject = 0;

	YandereInitializer* _yanInitializer;

	size_t _transformsSize;
	bool _singleColor;
	
private:
    std::string _usedModel;
    std::string _usedTexture;

	std::vector<ObjectData> _instancedData;
	std::vector<ObjectData> _scaledInstancedData;
};

class YandereLines : public YandereObjects
{
public:
	YandereLines();

	YandereLines(YandereInitializer*, std::vector<std::array<YanPosition, 2>> p, size_t s, std::vector<float> w, std::vector<YanColor> c = {}, YanBorder b = {});

	void setPositions(std::vector<std::array<YanPosition, 2>>);
	void setWidths(std::vector<float>);
	void setRotations(std::vector<float>);
	
private:
	void calculateVariables();

	std::vector<std::array<YanPosition, 2>> _points;
	std::vector<float> _widths;
};

class YandereText
{
public:
	YandereText();
	YandereText(YandereInitializer* yanInitializer, std::vector<LetterData> letters, std::string textureName);
	
	void drawUpdate();
	
	void changeText(std::string newText);
	
	void setPosition(float x, float y);
	std::array<float, 2> getPosition();
	
	float textWidth();
	float textHeight();
	
private:
	YandereInitializer* _yanInitializer;

	std::vector<YandereObject> _letterObjs;

	std::vector<LetterData> _letters;

	std::string _textureName;
	
	float x = 0;
	float y = 0;
	
	float _textWidth = 0;
	float _textHeight = 0;
};

#endif

//surprisingly this has nothing to do with yanderes
