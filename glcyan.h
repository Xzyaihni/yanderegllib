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

struct YVec2
{
	float x = 0;
	float y = 0;
};

struct YVec3
{
	float x = 0;
	float y = 0;
	float z = 0;
};

struct YVec4
{
	float x = 0;
	float y = 0;
	float z = 0;
	float w = 0;
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
	YandereCamera() {};
	YandereCamera(YanPosition position, YanPosition lookPosition);

	void create_projection(std::array<float, 4> orthoBox, std::array<float, 2> planes);
	void create_projection(float fov, float aspect, std::array<float, 2> planes);

	void set_position(YanPosition);
	void set_position_x(float);
	void set_position_y(float);
	void set_position_z(float);

	void set_rotation(float, float);
	void set_direction(std::array<float, 3>);
	void look_at(std::array<float, 3>);

	void translate_position(YanPosition);

	YanPosition position();

	//returns -1 if the point is in the negative direction, 1 if positive, 0 if inside the camera planes
	int x_point_side(YanPosition point);
	int y_point_side(YanPosition point);
	int z_point_side(YanPosition point);
	
	//i could check for cuboid but im too lazy taking in 8 coordinate points
	bool cube_in_frustum(YanPosition middle, float size);

	glm::mat4* proj_view_matrix_ptr();
	glm::mat4* view_matrix_ptr();
	glm::mat4* projection_matrix_ptr();

	float clipFar;
	float clipClose;
	
private:
	void calculate_view();
	
	void calculate_planes();

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
    YandereModel() {};
    YandereModel(std::string);

    void set_current(unsigned, unsigned, unsigned);

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
    YandereTexture() {};
    YandereTexture(std::string stringImagePath);
    YandereTexture(YandereImage image);

    void set_current(unsigned);
    
    int width();
    int height();
    
private:
    bool parse_image(std::string, std::string);

	unsigned set_texture_type(uint8_t bpp);

    unsigned _textureType;

    YandereImage _image;

    bool _empty = true;
};

class YandereShader
{
public:
	YandereShader() {};
	YandereShader(std::string shaderText);
	
	std::string& text();

private:
	std::string _shaderText;
};

class YandereShaderProgram
{
private:
	struct Prop
	{
		int length;
		unsigned location;
	};

public:
	YandereShaderProgram() {};
	YandereShaderProgram(unsigned programID);
	
	unsigned add_num(std::string name);
	unsigned add_vec2(std::string name);
	unsigned add_vec3(std::string name);
	unsigned add_vec4(std::string name);

	template<typename T>
	void set_prop(unsigned propID, T val)
	{
		if constexpr(std::is_same<T, YVec4>::value)
		{
			_shaderPropsVec4[propID/4] = val;
		} else if constexpr(std::is_same<T, YVec3>::value)
		{
			_shaderPropsVec3[propID/4] = val;
		} else if constexpr(std::is_same<T, YVec2>::value)
		{
			_shaderPropsVec2[propID/4] = val;
		} else
		{
			_shaderProps[propID/4] = val;
		}
	}

	void apply_uniforms();

	unsigned program() const;

	unsigned view_mat() const;
	unsigned projection_mat() const;

private:
	template<typename T, typename V>
	unsigned add_any(T& type, V vType, int index, std::string name);

	void matrix_setup();

	unsigned _programID;
	
	unsigned _viewMat;
	unsigned _projectionMat;

	std::vector<Prop> _propsVec;
	std::vector<float> _shaderProps;
	std::vector<YVec2> _shaderPropsVec2;
	std::vector<YVec3> _shaderPropsVec3;
	std::vector<YVec4> _shaderPropsVec4;
};

class YandereText;

class YandereInitializer
{
public:
	YandereInitializer();
	~YandereInitializer();

	void do_glew_init(bool stencil = true, bool antialiasing = true, bool culling = true);
	bool glew_initialized();

	void set_draw_model(std::string modelName);
	void set_draw_texture(std::string textureName);
	void set_shader_program(unsigned programID);
	
	void set_draw_camera(YandereCamera* camera);

	void load_shaders_from(std::string shadersFolder);
	void load_models_from(std::string modelsFolder);
	void load_textures_from(std::string texturesFolder);

	void load_font(std::string fPath);

	unsigned create_shader_program(std::vector<std::string> shaderIDArr);
	YandereShaderProgram* shader_program_ptr(unsigned programID);

	YandereText create_text(std::string text, std::string fontName, int size, float x, float y);

	glm::mat4 calculate_matrix(YanTransforms transforms);
	
	//probably not a good idea to use these directly
	std::unordered_map<std::string, YandereModel> _modelMap;
	std::unordered_map<std::string, YandereTexture> _textureMap;

private:
	void output_error(unsigned, bool = false);

	void config_GL_buffers(bool s = true, bool a = true, bool c = true);

	void load_default_models();

	void update_matrix_pointers(YandereCamera* camera);
	void apply_uniforms();

	static void debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam);

	bool _glewInitialized = false;

	YandereCamera* _mainCamera = nullptr;

	unsigned _lastProgramID = 0;
	unsigned _storedShaderID = -1;

	glm::mat4* _viewMatrix = nullptr;
	glm::mat4* _projectionMatrix = nullptr;

    std::string _currentModelUsed;
    size_t _currentModelSize;

    std::string _currentTextureUsed;
    
    std::unordered_map<std::string, YandereShader> _shaderMap;
	std::unordered_map<unsigned, YandereShaderProgram> _shaderProgramMap;
	
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

	void set_position(YanPosition position);
	void translate(YanPosition positionDelta);

	void set_scale(YanScale scale);
	void set_rotation(float rotation);
	void rotate(float rotation);

	void set_rotationAxis(YanPosition axis);

	void set_color(YanColor color);
	void set_border(YanBorder border);

	void draw_update();
	
protected:
	void mats_update();

	void yGL_setup(bool);

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

	bool empty();

	void set_positions(std::vector<YanPosition> positions);
	void set_position(YanPosition position);
	void translate(std::vector<YanPosition> positionDelta);
	void translate(YanPosition positionDelta);

	void set_scales(std::vector<YanScale> scales);
	void set_rotations(std::vector<float> rotations);
	void rotate(std::vector<float> rotations);

	void set_rotationAxes(std::vector<YanPosition> axes);

	void set_colors(std::vector<YanColor> colors);
	void set_border(YanBorder border);

	void draw_update();
	
protected:
	void mats_update();

	void yGL_setup(bool);

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

	void set_positions(std::vector<std::array<YanPosition, 2>>);
	void set_widths(std::vector<float>);
	void set_rotations(std::vector<float>);
	
private:
	void calculate_variables();

	std::vector<std::array<YanPosition, 2>> _points;
	std::vector<float> _widths;
};

class YandereText
{
public:
	YandereText();
	YandereText(YandereInitializer* yanInitializer, int size, std::vector<LetterData> letters, std::string textureName);
	
	void draw_update();
	
	void change_text(std::string newText);
	
	void set_position(float x, float y);
	std::array<float, 2> getPosition();
	
	float text_width();
	float text_height();
	
private:
	YandereInitializer* _yanInitializer;

	std::vector<YandereObject> _letterObjs;

	std::vector<LetterData> _letters;

	std::string _textureName;
	
	float x = 0;
	float y = 0;
	
	int _size = 0;
	
	float _textWidth = 0;
	float _textHeight = 0;
};

#endif

//surprisingly this has nothing to do with yanderes
