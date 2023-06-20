/**********************************************************************************************
*
*   rlgl 11.0 - A DX 11.0 abstraction layer with an immediate-mode style API
*
*   DESCRIPTION:
*       An abstraction layer for DX 11.0
*       that provides a pseudo-OpenGL 1.1 immediate-mode style API (rlVertex, rlTranslate, rlRotate...)
*
*   ADDITIONAL NOTES:
*       When choosing an OpenGL backend different than OpenGL 1.1, some internal buffer are
*       initialized on rlglInit() to accumulate vertex data.
*
*       When an internal state change is required all the stored vertex data is renderer in batch,
*       additionally, rlDrawRenderBatchActive() could be called to force flushing of the batch.
*
*       Some resources are also loaded for convenience, here the complete list:
*          - Default batch (RLGH.defaultBatch): RenderBatch system to accumulate vertex data
*          - Default texture (RLGH.defaultTexture): 1x1 white pixel R8G8B8A8
*          - Default shader (RLGH.State.defaultShaderId, RLGH.State.defaultShaderLocs)
*
*       Internal buffer (and resources) must be manually unloaded calling rlglClose().
*
*   CONFIGURATION:
*           If DX 11 feature level is required by any other module, use rlGetVersion() to check it
*
*       #define RLGL_IMPLEMENTATION
*           Generates the implementation of the library into the included file.
*           If not defined, the library is in header only mode and can be included in other headers
*           or source files without problems. But only ONE file should hold the implementation.
*
*       #define RLGL_SHOW_GL_DETAILS_INFO
*           Show OpenGL extensions and capabilities detailed logs on init
*
*       #define RLGL_ENABLE_OPENGL_DEBUG_CONTEXT
*           Enable debug context (only available on OpenGL 4.3)
*
*       rlgl capabilities could be customized just defining some internal
*       values before library inclusion (default values listed):
*
*       #define RL_DEFAULT_BATCH_BUFFER_ELEMENTS   8192    // Default internal render batch elements limits
*       #define RL_DEFAULT_BATCH_BUFFERS              1    // Default number of batch buffers (multi-buffering)
*       #define RL_DEFAULT_BATCH_DRAWCALLS          256    // Default number of batch draw calls (by state changes: mode, texture)
*       #define RL_DEFAULT_BATCH_MAX_TEXTURE_UNITS    4    // Maximum number of textures units that can be activated on batch drawing (SetShaderValueTexture())
*
*       #define RL_MAX_MATRIX_STACK_SIZE             32    // Maximum size of internal Matrix stack
*       #define RL_MAX_SHADER_LOCATIONS              32    // Maximum number of shader locations supported
*       #define RL_CULL_DISTANCE_NEAR              0.01    // Default projection matrix near cull distance
*       #define RL_CULL_DISTANCE_FAR             1000.0    // Default projection matrix far cull distance
*
*       When loading a shader, the following vertex attribute and uniform
*       location names are tried to be set automatically:
*
*       #define RL_DEFAULT_SHADER_ATTRIB_NAME_POSITION     "vertexPosition"    // Bound by default to shader location: 0
*       #define RL_DEFAULT_SHADER_ATTRIB_NAME_TEXCOORD     "vertexTexCoord"    // Bound by default to shader location: 1
*       #define RL_DEFAULT_SHADER_ATTRIB_NAME_NORMAL       "vertexNormal"      // Bound by default to shader location: 2
*       #define RL_DEFAULT_SHADER_ATTRIB_NAME_COLOR        "vertexColor"       // Bound by default to shader location: 3
*       #define RL_DEFAULT_SHADER_ATTRIB_NAME_TANGENT      "vertexTangent"     // Bound by default to shader location: 4
*       #define RL_DEFAULT_SHADER_UNIFORM_NAME_MVP         "mvp"               // model-view-projection matrix
*       #define RL_DEFAULT_SHADER_UNIFORM_NAME_VIEW        "matView"           // view matrix
*       #define RL_DEFAULT_SHADER_UNIFORM_NAME_PROJECTION  "matProjection"     // projection matrix
*       #define RL_DEFAULT_SHADER_UNIFORM_NAME_MODEL       "matModel"          // model matrix
*       #define RL_DEFAULT_SHADER_UNIFORM_NAME_NORMAL      "matNormal"         // normal matrix (transpose(inverse(matModelView))
*       #define RL_DEFAULT_SHADER_UNIFORM_NAME_COLOR       "colDiffuse"        // color diffuse (base tint color, multiplied by texture color)
*       #define RL_DEFAULT_SHADER_SAMPLER2D_NAME_TEXTURE0  "texture0"          // texture0 (texture slot active 0)
*       #define RL_DEFAULT_SHADER_SAMPLER2D_NAME_TEXTURE1  "texture1"          // texture1 (texture slot active 1)
*       #define RL_DEFAULT_SHADER_SAMPLER2D_NAME_TEXTURE2  "texture2"          // texture2 (texture slot active 2)
*
*   DEPENDENCIES:
*      - OpenGL libraries (depending on platform and OpenGL version selected)
*      - GLAD OpenGL extensions loading library (only for OpenGL 3.3 Core, 4.3 Core)
*
*
*   LICENSE: zlib/libpng
*
*   Copyright (c) 2014-2023 Ramon Santamaria (@raysan5)
*
*   This software is provided "as-is", without any express or implied warranty. In no event
*   will the authors be held liable for any damages arising from the use of this software.
*
*   Permission is granted to anyone to use this software for any purpose, including commercial
*   applications, and to alter it and redistribute it freely, subject to the following restrictions:
*
*     1. The origin of this software must not be misrepresented; you must not claim that you
*     wrote the original software. If you use this software in a product, an acknowledgment
*     in the product documentation would be appreciated but is not required.
*
*     2. Altered source versions must be plainly marked as such, and must not be misrepresented
*     as being the original software.
*
*     3. This notice may not be removed or altered from any source distribution.
*
**********************************************************************************************/

#ifndef RLGL_H
#define RLGL_H

#define RLGL_VERSION  "11.0"

// Function specifiers in case library is build/used as a shared library (Windows)
// NOTE: Microsoft specifiers to tell compiler that symbols are imported/exported from a .dll
#if defined(_WIN32)
#if defined(BUILD_LIBTYPE_SHARED)
#define RLAPI __declspec(dllexport)     // We are building the library as a Win32 shared library (.dll)
#elif defined(USE_LIBTYPE_SHARED)
#define RLAPI __declspec(dllimport)     // We are using the library as a Win32 shared library (.dll)
#endif
#endif

// Function specifiers definition
#ifndef RLAPI
#define RLAPI       // Functions defined as 'extern' by default (implicit specifiers)
#endif

// Support TRACELOG macros
#ifndef TRACELOG
#define TRACELOG(level, ...) (void)0
#define TRACELOGD(...) (void)0
#endif

// Allow custom memory allocators
#ifndef RL_MALLOC
#define RL_MALLOC(sz)     malloc(sz)
#endif
#ifndef RL_CALLOC
#define RL_CALLOC(n,sz)   calloc(n,sz)
#endif
#ifndef RL_REALLOC
#define RL_REALLOC(n,sz)  realloc(n,sz)
#endif
#ifndef RL_FREE
#define RL_FREE(p)        free(p)
#endif

//----------------------------------------------------------------------------------
// Defines and Macros
//----------------------------------------------------------------------------------

// Default internal render batch elements limits
#ifndef RL_DEFAULT_BATCH_BUFFER_ELEMENTS
	// This is the maximum amount of elements (quads) per batch
	// NOTE: Be careful with text, every letter maps to a quad
#define RL_DEFAULT_BATCH_BUFFER_ELEMENTS  8192
#endif
#ifndef RL_DEFAULT_BATCH_BUFFERS
#define RL_DEFAULT_BATCH_BUFFERS                 1      // Default number of batch buffers (multi-buffering)
#endif
#ifndef RL_DEFAULT_BATCH_DRAWCALLS
#define RL_DEFAULT_BATCH_DRAWCALLS             256      // Default number of batch draw calls (by state changes: mode, texture)
#endif
#ifndef RL_DEFAULT_BATCH_MAX_TEXTURE_UNITS
#define RL_DEFAULT_BATCH_MAX_TEXTURE_UNITS       4      // Maximum number of textures units that can be activated on batch drawing (SetShaderValueTexture())
#endif

// Internal Matrix stack
#ifndef RL_MAX_MATRIX_STACK_SIZE
#define RL_MAX_MATRIX_STACK_SIZE                32      // Maximum size of Matrix stack
#endif

// Shader limits
#ifndef RL_MAX_SHADER_LOCATIONS
#define RL_MAX_SHADER_LOCATIONS                 32      // Maximum number of shader locations supported
#endif

// Projection matrix culling
#ifndef RL_CULL_DISTANCE_NEAR
#define RL_CULL_DISTANCE_NEAR                 0.01      // Default near cull distance
#endif
#ifndef RL_CULL_DISTANCE_FAR
#define RL_CULL_DISTANCE_FAR                1000.0      // Default far cull distance
#endif

// Texture parameters (equivalent to OpenGL defines)
#define RL_TEXTURE_WRAP_S                       0x2802      // GL_TEXTURE_WRAP_S
#define RL_TEXTURE_WRAP_T                       0x2803      // GL_TEXTURE_WRAP_T
#define RL_TEXTURE_MAG_FILTER                   0x2800      // GL_TEXTURE_MAG_FILTER
#define RL_TEXTURE_MIN_FILTER                   0x2801      // GL_TEXTURE_MIN_FILTER

#define RL_TEXTURE_FILTER_NEAREST               0x2600      // GL_NEAREST
#define RL_TEXTURE_FILTER_LINEAR                0x2601      // GL_LINEAR
#define RL_TEXTURE_FILTER_MIP_NEAREST           0x2700      // GL_NEAREST_MIPMAP_NEAREST
#define RL_TEXTURE_FILTER_NEAREST_MIP_LINEAR    0x2702      // GL_NEAREST_MIPMAP_LINEAR
#define RL_TEXTURE_FILTER_LINEAR_MIP_NEAREST    0x2701      // GL_LINEAR_MIPMAP_NEAREST
#define RL_TEXTURE_FILTER_MIP_LINEAR            0x2703      // GL_LINEAR_MIPMAP_LINEAR
#define RL_TEXTURE_FILTER_ANISOTROPIC           0x3000      // Anisotropic filter (custom identifier)
#define RL_TEXTURE_MIPMAP_BIAS_RATIO            0x4000      // Texture mipmap bias, percentage ratio (custom identifier)

#define RL_TEXTURE_WRAP_REPEAT                  0x2901      // GL_REPEAT
#define RL_TEXTURE_WRAP_CLAMP                   0x812F      // GL_CLAMP_TO_EDGE
#define RL_TEXTURE_WRAP_MIRROR_REPEAT           0x8370      // GL_MIRRORED_REPEAT
#define RL_TEXTURE_WRAP_MIRROR_CLAMP            0x8742      // GL_MIRROR_CLAMP_EXT

// Matrix modes (equivalent to OpenGL)
#define RL_MODELVIEW                            0x1700      // GL_MODELVIEW
#define RL_PROJECTION                           0x1701      // GL_PROJECTION
#define RL_TEXTURE                              0x1702      // GL_TEXTURE

// Primitive assembly draw modes
#define RL_LINES                                0x0001      // GL_LINES
#define RL_TRIANGLES                            0x0004      // GL_TRIANGLES
#define RL_QUADS                                0x0007      // GL_QUADS

// GL equivalent data types
#define RL_UNSIGNED_BYTE                        0x1401      // GL_UNSIGNED_BYTE
#define RL_FLOAT                                0x1406      // GL_FLOAT

// GL buffer usage hint
#define RL_STREAM_DRAW                          0x88E0      // GL_STREAM_DRAW
#define RL_STREAM_READ                          0x88E1      // GL_STREAM_READ
#define RL_STREAM_COPY                          0x88E2      // GL_STREAM_COPY
#define RL_STATIC_DRAW                          0x88E4      // GL_STATIC_DRAW
#define RL_STATIC_READ                          0x88E5      // GL_STATIC_READ
#define RL_STATIC_COPY                          0x88E6      // GL_STATIC_COPY
#define RL_DYNAMIC_DRAW                         0x88E8      // GL_DYNAMIC_DRAW
#define RL_DYNAMIC_READ                         0x88E9      // GL_DYNAMIC_READ
#define RL_DYNAMIC_COPY                         0x88EA      // GL_DYNAMIC_COPY

// GL Shader type
#define RL_FRAGMENT_SHADER                      0x8B30      // GL_FRAGMENT_SHADER
#define RL_VERTEX_SHADER                        0x8B31      // GL_VERTEX_SHADER
#define RL_COMPUTE_SHADER                       0x91B9      // GL_COMPUTE_SHADER

// GL blending factors
#define RL_ZERO                                 0           // GL_ZERO
#define RL_ONE                                  1           // GL_ONE
#define RL_SRC_COLOR                            0x0300      // GL_SRC_COLOR
#define RL_ONE_MINUS_SRC_COLOR                  0x0301      // GL_ONE_MINUS_SRC_COLOR
#define RL_SRC_ALPHA                            0x0302      // GL_SRC_ALPHA
#define RL_ONE_MINUS_SRC_ALPHA                  0x0303      // GL_ONE_MINUS_SRC_ALPHA
#define RL_DST_ALPHA                            0x0304      // GL_DST_ALPHA
#define RL_ONE_MINUS_DST_ALPHA                  0x0305      // GL_ONE_MINUS_DST_ALPHA
#define RL_DST_COLOR                            0x0306      // GL_DST_COLOR
#define RL_ONE_MINUS_DST_COLOR                  0x0307      // GL_ONE_MINUS_DST_COLOR
#define RL_SRC_ALPHA_SATURATE                   0x0308      // GL_SRC_ALPHA_SATURATE
#define RL_CONSTANT_COLOR                       0x8001      // GL_CONSTANT_COLOR
#define RL_ONE_MINUS_CONSTANT_COLOR             0x8002      // GL_ONE_MINUS_CONSTANT_COLOR
#define RL_CONSTANT_ALPHA                       0x8003      // GL_CONSTANT_ALPHA
#define RL_ONE_MINUS_CONSTANT_ALPHA             0x8004      // GL_ONE_MINUS_CONSTANT_ALPHA

// GL blending functions/equations
#define RL_FUNC_ADD                             0x8006      // GL_FUNC_ADD
#define RL_MIN                                  0x8007      // GL_MIN
#define RL_MAX                                  0x8008      // GL_MAX
#define RL_FUNC_SUBTRACT                        0x800A      // GL_FUNC_SUBTRACT
#define RL_FUNC_REVERSE_SUBTRACT                0x800B      // GL_FUNC_REVERSE_SUBTRACT
#define RL_BLEND_EQUATION                       0x8009      // GL_BLEND_EQUATION
#define RL_BLEND_EQUATION_RGB                   0x8009      // GL_BLEND_EQUATION_RGB   // (Same as BLEND_EQUATION)
#define RL_BLEND_EQUATION_ALPHA                 0x883D      // GL_BLEND_EQUATION_ALPHA
#define RL_BLEND_DST_RGB                        0x80C8      // GL_BLEND_DST_RGB
#define RL_BLEND_SRC_RGB                        0x80C9      // GL_BLEND_SRC_RGB
#define RL_BLEND_DST_ALPHA                      0x80CA      // GL_BLEND_DST_ALPHA
#define RL_BLEND_SRC_ALPHA                      0x80CB      // GL_BLEND_SRC_ALPHA
#define RL_BLEND_COLOR                          0x8005      // GL_BLEND_COLOR


//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
#if (defined(__STDC__) && __STDC_VERSION__ >= 199901L) || (defined(_MSC_VER) && _MSC_VER >= 1800)
#include <stdbool.h>
#elif !defined(__cplusplus) && !defined(bool) && !defined(RL_BOOL_TYPE)
	// Boolean type
typedef enum bool{ false = 0, true = !false } bool;
#endif

#if !defined(RL_MATRIX_TYPE)
// Matrix, 4x4 components, column major, OpenGL style, right handed
typedef struct Matrix {
	float m0, m4, m8, m12;      // Matrix first row (4 components)
	float m1, m5, m9, m13;      // Matrix second row (4 components)
	float m2, m6, m10, m14;     // Matrix third row (4 components)
	float m3, m7, m11, m15;     // Matrix fourth row (4 components)
} Matrix;
#define RL_MATRIX_TYPE
#endif

typedef struct rlInputLayoutType {
	int position : 1;
	int texcoord_0 : 1;
	int texcoord_1 : 1;
	int color : 1;
} rlInputLayoutType;

// Dynamic vertex buffers (position + texcoords + colors + indices arrays)
typedef struct rlVertexBuffer {
	int elementCount;           // Number of elements in the buffer (QUADS)

	float *vertices;            // Vertex position (XYZ - 3 components per vertex) (shader-location = 0)
	float *texcoords;           // Vertex texture coordinates (UV - 2 components per vertex) (shader-location = 1)
	unsigned char *colors;      // Vertex colors (RGBA - 4 components per vertex) (shader-location = 3)
	unsigned int *indices;      // Vertex indices (in case vertex data comes indexed) (6 indices per quad)
	unsigned short *indices;    // Vertex indices (in case vertex data comes indexed) (6 indices per quad)

	ID3D11Buffer *vertex_buffer;
	rlInputLayoutType input_layout_type;
} rlVertexBuffer;

typedef struct rlTexture {
	ID3D11Texture2D *tex2D;

	ID3D11ShaderResourceView *srv;
	ID3D11DepthStencilView *dsv;
	ID3D11DepthStencilView *rtv;
} rlTexture;

// Draw call type
// NOTE: Only texture changes register a new draw, other state-change-related elements are not
// used at this moment (vaoId, shaderId, matrices), raylib just forces a batch draw call if any
// of those state-change happens (this is done in core module)
typedef struct rlDrawCall {
	int mode;                   // Drawing mode: LINES, TRIANGLES, QUADS
	int vertexCount;            // Number of vertex of the draw
	int vertexAlignment;        // Number of vertex required for index alignment (LINES, TRIANGLES)
	//unsigned int vaoId;       // Vertex array id to be used on the draw -> Using RLGH.currentBatch->vertexBuffer.vaoId
	//unsigned int shaderId;    // Shader id to be used on the draw -> Using RLGH.currentShaderId
	rlTexture texture;        // Texture id to be used on the draw -> Use to create new draw call if changes

	//Matrix projection;        // Projection matrix for this draw -> Using RLGH.projection by default
	//Matrix modelview;         // Modelview matrix for this draw -> Using RLGH.modelview by default
} rlDrawCall;

// rlRenderBatch type
typedef struct rlRenderBatch {
	int bufferCount;              // Number of vertex buffers (multi-buffering support)
	int currentBuffer;            // Current buffer tracking in case of multi-buffering
	rlVertexBuffer *vertexBuffer; // Dynamic buffer(s) for vertex data

	rlDrawCall *draws;          // Draw calls array, depends on textureId
	int drawCounter;            // Draw calls counter
	float currentDepth;         // Current depth value for next draw
} rlRenderBatch;

// Graphics API
typedef enum {
	RL_DIRECT3D_11_0 = 1,       // Direct3D 11.0
	RL_PSVITA = 1,       // Direct3D 11.0
} rlGraphicsAPI;

// Trace log level
// NOTE: Organized by priority level
typedef enum {
	RL_LOG_ALL = 0,             // Display all logs
	RL_LOG_TRACE,               // Trace logging, intended for internal use only
	RL_LOG_DEBUG,               // Debug logging, used for internal debugging, it should be disabled on release builds
	RL_LOG_INFO,                // Info logging, used for program execution info
	RL_LOG_WARNING,             // Warning logging, used on recoverable failures
	RL_LOG_ERROR,               // Error logging, used on unrecoverable failures
	RL_LOG_FATAL,               // Fatal logging, used to abort program: exit(EXIT_FAILURE)
	RL_LOG_NONE                 // Disable logging
} rlTraceLogLevel;

// Texture pixel formats
// NOTE: Support depends on OpenGL version
typedef enum {
	RL_PIXELFORMAT_UNCOMPRESSED_GRAYSCALE = 1,     // 8 bit per pixel (no alpha)
	RL_PIXELFORMAT_UNCOMPRESSED_GRAY_ALPHA,        // 8*2 bpp (2 channels)
	RL_PIXELFORMAT_UNCOMPRESSED_R5G6B5,            // 16 bpp
	RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8,            // 24 bpp
	RL_PIXELFORMAT_UNCOMPRESSED_R5G5B5A1,          // 16 bpp (1 bit alpha)
	RL_PIXELFORMAT_UNCOMPRESSED_R4G4B4A4,          // 16 bpp (4 bit alpha)
	RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,          // 32 bpp
	RL_PIXELFORMAT_UNCOMPRESSED_R9G9B9E5,          // 32 bpp
	RL_PIXELFORMAT_UNCOMPRESSED_R32,               // 32 bpp (1 channel - float)
	RL_PIXELFORMAT_UNCOMPRESSED_R32G32B32,         // 32*3 bpp (3 channels - float)
	RL_PIXELFORMAT_UNCOMPRESSED_R32G32B32A32,      // 32*4 bpp (4 channels - float)
	RL_PIXELFORMAT_COMPRESSED_DXT1_RGB,            // 4 bpp (no alpha)
	RL_PIXELFORMAT_COMPRESSED_DXT1_RGBA,           // 4 bpp (1 bit alpha)
	RL_PIXELFORMAT_COMPRESSED_DXT3_RGBA,           // 8 bpp
	RL_PIXELFORMAT_COMPRESSED_DXT5_RGBA,           // 8 bpp
	RL_PIXELFORMAT_COMPRESSED_ETC1_RGB,            // 4 bpp
	RL_PIXELFORMAT_COMPRESSED_ETC2_RGB,            // 4 bpp
	RL_PIXELFORMAT_COMPRESSED_ETC2_EAC_RGBA,       // 8 bpp
	RL_PIXELFORMAT_COMPRESSED_PVRT_RGB,            // 4 bpp
	RL_PIXELFORMAT_COMPRESSED_PVRT_RGBA,           // 4 bpp
	RL_PIXELFORMAT_COMPRESSED_ASTC_4x4_RGBA,       // 8 bpp
	RL_PIXELFORMAT_COMPRESSED_ASTC_8x8_RGBA        // 2 bpp
} rlPixelFormat;

// Texture parameters: filter mode
// NOTE 1: Filtering considers mipmaps if available in the texture
// NOTE 2: Filter is accordingly set for minification and magnification
typedef enum {
	RL_TEXTURE_FILTER_POINT = 0,        // No filter, just pixel approximation
	RL_TEXTURE_FILTER_BILINEAR,         // Linear filtering
	RL_TEXTURE_FILTER_TRILINEAR,        // Trilinear filtering (linear with mipmaps)
	RL_TEXTURE_FILTER_ANISOTROPIC_4X,   // Anisotropic filtering 4x
	RL_TEXTURE_FILTER_ANISOTROPIC_8X,   // Anisotropic filtering 8x
	RL_TEXTURE_FILTER_ANISOTROPIC_16X,  // Anisotropic filtering 16x
} rlTextureFilter;

// Color blending modes (pre-defined)
typedef enum {
	RL_BLEND_ALPHA = 0,                 // Blend textures considering alpha (default)
	RL_BLEND_ADDITIVE,                  // Blend textures adding colors
	RL_BLEND_MULTIPLIED,                // Blend textures multiplying colors
	RL_BLEND_ADD_COLORS,                // Blend textures adding colors (alternative)
	RL_BLEND_SUBTRACT_COLORS,           // Blend textures subtracting colors (alternative)
	RL_BLEND_ALPHA_PREMULTIPLY,         // Blend premultiplied textures considering alpha
	RL_BLEND_CUSTOM,                    // Blend textures using custom src/dst factors (use rlSetBlendFactors())
	RL_BLEND_CUSTOM_SEPARATE            // Blend textures using custom src/dst factors (use rlSetBlendFactorsSeparate())
} rlBlendMode;

// Shader location point type
typedef enum {
	RL_SHADER_LOC_VERTEX_POSITION = 0,  // Shader location: vertex attribute: position
	RL_SHADER_LOC_VERTEX_TEXCOORD01,    // Shader location: vertex attribute: texcoord01
	RL_SHADER_LOC_VERTEX_TEXCOORD02,    // Shader location: vertex attribute: texcoord02
	RL_SHADER_LOC_VERTEX_NORMAL,        // Shader location: vertex attribute: normal
	RL_SHADER_LOC_VERTEX_TANGENT,       // Shader location: vertex attribute: tangent
	RL_SHADER_LOC_VERTEX_COLOR,         // Shader location: vertex attribute: color
	RL_SHADER_LOC_MATRIX_MVP,           // Shader location: matrix uniform: model-view-projection
	RL_SHADER_LOC_MATRIX_VIEW,          // Shader location: matrix uniform: view (camera transform)
	RL_SHADER_LOC_MATRIX_PROJECTION,    // Shader location: matrix uniform: projection
	RL_SHADER_LOC_MATRIX_MODEL,         // Shader location: matrix uniform: model (transform)
	RL_SHADER_LOC_MATRIX_NORMAL,        // Shader location: matrix uniform: normal
	RL_SHADER_LOC_VECTOR_VIEW,          // Shader location: vector uniform: view
	RL_SHADER_LOC_COLOR_DIFFUSE,        // Shader location: vector uniform: diffuse color
	RL_SHADER_LOC_COLOR_SPECULAR,       // Shader location: vector uniform: specular color
	RL_SHADER_LOC_COLOR_AMBIENT,        // Shader location: vector uniform: ambient color
	RL_SHADER_LOC_MAP_ALBEDO,           // Shader location: sampler2d texture: albedo (same as: RL_SHADER_LOC_MAP_DIFFUSE)
	RL_SHADER_LOC_MAP_METALNESS,        // Shader location: sampler2d texture: metalness (same as: RL_SHADER_LOC_MAP_SPECULAR)
	RL_SHADER_LOC_MAP_NORMAL,           // Shader location: sampler2d texture: normal
	RL_SHADER_LOC_MAP_ROUGHNESS,        // Shader location: sampler2d texture: roughness
	RL_SHADER_LOC_MAP_OCCLUSION,        // Shader location: sampler2d texture: occlusion
	RL_SHADER_LOC_MAP_EMISSION,         // Shader location: sampler2d texture: emission
	RL_SHADER_LOC_MAP_HEIGHT,           // Shader location: sampler2d texture: height
	RL_SHADER_LOC_MAP_CUBEMAP,          // Shader location: samplerCube texture: cubemap
	RL_SHADER_LOC_MAP_IRRADIANCE,       // Shader location: samplerCube texture: irradiance
	RL_SHADER_LOC_MAP_PREFILTER,        // Shader location: samplerCube texture: prefilter
	RL_SHADER_LOC_MAP_BRDF              // Shader location: sampler2d texture: brdf
} rlShaderLocationIndex;

#define RL_SHADER_LOC_MAP_DIFFUSE       RL_SHADER_LOC_MAP_ALBEDO
#define RL_SHADER_LOC_MAP_SPECULAR      RL_SHADER_LOC_MAP_METALNESS

// Shader uniform data type
typedef enum {
	RL_SHADER_UNIFORM_FLOAT = 0,        // Shader uniform type: float
	RL_SHADER_UNIFORM_VEC2,             // Shader uniform type: vec2 (2 float)
	RL_SHADER_UNIFORM_VEC3,             // Shader uniform type: vec3 (3 float)
	RL_SHADER_UNIFORM_VEC4,             // Shader uniform type: vec4 (4 float)
	RL_SHADER_UNIFORM_INT,              // Shader uniform type: int
	RL_SHADER_UNIFORM_IVEC2,            // Shader uniform type: ivec2 (2 int)
	RL_SHADER_UNIFORM_IVEC3,            // Shader uniform type: ivec3 (3 int)
	RL_SHADER_UNIFORM_IVEC4,            // Shader uniform type: ivec4 (4 int)
	RL_SHADER_UNIFORM_SAMPLER2D         // Shader uniform type: sampler2d
} rlShaderUniformDataType;

// Shader attribute data types
typedef enum {
	RL_SHADER_ATTRIB_FLOAT = 0,         // Shader attribute type: float
	RL_SHADER_ATTRIB_VEC2,              // Shader attribute type: vec2 (2 float)
	RL_SHADER_ATTRIB_VEC3,              // Shader attribute type: vec3 (3 float)
	RL_SHADER_ATTRIB_VEC4               // Shader attribute type: vec4 (4 float)
} rlShaderAttributeDataType;

// Framebuffer attachment type
// NOTE: By default up to 8 color channels defined, but it can be more
typedef enum {
	RL_ATTACHMENT_COLOR_CHANNEL0 = 0,   // Framebuffer attachment type: color 0
	RL_ATTACHMENT_COLOR_CHANNEL1,       // Framebuffer attachment type: color 1
	RL_ATTACHMENT_COLOR_CHANNEL2,       // Framebuffer attachment type: color 2
	RL_ATTACHMENT_COLOR_CHANNEL3,       // Framebuffer attachment type: color 3
	RL_ATTACHMENT_COLOR_CHANNEL4,       // Framebuffer attachment type: color 4
	RL_ATTACHMENT_COLOR_CHANNEL5,       // Framebuffer attachment type: color 5
	RL_ATTACHMENT_COLOR_CHANNEL6,       // Framebuffer attachment type: color 6
	RL_ATTACHMENT_COLOR_CHANNEL7,       // Framebuffer attachment type: color 7
	RL_ATTACHMENT_DEPTH = 100,          // Framebuffer attachment type: depth
	RL_ATTACHMENT_STENCIL = 200,        // Framebuffer attachment type: stencil
} rlFramebufferAttachType;

// Framebuffer texture attachment type
typedef enum {
	RL_ATTACHMENT_CUBEMAP_POSITIVE_X = 0, // Framebuffer texture attachment type: cubemap, +X side
	RL_ATTACHMENT_CUBEMAP_NEGATIVE_X,   // Framebuffer texture attachment type: cubemap, -X side
	RL_ATTACHMENT_CUBEMAP_POSITIVE_Y,   // Framebuffer texture attachment type: cubemap, +Y side
	RL_ATTACHMENT_CUBEMAP_NEGATIVE_Y,   // Framebuffer texture attachment type: cubemap, -Y side
	RL_ATTACHMENT_CUBEMAP_POSITIVE_Z,   // Framebuffer texture attachment type: cubemap, +Z side
	RL_ATTACHMENT_CUBEMAP_NEGATIVE_Z,   // Framebuffer texture attachment type: cubemap, -Z side
	RL_ATTACHMENT_TEXTURE2D = 100,      // Framebuffer texture attachment type: texture2d
	RL_ATTACHMENT_RENDERBUFFER = 200,   // Framebuffer texture attachment type: renderbuffer
} rlFramebufferAttachTextureType;

// Face culling mode
typedef enum {
	RL_CULL_FACE_FRONT = 0,
	RL_CULL_FACE_BACK
} rlCullMode;

//------------------------------------------------------------------------------------
// Functions Declaration - Matrix operations
//------------------------------------------------------------------------------------

#if defined(__cplusplus)
extern "C" {            // Prevents name mangling of functions
	#endif

	RLAPI void rlMatrixMode(int mode);                    // Choose the current matrix to be transformed
	RLAPI void rlPushMatrix(void);                        // Push the current matrix to stack
	RLAPI void rlPopMatrix(void);                         // Pop latest inserted matrix from stack
	RLAPI void rlLoadIdentity(void);                      // Reset current matrix to identity matrix
	RLAPI void rlTranslatef(float x, float y, float z);   // Multiply the current matrix by a translation matrix
	RLAPI void rlRotatef(float angle, float x, float y, float z);  // Multiply the current matrix by a rotation matrix
	RLAPI void rlScalef(float x, float y, float z);       // Multiply the current matrix by a scaling matrix
	RLAPI void rlMultMatrixf(const float *matf);                // Multiply the current matrix by another matrix
	RLAPI void rlFrustum(double left, double right, double bottom, double top, double znear, double zfar);
	RLAPI void rlOrtho(double left, double right, double bottom, double top, double znear, double zfar);
	RLAPI void rlViewport(int x, int y, int width, int height); // Set the viewport area

	//------------------------------------------------------------------------------------
	// Functions Declaration - Vertex level operations
	//------------------------------------------------------------------------------------
	RLAPI void rlBegin(int mode);                         // Initialize drawing mode (how to organize vertex)
	RLAPI void rlEnd(void);                               // Finish vertex providing
	RLAPI void rlVertex2i(int x, int y);                  // Define one vertex (position) - 2 int
	RLAPI void rlVertex2f(float x, float y);              // Define one vertex (position) - 2 float
	RLAPI void rlVertex3f(float x, float y, float z);     // Define one vertex (position) - 3 float
	RLAPI void rlTexCoord2f(float x, float y);            // Define one vertex (texture coordinate) - 2 float
	RLAPI void rlNormal3f(float x, float y, float z);     // Define one vertex (normal) - 3 float
	RLAPI void rlColor4ub(unsigned char r, unsigned char g, unsigned char b, unsigned char a);  // Define one vertex (color) - 4 byte
	RLAPI void rlColor3f(float x, float y, float z);          // Define one vertex (color) - 3 float
	RLAPI void rlColor4f(float x, float y, float z, float w); // Define one vertex (color) - 4 float

	//------------------------------------------------------------------------------------
	// Functions Declaration - OpenGL style functions (common to 1.1, 3.3+, ES2)
	// NOTE: This functions are used to completely abstract raylib code from OpenGL layer,
	// some of them are direct wrappers over OpenGL calls, some others are custom
	//------------------------------------------------------------------------------------

	// Vertex buffers state
	RLAPI bool rlEnableVertexArray(unsigned int vaoId);     // Enable vertex array (VAO, if supported)
	RLAPI void rlDisableVertexArray(void);                  // Disable vertex array (VAO, if supported)
	RLAPI void rlEnableVertexBuffer(unsigned int id);       // Enable vertex buffer (VBO)
	RLAPI void rlDisableVertexBuffer(void);                 // Disable vertex buffer (VBO)
	RLAPI void rlEnableVertexBufferElement(unsigned int id);// Enable vertex buffer element (VBO element)
	RLAPI void rlDisableVertexBufferElement(void);          // Disable vertex buffer element (VBO element)
	RLAPI void rlEnableVertexAttribute(unsigned int index); // Enable vertex attribute index
	RLAPI void rlDisableVertexAttribute(unsigned int index);// Disable vertex attribute index

	// Textures state
	RLAPI void rlActiveTextureSlot(int slot);               // Select and active a texture slot
	RLAPI void rlEnableTexture(unsigned int id);            // Enable texture
	RLAPI void rlDisableTexture(void);                      // Disable texture
	RLAPI void rlEnableTextureCubemap(unsigned int id);     // Enable texture cubemap
	RLAPI void rlDisableTextureCubemap(void);               // Disable texture cubemap
	RLAPI void rlTextureParameters(unsigned int id, int param, int value); // Set texture parameters (filter, wrap)
	RLAPI void rlCubemapParameters(unsigned int id, int param, int value); // Set cubemap parameters (filter, wrap)

	// Shader state
	RLAPI void rlEnableShader(unsigned int id);             // Enable shader program
	RLAPI void rlDisableShader(void);                       // Disable shader program

	// Framebuffer state
	RLAPI void rlEnableFramebuffer(unsigned int id);        // Enable render texture (fbo)
	RLAPI void rlDisableFramebuffer(void);                  // Disable render texture (fbo), return to default framebuffer
	RLAPI void rlActiveDrawBuffers(int count);              // Activate multiple draw color buffers

	// General render state
	RLAPI void rlEnableColorBlend(void);                     // Enable color blending
	RLAPI void rlDisableColorBlend(void);                   // Disable color blending
	RLAPI void rlEnableDepthTest(void);                     // Enable depth test
	RLAPI void rlDisableDepthTest(void);                    // Disable depth test
	RLAPI void rlEnableDepthMask(void);                     // Enable depth write
	RLAPI void rlDisableDepthMask(void);                    // Disable depth write
	RLAPI void rlEnableBackfaceCulling(void);               // Enable backface culling
	RLAPI void rlDisableBackfaceCulling(void);              // Disable backface culling
	RLAPI void rlSetCullFace(int mode);                     // Set face culling mode
	RLAPI void rlEnableScissorTest(void);                   // Enable scissor test
	RLAPI void rlDisableScissorTest(void);                  // Disable scissor test
	RLAPI void rlScissor(int x, int y, int width, int height); // Scissor test
	RLAPI void rlEnableWireMode(void);                      // Enable wire mode
	RLAPI void rlDisableWireMode(void);                     // Disable wire mode
	RLAPI void rlSetLineWidth(float width);                 // Set the line drawing width
	RLAPI float rlGetLineWidth(void);                       // Get the line drawing width
	RLAPI void rlEnableSmoothLines(void);                   // Enable line aliasing
	RLAPI void rlDisableSmoothLines(void);                  // Disable line aliasing
	RLAPI void rlEnableStereoRender(void);                  // Enable stereo rendering
	RLAPI void rlDisableStereoRender(void);                 // Disable stereo rendering
	RLAPI bool rlIsStereoRenderEnabled(void);               // Check if stereo render is enabled

	RLAPI void rlClearColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a); // Clear color buffer with color
	RLAPI void rlClearScreenBuffers(void);                  // Clear used screen buffers (color and depth)
	RLAPI void rlCheckErrors(void);                         // Check and log OpenGL error codes
	RLAPI void rlSetBlendMode(int mode);                    // Set blending mode
	RLAPI void rlSetBlendFactors(int glSrcFactor, int glDstFactor, int glEquation); // Set blending mode factor and equation (using OpenGL factors)
	RLAPI void rlSetBlendFactorsSeparate(int glSrcRGB, int glDstRGB, int glSrcAlpha, int glDstAlpha, int glEqRGB, int glEqAlpha); // Set blending mode factors and equations separately (using OpenGL factors)

	//------------------------------------------------------------------------------------
	// Functions Declaration - rlgl functionality
	//------------------------------------------------------------------------------------
	// rlgl initialization functions
	RLAPI void rlglInit(int width, int height);             // Initialize rlgl (buffers, shaders, textures, states)
	RLAPI void rlglClose(void);                             // De-initialize rlgl (buffers, shaders, textures)
	RLAPI void rlLoadExtensions(void *loader);              // Load OpenGL extensions (loader function required)
	RLAPI int rlGetVersion(void);                           // Get current OpenGL version
	RLAPI void rlSetFramebufferWidth(int width);            // Set current framebuffer width
	RLAPI int rlGetFramebufferWidth(void);                  // Get default framebuffer width
	RLAPI void rlSetFramebufferHeight(int height);          // Set current framebuffer height
	RLAPI int rlGetFramebufferHeight(void);                 // Get default framebuffer height

	RLAPI unsigned int rlGetTextureIdDefault(void);         // Get default texture id
	RLAPI unsigned int rlGetShaderIdDefault(void);          // Get default shader id
	RLAPI int *rlGetShaderLocsDefault(void);                // Get default shader locations

	// Render batch management
	// NOTE: rlgl provides a default render batch to behave like OpenGL 1.1 immediate mode
	// but this render batch API is exposed in case of custom batches are required
	RLAPI rlRenderBatch rlLoadRenderBatch(int numBuffers, int bufferElements);  // Load a render batch system
	RLAPI void rlUnloadRenderBatch(rlRenderBatch batch);                        // Unload render batch system
	RLAPI void rlDrawRenderBatch(rlRenderBatch *batch);                         // Draw render batch data (Update->Draw->Reset)
	RLAPI void rlSetRenderBatchActive(rlRenderBatch *batch);                    // Set the active render batch for rlgl (NULL for default internal)
	RLAPI void rlDrawRenderBatchActive(void);                                   // Update and draw internal render batch
	RLAPI bool rlCheckRenderBatchLimit(int vCount);                             // Check internal buffer overflow for a given number of vertex

	RLAPI void rlSetTexture(unsigned int id);               // Set current texture for render batch and check buffers limits

	//------------------------------------------------------------------------------------------------------------------------

	// Vertex buffers management
	RLAPI unsigned int rlLoadVertexArray(void);                               // Load vertex array (vao) if supported
	RLAPI unsigned int rlLoadVertexBuffer(const void *buffer, int size, bool dynamic);            // Load a vertex buffer attribute
	RLAPI unsigned int rlLoadVertexBufferElement(const void *buffer, int size, bool dynamic);     // Load a new attributes element buffer
	RLAPI void rlUpdateVertexBuffer(unsigned int bufferId, const void *data, int dataSize, int offset);     // Update GPU buffer with new data
	RLAPI void rlUpdateVertexBufferElements(unsigned int id, const void *data, int dataSize, int offset);   // Update vertex buffer elements with new data
	RLAPI void rlUnloadVertexArray(unsigned int vaoId);
	RLAPI void rlUnloadVertexBuffer(unsigned int vboId);
	RLAPI void rlSetVertexAttribute(unsigned int index, int compSize, int type, bool normalized, int stride, const void *pointer);
	RLAPI void rlSetVertexAttributeDivisor(unsigned int index, int divisor);
	RLAPI void rlSetVertexAttributeDefault(int locIndex, const void *value, int attribType, int count); // Set vertex attribute default value
	RLAPI void rlDrawVertexArray(int offset, int count);
	RLAPI void rlDrawVertexArrayElements(int offset, int count, const void *buffer);
	RLAPI void rlDrawVertexArrayInstanced(int offset, int count, int instances);
	RLAPI void rlDrawVertexArrayElementsInstanced(int offset, int count, const void *buffer, int instances);

	// Textures management
	RLAPI rlTexture *rlLoadTexture(const void *data, int width, int height, int format, int mipmapCount); // Load texture in GPU
	RLAPI rlTexture *rlLoadTextureDepth(int width, int height, bool useRenderBuffer);                     // Load depth texture/renderbuffer (to be attached to fbo)
	RLAPI rlTexture *rlLoadTextureCubemap(const void *data, int size, int format);                        // Load texture cubemap
	RLAPI void rlUpdateTexture(unsigned int id, int offsetX, int offsetY, int width, int height, int format, const void *data);  // Update GPU texture with new data
	RLAPI void rlGetGlTextureFormats(int format, unsigned int *glInternalFormat, unsigned int *glFormat, unsigned int *glType);  // Get OpenGL internal formats
	RLAPI const char *rlGetPixelFormatName(unsigned int format);              // Get name string for pixel format
	RLAPI void rlUnloadTexture(unsigned int id);                              // Unload texture from GPU memory
	RLAPI void rlGenTextureMipmaps(const rlTexture *texture, int width, int height, int format, int *mipmaps); // Generate mipmap data for selected texture
	RLAPI void *rlReadTexturePixels(unsigned int id, int width, int height, int format);              // Read texture pixel data
	RLAPI unsigned char *rlReadScreenPixels(int width, int height);           // Read screen pixel data (color buffer)

	// Framebuffer management (fbo)
	RLAPI unsigned int rlLoadFramebuffer(int width, int height);              // Load an empty framebuffer
	RLAPI void rlFramebufferAttach(unsigned int fboId, unsigned int texId, int attachType, int texType, int mipLevel);  // Attach texture/renderbuffer to a framebuffer
	RLAPI bool rlFramebufferComplete(unsigned int id);                        // Verify framebuffer is complete
	RLAPI void rlUnloadFramebuffer(unsigned int id);                          // Delete framebuffer from GPU

	// Shaders management
	RLAPI unsigned int rlLoadShaderCode(const char *vsCode, const char *fsCode);    // Load shader from code strings
	RLAPI unsigned int rlCompileShader(const char *shaderCode, int type);           // Compile custom shader and return shader id (type: RL_VERTEX_SHADER, RL_FRAGMENT_SHADER, RL_COMPUTE_SHADER)
	RLAPI unsigned int rlLoadShaderProgram(unsigned int vShaderId, unsigned int fShaderId); // Load custom shader program
	RLAPI void rlUnloadShaderProgram(unsigned int id);                              // Unload shader program
	RLAPI int rlGetLocationUniform(unsigned int shaderId, const char *uniformName); // Get shader location uniform
	RLAPI int rlGetLocationAttrib(unsigned int shaderId, const char *attribName);   // Get shader location attribute
	RLAPI void rlSetUniform(int locIndex, const void *value, int uniformType, int count);   // Set shader value uniform
	RLAPI void rlSetUniformMatrix(int locIndex, Matrix mat);                        // Set shader value matrix
	RLAPI void rlSetUniformSampler(int locIndex, unsigned int textureId);           // Set shader value sampler
	RLAPI void rlSetShader(unsigned int id, int *locs);                             // Set shader currently active (id and locations)

	// Compute shader management
	RLAPI unsigned int rlLoadComputeShaderProgram(unsigned int shaderId);           // Load compute shader program
	RLAPI void rlComputeShaderDispatch(unsigned int groupX, unsigned int groupY, unsigned int groupZ);  // Dispatch compute shader (equivalent to *draw* for graphics pipeline)

	// Shader buffer storage object management (ssbo)
	RLAPI unsigned int rlLoadShaderBuffer(unsigned int size, const void *data, int usageHint); // Load shader storage buffer object (SSBO)
	RLAPI void rlUnloadShaderBuffer(unsigned int ssboId);                           // Unload shader storage buffer object (SSBO)
	RLAPI void rlUpdateShaderBuffer(unsigned int id, const void *data, unsigned int dataSize, unsigned int offset); // Update SSBO buffer data
	RLAPI void rlBindShaderBuffer(unsigned int id, unsigned int index);             // Bind SSBO buffer
	RLAPI void rlReadShaderBuffer(unsigned int id, void *dest, unsigned int count, unsigned int offset); // Read SSBO buffer data (GPU->CPU)
	RLAPI void rlCopyShaderBuffer(unsigned int destId, unsigned int srcId, unsigned int destOffset, unsigned int srcOffset, unsigned int count); // Copy SSBO data between buffers
	RLAPI unsigned int rlGetShaderBufferSize(unsigned int id);                      // Get SSBO buffer size

	// Buffer management
	RLAPI void rlBindImageTexture(unsigned int id, unsigned int index, int format, bool readonly);  // Bind image texture

	// Matrix state management
	RLAPI Matrix rlGetMatrixModelview(void);                                  // Get internal modelview matrix
	RLAPI Matrix rlGetMatrixProjection(void);                                 // Get internal projection matrix
	RLAPI Matrix rlGetMatrixTransform(void);                                  // Get internal accumulated transform matrix
	RLAPI Matrix rlGetMatrixProjectionStereo(int eye);                        // Get internal projection matrix for stereo render (selected eye)
	RLAPI Matrix rlGetMatrixViewOffsetStereo(int eye);                        // Get internal view offset matrix for stereo render (selected eye)
	RLAPI void rlSetMatrixProjection(Matrix proj);                            // Set a custom projection matrix (replaces internal projection matrix)
	RLAPI void rlSetMatrixModelview(Matrix view);                             // Set a custom modelview matrix (replaces internal modelview matrix)
	RLAPI void rlSetMatrixProjectionStereo(Matrix right, Matrix left);        // Set eyes projection matrices for stereo rendering
	RLAPI void rlSetMatrixViewOffsetStereo(Matrix right, Matrix left);        // Set eyes view offsets matrices for stereo rendering

	// Quick and dirty cube/quad buffers load->draw->unload
	RLAPI void rlLoadDrawCube(void);     // Load and draw a cube
	RLAPI void rlLoadDrawQuad(void);     // Load and draw a quad

	#if defined(__cplusplus)
}
#endif

#endif // RLGL_H

/***********************************************************************************
*
*   RLGH IMPLEMENTATION
*
************************************************************************************/

#if defined(RLGL_IMPLEMENTATION)

#define GLAD_MALLOC RL_MALLOC
#define GLAD_FREE RL_FREE

#define GLAD_GL_IMPLEMENTATION
//#include "external/glad.h"      // GLAD extensions loading library, includes OpenGL headers

#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi.h>

#include <stdlib.h>                     // Required for: malloc(), free()
#include <string.h>                     // Required for: strcmp(), strlen() [Used in rlglInit(), on extensions loading]
#include <math.h>                       // Required for: sqrtf(), sinf(), cosf(), floor(), log()

//----------------------------------------------------------------------------------
// Defines and Macros
//----------------------------------------------------------------------------------
#ifndef PI
#define PI 3.14159265358979323846f
#endif
#ifndef DEG2RAD
#define DEG2RAD (PI/180.0f)
#endif
#ifndef RAD2DEG
#define RAD2DEG (180.0f/PI)
#endif

#ifndef GL_SHADING_LANGUAGE_VERSION
#define GL_SHADING_LANGUAGE_VERSION         0x8B8C
#endif

#ifndef GL_COMPRESSED_RGB_S3TC_DXT1_EXT
#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT     0x83F0
#endif
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT    0x83F1
#endif
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT    0x83F2
#endif
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT    0x83F3
#endif
#ifndef GL_ETC1_RGB8_OES
#define GL_ETC1_RGB8_OES                    0x8D64
#endif
#ifndef GL_COMPRESSED_RGB8_ETC2
#define GL_COMPRESSED_RGB8_ETC2             0x9274
#endif
#ifndef GL_COMPRESSED_RGBA8_ETC2_EAC
#define GL_COMPRESSED_RGBA8_ETC2_EAC        0x9278
#endif
#ifndef GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG
#define GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG  0x8C00
#endif
#ifndef GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG
#define GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG 0x8C02
#endif
#ifndef GL_COMPRESSED_RGBA_ASTC_4x4_KHR
#define GL_COMPRESSED_RGBA_ASTC_4x4_KHR     0x93b0
#endif
#ifndef GL_COMPRESSED_RGBA_ASTC_8x8_KHR
#define GL_COMPRESSED_RGBA_ASTC_8x8_KHR     0x93b7
#endif

#ifndef GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT   0x84FF
#endif
#ifndef GL_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_TEXTURE_MAX_ANISOTROPY_EXT       0x84FE
#endif

// Default shader vertex attribute names to set location points
#ifndef RL_DEFAULT_SHADER_ATTRIB_NAME_POSITION
#define RL_DEFAULT_SHADER_ATTRIB_NAME_POSITION     "vertexPosition"    // Bound by default to shader location: 0
#endif
#ifndef RL_DEFAULT_SHADER_ATTRIB_NAME_TEXCOORD
#define RL_DEFAULT_SHADER_ATTRIB_NAME_TEXCOORD     "vertexTexCoord"    // Bound by default to shader location: 1
#endif
#ifndef RL_DEFAULT_SHADER_ATTRIB_NAME_NORMAL
#define RL_DEFAULT_SHADER_ATTRIB_NAME_NORMAL       "vertexNormal"      // Bound by default to shader location: 2
#endif
#ifndef RL_DEFAULT_SHADER_ATTRIB_NAME_COLOR
#define RL_DEFAULT_SHADER_ATTRIB_NAME_COLOR        "vertexColor"       // Bound by default to shader location: 3
#endif
#ifndef RL_DEFAULT_SHADER_ATTRIB_NAME_TANGENT
#define RL_DEFAULT_SHADER_ATTRIB_NAME_TANGENT      "vertexTangent"     // Bound by default to shader location: 4
#endif
#ifndef RL_DEFAULT_SHADER_ATTRIB_NAME_TEXCOORD2
#define RL_DEFAULT_SHADER_ATTRIB_NAME_TEXCOORD2    "vertexTexCoord2"   // Bound by default to shader location: 5
#endif

#ifndef RL_DEFAULT_SHADER_UNIFORM_NAME_MVP
#define RL_DEFAULT_SHADER_UNIFORM_NAME_MVP         "mvp"               // model-view-projection matrix
#endif
#ifndef RL_DEFAULT_SHADER_UNIFORM_NAME_VIEW
#define RL_DEFAULT_SHADER_UNIFORM_NAME_VIEW        "matView"           // view matrix
#endif
#ifndef RL_DEFAULT_SHADER_UNIFORM_NAME_PROJECTION
#define RL_DEFAULT_SHADER_UNIFORM_NAME_PROJECTION  "matProjection"     // projection matrix
#endif
#ifndef RL_DEFAULT_SHADER_UNIFORM_NAME_MODEL
#define RL_DEFAULT_SHADER_UNIFORM_NAME_MODEL       "matModel"          // model matrix
#endif
#ifndef RL_DEFAULT_SHADER_UNIFORM_NAME_NORMAL
#define RL_DEFAULT_SHADER_UNIFORM_NAME_NORMAL      "matNormal"         // normal matrix (transpose(inverse(matModelView))
#endif
#ifndef RL_DEFAULT_SHADER_UNIFORM_NAME_COLOR
#define RL_DEFAULT_SHADER_UNIFORM_NAME_COLOR       "colDiffuse"        // color diffuse (base tint color, multiplied by texture color)
#endif
#ifndef RL_DEFAULT_SHADER_SAMPLER2D_NAME_TEXTURE0
#define RL_DEFAULT_SHADER_SAMPLER2D_NAME_TEXTURE0  "texture0"          // texture0 (texture slot active 0)
#endif
#ifndef RL_DEFAULT_SHADER_SAMPLER2D_NAME_TEXTURE1
#define RL_DEFAULT_SHADER_SAMPLER2D_NAME_TEXTURE1  "texture1"          // texture1 (texture slot active 1)
#endif
#ifndef RL_DEFAULT_SHADER_SAMPLER2D_NAME_TEXTURE2
#define RL_DEFAULT_SHADER_SAMPLER2D_NAME_TEXTURE2  "texture2"          // texture2 (texture slot active 2)
#endif

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
typedef struct rlGraphicsData {
	rlRenderBatch *currentBatch;            // Current render batch
	rlRenderBatch defaultBatch;             // Default internal render batch

	D3D_FEATURE_LEVEL feature_level;
	ID3D11Device *device;
	ID3D11DeviceContext *context;

	struct {
		int vertexCounter;                  // Current active render batch vertex counter (generic, used for all batches)
		float texcoordx, texcoordy;         // Current active texture coordinate (added on glVertex*())
		float normalx, normaly, normalz;    // Current active normal (added on glVertex*())
		unsigned char colorr, colorg, colorb, colora;   // Current active color (added on glVertex*())

		int currentMatrixMode;              // Current matrix mode
		Matrix *currentMatrix;              // Current matrix pointer
		Matrix modelview;                   // Default modelview matrix
		Matrix projection;                  // Default projection matrix
		Matrix transform;                   // Transform matrix to be used with rlTranslate, rlRotate, rlScale
		bool transformRequired;             // Require transform matrix application to current draw-call vertex (if required)
		Matrix stack[RL_MAX_MATRIX_STACK_SIZE];// Matrix stack for push/pop
		int stackCounter;                   // Matrix stack counter

		rlTexture defaultTexture;           // Default texture used on shapes/poly drawing (required by shader)
		rlTexture activeTextureId[RL_DEFAULT_BATCH_MAX_TEXTURE_UNITS];    // Active texture ids to be enabled on batch drawing (0 active by default)
		rlVertexShader defaultVShaderId;    // Default vertex shader id (used by default shader program)
		rlPixelShader  defaultFShaderId;    // Default fragment shader id (used by default shader program)
		rlShader       defaultShaderId;     // Default shader program id, supports vertex color and diffuse texture
		int *defaultShaderLocs;             // Default shader locations pointer to be used on rendering
		unsigned int currentShaderId;       // Current shader id to be used on rendering (by default, defaultShaderId)
		int *currentShaderLocs;             // Current shader locations pointer to be used on rendering (by default, defaultShaderLocs)

		bool stereoRender;                  // Stereo rendering flag
		Matrix projectionStereo[2];         // VR stereo rendering eyes projection matrices
		Matrix viewOffsetStereo[2];         // VR stereo rendering eyes view offset matrices

		// Blending variables
		int currentBlendMode;               // Blending mode active
		int glBlendSrcFactor;               // Blending source factor
		int glBlendDstFactor;               // Blending destination factor
		int glBlendEquation;                // Blending equation
		int glBlendSrcFactorRGB;            // Blending source RGB factor
		int glBlendDestFactorRGB;           // Blending destination RGB factor
		int glBlendSrcFactorAlpha;          // Blending source alpha factor
		int glBlendDestFactorAlpha;         // Blending destination alpha factor
		int glBlendEquationRGB;             // Blending equation for RGB
		int glBlendEquationAlpha;           // Blending equation for alpha
		bool glCustomBlendModeModified;     // Custom blending factor and equation modification status

		int framebufferWidth;               // Current framebuffer width
		int framebufferHeight;              // Current framebuffer height

		struct {
			int x;
			int y;
			int width;
			int height;
		} Viewport;

		struct {
			int x;
			int y;
			int width;
			int height;
		} Scissor;

	} State;            // Renderer state
	struct {
		bool texCompDXT;                    // DDS texture compression support (GL_EXT_texture_compression_s3tc, GL_WEBGL_compressed_texture_s3tc, GL_WEBKIT_WEBGL_compressed_texture_s3tc)
		bool texCompETC1;                   // ETC1 texture compression support (GL_OES_compressed_ETC1_RGB8_texture, GL_WEBGL_compressed_texture_etc1)
		bool texCompETC2;                   // ETC2/EAC texture compression support (GL_ARB_ES3_compatibility)
		bool texCompPVRT;                   // PVR texture compression support (GL_IMG_texture_compression_pvrtc)
		bool texCompASTC;                   // ASTC texture compression support (GL_KHR_texture_compression_astc_hdr, GL_KHR_texture_compression_astc_ldr)
		bool computeShader;                 // Compute shaders support (GL_ARB_compute_shader)

		float maxAnisotropyLevel;           // Maximum anisotropy level supported (minimum is 2.0f)
		int maxDepthBits;                   // Maximum bits for depth component

	} ExtSupported;     // Extensions supported flags
} rlGraphicsData;

typedef void *(*rlglLoadProc)(const char *name);   // OpenGL extension functions loader signature (same as GLADloadproc)

//----------------------------------------------------------------------------------
// Global Variables Definition
//----------------------------------------------------------------------------------
static rlGraphicsData RLGH = { 0 };

//----------------------------------------------------------------------------------
// Module specific Functions Declaration
//----------------------------------------------------------------------------------
static void rlLoadShaderDefault(void);      // Load default shader
static void rlUnloadShaderDefault(void);    // Unload default shader

#if defined(RLGL_SHOW_GL_DETAILS_INFO)
static char *rlGetCompressedFormatName(int format); // Get compressed format official GL identifier name
#endif  // RLGL_SHOW_GL_DETAILS_INFO

static int rlGetPixelDataSize(int width, int height, int format);   // Get pixel data size in bytes (image or texture)

// Auxiliar matrix math functions
static Matrix rlMatrixIdentity(void);                       // Get identity matrix
static Matrix rlMatrixMultiply(Matrix left, Matrix right);  // Multiply two matrices

//----------------------------------------------------------------------------------
// Module Functions Definition - Matrix operations
//----------------------------------------------------------------------------------

// Choose the current matrix to be transformed
void rlMatrixMode(int mode)
{
	if (mode == RL_PROJECTION)
		RLGH.State.currentMatrix = &RLGH.State.projection;
	else if (mode == RL_MODELVIEW)
		RLGH.State.currentMatrix = &RLGH.State.modelview;
	//else if (mode == RL_TEXTURE) // Not supported

	RLGH.State.currentMatrixMode = mode;
}

// Push the current matrix into RLGH.State.stack
void rlPushMatrix(void)
{
	if (RLGH.State.stackCounter >= RL_MAX_MATRIX_STACK_SIZE)
		TRACELOG(RL_LOG_ERROR, "RLGH: Matrix stack overflow (RL_MAX_MATRIX_STACK_SIZE)");

	if (RLGH.State.currentMatrixMode == RL_MODELVIEW)
	{
		RLGH.State.transformRequired = true;
		RLGH.State.currentMatrix = &RLGH.State.transform;
	}

	RLGH.State.stack[RLGH.State.stackCounter] = *RLGH.State.currentMatrix;
	RLGH.State.stackCounter++;
}

// Pop lattest inserted matrix from RLGH.State.stack
void rlPopMatrix(void)
{
	if (RLGH.State.stackCounter > 0)
	{
		Matrix mat = RLGH.State.stack[RLGH.State.stackCounter - 1];
		*RLGH.State.currentMatrix = mat;
		RLGH.State.stackCounter--;
	}

	if ((RLGH.State.stackCounter == 0) && (RLGH.State.currentMatrixMode == RL_MODELVIEW))
	{
		RLGH.State.currentMatrix = &RLGH.State.modelview;
		RLGH.State.transformRequired = false;
	}
}

// Reset current matrix to identity matrix
void rlLoadIdentity(void)
{
	*RLGH.State.currentMatrix = rlMatrixIdentity();
}

// Multiply the current matrix by a translation matrix
void rlTranslatef(float x, float y, float z)
{
	Matrix matTranslation = {
		1.0f, 0.0f, 0.0f, x,
		0.0f, 1.0f, 0.0f, y,
		0.0f, 0.0f, 1.0f, z,
		0.0f, 0.0f, 0.0f, 1.0f
	};

	// NOTE: We transpose matrix with multiplication order
	*RLGH.State.currentMatrix = rlMatrixMultiply(matTranslation, *RLGH.State.currentMatrix);
}

// Multiply the current matrix by a rotation matrix
// NOTE: The provided angle must be in degrees
void rlRotatef(float angle, float x, float y, float z)
{
	Matrix matRotation = rlMatrixIdentity();

	// Axis vector (x, y, z) normalization
	float lengthSquared = x * x + y * y + z * z;
	if ((lengthSquared != 1.0f) && (lengthSquared != 0.0f))
	{
		float inverseLength = 1.0f / sqrtf(lengthSquared);
		x *= inverseLength;
		y *= inverseLength;
		z *= inverseLength;
	}

	// Rotation matrix generation
	float sinres = sinf(DEG2RAD * angle);
	float cosres = cosf(DEG2RAD * angle);
	float t = 1.0f - cosres;

	matRotation.m0 = x * x * t + cosres;
	matRotation.m1 = y * x * t + z * sinres;
	matRotation.m2 = z * x * t - y * sinres;
	matRotation.m3 = 0.0f;

	matRotation.m4 = x * y * t - z * sinres;
	matRotation.m5 = y * y * t + cosres;
	matRotation.m6 = z * y * t + x * sinres;
	matRotation.m7 = 0.0f;

	matRotation.m8 = x * z * t + y * sinres;
	matRotation.m9 = y * z * t - x * sinres;
	matRotation.m10 = z * z * t + cosres;
	matRotation.m11 = 0.0f;

	matRotation.m12 = 0.0f;
	matRotation.m13 = 0.0f;
	matRotation.m14 = 0.0f;
	matRotation.m15 = 1.0f;

	// NOTE: We transpose matrix with multiplication order
	*RLGH.State.currentMatrix = rlMatrixMultiply(matRotation, *RLGH.State.currentMatrix);
}

// Multiply the current matrix by a scaling matrix
void rlScalef(float x, float y, float z)
{
	Matrix matScale = {
		x, 0.0f, 0.0f, 0.0f,
		0.0f, y, 0.0f, 0.0f,
		0.0f, 0.0f, z, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};

	// NOTE: We transpose matrix with multiplication order
	*RLGH.State.currentMatrix = rlMatrixMultiply(matScale, *RLGH.State.currentMatrix);
}

// Multiply the current matrix by another matrix
void rlMultMatrixf(const float *matf)
{
	// Matrix creation from array
	Matrix mat = { matf[0], matf[4], matf[8], matf[12],
				   matf[1], matf[5], matf[9], matf[13],
				   matf[2], matf[6], matf[10], matf[14],
				   matf[3], matf[7], matf[11], matf[15] };

	*RLGH.State.currentMatrix = rlMatrixMultiply(*RLGH.State.currentMatrix, mat);
}

// Multiply the current matrix by a perspective matrix generated by parameters
void rlFrustum(double left, double right, double bottom, double top, double znear, double zfar)
{
	Matrix matFrustum = { 0 };

	float rl = (float)(right - left);
	float tb = (float)(top - bottom);
	float fn = (float)(zfar - znear);

	matFrustum.m0 = ((float)znear * 2.0f) / rl;
	matFrustum.m1 = 0.0f;
	matFrustum.m2 = 0.0f;
	matFrustum.m3 = 0.0f;

	matFrustum.m4 = 0.0f;
	matFrustum.m5 = ((float)znear * 2.0f) / tb;
	matFrustum.m6 = 0.0f;
	matFrustum.m7 = 0.0f;

	matFrustum.m8 = ((float)right + (float)left) / rl;
	matFrustum.m9 = ((float)top + (float)bottom) / tb;
	matFrustum.m10 = -((float)zfar + (float)znear) / fn;
	matFrustum.m11 = -1.0f;

	matFrustum.m12 = 0.0f;
	matFrustum.m13 = 0.0f;
	matFrustum.m14 = -((float)zfar * (float)znear * 2.0f) / fn;
	matFrustum.m15 = 0.0f;

	*RLGH.State.currentMatrix = rlMatrixMultiply(*RLGH.State.currentMatrix, matFrustum);
}

// Multiply the current matrix by an orthographic matrix generated by parameters
void rlOrtho(double left, double right, double bottom, double top, double znear, double zfar)
{
	// NOTE: If left-right and top-botton values are equal it could create a division by zero,
	// response to it is platform/compiler dependant
	Matrix matOrtho = { 0 };

	float rl = (float)(right - left);
	float tb = (float)(top - bottom);
	float fn = (float)(zfar - znear);

	matOrtho.m0 = 2.0f / rl;
	matOrtho.m1 = 0.0f;
	matOrtho.m2 = 0.0f;
	matOrtho.m3 = 0.0f;
	matOrtho.m4 = 0.0f;
	matOrtho.m5 = 2.0f / tb;
	matOrtho.m6 = 0.0f;
	matOrtho.m7 = 0.0f;
	matOrtho.m8 = 0.0f;
	matOrtho.m9 = 0.0f;
	matOrtho.m10 = -2.0f / fn;
	matOrtho.m11 = 0.0f;
	matOrtho.m12 = -((float)left + (float)right) / rl;
	matOrtho.m13 = -((float)top + (float)bottom) / tb;
	matOrtho.m14 = -((float)zfar + (float)znear) / fn;
	matOrtho.m15 = 1.0f;

	*RLGH.State.currentMatrix = rlMatrixMultiply(*RLGH.State.currentMatrix, matOrtho);
}

// Set the viewport area (transformation from normalized device coordinates to window coordinates)
// NOTE: We store current viewport dimensions
void rlViewport(int x, int y, int width, int height)
{
	RLGH.State.Viewport.x = x;
	RLGH.State.Viewport.y = y;
	RLGH.State.Viewport.width = width;
	RLGH.State.Viewport.height = height;
}

//----------------------------------------------------------------------------------
// Module Functions Definition - Vertex level operations
//----------------------------------------------------------------------------------

// Initialize drawing mode (how to organize vertex)
void rlBegin(int mode)
{
	// Draw mode can be RL_LINES, RL_TRIANGLES and RL_QUADS
	// NOTE: In all three cases, vertex are accumulated over default internal vertex buffer
	rlDrawCall *drawcall = &RLGH.currentBatch->draws[RLGH.currentBatch->drawCounter - 1];
	if (drawcall->mode != mode)
	{
		int vertex_count = vertex_count;
		if (vertex_count > 0)
		{
			// Make sure current RLGH.currentBatch->draws[i].vertexCount is aligned a multiple of 4,
			// that way, following QUADS drawing will keep aligned with index processing
			// It implies adding some extra alignment vertex at the end of the draw,
			// those vertex are not processed but they are considered as an additional offset
			// for the next set of vertex to be drawn
			if (drawcall->mode == RL_LINES)
				drawcall->vertexAlignment = ((vertex_count < 4) ? vertex_count : vertex_count % 4);
			else if (drawcall->mode == RL_TRIANGLES)
				drawcall->vertexAlignment = ((vertex_count < 4) ? 1 : (4 - (vertex_count % 4)));
			else
				drawcall->vertexAlignment = 0;

			if (!rlCheckRenderBatchLimit(drawcall->vertexAlignment))
			{
				RLGH.State.vertexCounter += drawcall->vertexAlignment;
				RLGH.currentBatch->drawCounter++;
				drawcall = &RLGH.currentBatch->draws[RLGH.currentBatch->drawCounter - 1];
			}
		}

		if (RLGH.currentBatch->drawCounter >= RL_DEFAULT_BATCH_DRAWCALLS)
			rlDrawRenderBatch(RLGH.currentBatch);

		drawcall->mode = mode;
		vertex_count = 0;
		drawcall->texture = RLGH.State.defaultTexture;
	}
}

// Finish vertex providing
void rlEnd(void)
{
	// NOTE: Depth increment is dependant on rlOrtho(): z-near and z-far values,
	// as well as depth buffer bit-depth (16bit or 24bit or 32bit)
	// Correct increment formula would be: depthInc = (zfar - znear)/pow(2, bits)
	RLGH.currentBatch->currentDepth += (1.0f / 20000.0f);
}

// Define one vertex (position)
// NOTE: Vertex position data is the basic information required for drawing
void rlVertex3f(float x, float y, float z)
{
	float tx = x;
	float ty = y;
	float tz = z;

	// Transform provided vector if required
	if (RLGH.State.transformRequired)
	{
		const Matrix &transform = RLGH.State.transform;
		tx = transform.m0 * x + transform.m4 * y + transform.m8 * z + transform.m12;
		ty = transform.m1 * x + transform.m5 * y + transform.m9 * z + transform.m13;
		tz = transform.m2 * x + transform.m6 * y + transform.m10 * z + transform.m14;
	}

	rlDrawCall *drawcall = &RLGH.currentBatch->draws[RLGH.currentBatch->drawCounter - 1];
	rlVertexBuffer *vertex_buffer = &RLGH.currentBatch->vertexBuffer[RLGH.currentBatch->currentBuffer];
	int vertex_counter = RLGH.State.vertexCounter;
	int vertex_count = drawcall->vertexCount;

	// WARNING: We can't break primitives when launching a new batch.
	// RL_LINES comes in pairs, RL_TRIANGLES come in groups of 3 vertices and RL_QUADS come in groups of 4 vertices.
	// We must check current draw.mode when a new vertex is required and finish the batch only if the draw.mode draw.vertexCount is %2, %3 or %4
	if (vertex_counter > (vertex_buffer->elementCount * 4 - 4))
	{
		if ((drawcall->mode == RL_LINES) && (vertex_count % 2 == 0))
		{
			// Reached the maximum number of vertices for RL_LINES drawing
			// Launch a draw call but keep current state for next vertices comming
			// NOTE: We add +1 vertex to the check for security
			rlCheckRenderBatchLimit(2 + 1);
		} else if ((drawcall->mode == RL_TRIANGLES) && (vertex_count % 3 == 0))
		{
			rlCheckRenderBatchLimit(3 + 1);
		} else if ((drawcall->mode == RL_QUADS) && (vertex_count % 4 == 0))
		{
			rlCheckRenderBatchLimit(4 + 1);
		}

		drawcall = &RLGH.currentBatch->draws[RLGH.currentBatch->drawCounter - 1];
		vertex_buffer = &RLGH.currentBatch->vertexBuffer[RLGH.currentBatch->currentBuffer]; // TODO: Check me
		vertex_counter = RLGH.State.vertexCounter;
		vertex_count = drawcall->vertexCount;
	}

	// Add vertices
	vertex_buffer->vertices[3 * vertex_counter + 0] = tx;
	vertex_buffer->vertices[3 * vertex_counter + 1] = ty;
	vertex_buffer->vertices[3 * vertex_counter + 2] = tz;

	// Add current texcoord
	vertex_buffer->texcoords[2 * vertex_counter + 0] = RLGH.State.texcoordx;
	vertex_buffer->texcoords[2 * vertex_counter + 1] = RLGH.State.texcoordy;

	// WARNING: By default rlVertexBuffer struct does not store normals

	// Add current color
	vertex_buffer->colors[4 * vertex_counter + 0] = RLGH.State.colorr;
	vertex_buffer->colors[4 * vertex_counter + 1] = RLGH.State.colorg;
	vertex_buffer->colors[4 * vertex_counter + 2] = RLGH.State.colorb;
	vertex_buffer->colors[4 * vertex_counter + 3] = RLGH.State.colora;

	RLGH.State.vertexCounter++;
	drawcall->vertexCount++;
}

// Define one vertex (position)
void rlVertex2f(float x, float y)
{
	rlVertex3f(x, y, RLGH.currentBatch->currentDepth);
}

// Define one vertex (position)
void rlVertex2i(int x, int y)
{
	rlVertex3f((float)x, (float)y, RLGH.currentBatch->currentDepth);
}

// Define one vertex (texture coordinate)
// NOTE: Texture coordinates are limited to QUADS only
void rlTexCoord2f(float x, float y)
{
	RLGH.State.texcoordx = x;
	RLGH.State.texcoordy = y;
}

// Define one vertex (normal)
// NOTE: Normals limited to TRIANGLES only?
void rlNormal3f(float x, float y, float z)
{
	RLGH.State.normalx = x;
	RLGH.State.normaly = y;
	RLGH.State.normalz = z;
}

// Define one vertex (color)
void rlColor4ub(unsigned char x, unsigned char y, unsigned char z, unsigned char w)
{
	RLGH.State.colorr = x;
	RLGH.State.colorg = y;
	RLGH.State.colorb = z;
	RLGH.State.colora = w;
}

// Define one vertex (color)
void rlColor4f(float r, float g, float b, float a)
{
	rlColor4ub((unsigned char)(r * 255), (unsigned char)(g * 255), (unsigned char)(b * 255), (unsigned char)(a * 255));
}

// Define one vertex (color)
void rlColor3f(float x, float y, float z)
{
	rlColor4ub((unsigned char)(x * 255), (unsigned char)(y * 255), (unsigned char)(z * 255), 255);
}

//--------------------------------------------------------------------------------------
// Module Functions Definition - OpenGL style functions (common to 1.1, 3.3+, ES2)
//--------------------------------------------------------------------------------------

// Set current texture to use
void rlSetTexture(rlTexture *texture)
{
	if (texture == nullptr)
	{
		// NOTE: If quads batch limit is reached, we force a draw call and next batch starts
		if (RLGH.State.vertexCounter >= RLGH.currentBatch->vertexBuffer[RLGH.currentBatch->currentBuffer].elementCount * 4)
			rlDrawRenderBatch(RLGH.currentBatch);
	} else
	{
		rlDrawCall *drawcall = &RLGH.currentBatch->draws[RLGH.currentBatch->drawCounter - 1];
		if (drawcall->texture != *texture)
		{
			int vertex_count = drawcall->vertexCount;
			if (vertex_count > 0)
			{
				// Make sure current RLGH.currentBatch->draws[i].vertexCount is aligned a multiple of 4,
				// that way, following QUADS drawing will keep aligned with index processing
				// It implies adding some extra alignment vertex at the end of the draw,
				// those vertex are not processed but they are considered as an additional offset
				// for the next set of vertex to be drawn
				if (drawcall->mode == RL_LINES)
					drawcall->vertexAlignment = ((vertex_count < 4) ? vertex_count : vertex_count % 4);
				else if (drawcall->mode == RL_TRIANGLES)
					drawcall->vertexAlignment = ((vertex_count < 4) ? 1 : (4 - (vertex_count % 4)));
				else
					drawcall->vertexAlignment = 0;

				if (!rlCheckRenderBatchLimit(drawcall->vertexAlignment))
				{
					RLGH.State.vertexCounter += drawcall->vertexAlignment;

					RLGH.currentBatch->drawCounter++;
					drawcall = &RLGH.currentBatch->draws[RLGH.currentBatch->drawCounter - 1];
				}
			}

			if (RLGH.currentBatch->drawCounter >= RL_DEFAULT_BATCH_DRAWCALLS)
				rlDrawRenderBatch(RLGH.currentBatch);

			drawcall->texture = *texture;
			drawcall->vertexCount = 0;
		}
	}
}

// Select and active a texture slot
void rlActiveTextureSlot(int slot)
{
	glActiveTexture(GL_TEXTURE0 + slot);
}

// Enable texture
void rlEnableTexture(unsigned int id)
{
	glBindTexture(GL_TEXTURE_2D, id);
}

// Disable texture
void rlDisableTexture(void)
{
	glBindTexture(GL_TEXTURE_2D, 0);
}

// Enable texture cubemap
void rlEnableTextureCubemap(unsigned int id)
{
	glBindTexture(GL_TEXTURE_CUBE_MAP, id);
}

// Disable texture cubemap
void rlDisableTextureCubemap(void)
{
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

// Set texture parameters (wrap mode/filter mode)
void rlTextureParameters(unsigned int id, int param, int value)
{
	glBindTexture(GL_TEXTURE_2D, id);

	// Reset anisotropy filter, in case it was set
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.0f);

	switch (param)
	{
		case RL_TEXTURE_WRAP_S:
		case RL_TEXTURE_WRAP_T:
		{
			if (value == RL_TEXTURE_WRAP_MIRROR_CLAMP)
				glTexParameteri(GL_TEXTURE_2D, param, value);
			else
				glTexParameteri(GL_TEXTURE_2D, param, value);

		} break;
		case RL_TEXTURE_MAG_FILTER:
		case RL_TEXTURE_MIN_FILTER: glTexParameteri(GL_TEXTURE_2D, param, value); break;
		case RL_TEXTURE_FILTER_ANISOTROPIC:
		{
			if (value <= RLGH.ExtSupported.maxAnisotropyLevel)
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, (float)value);

			else if (RLGH.ExtSupported.maxAnisotropyLevel > 0.0f)
			{
				TRACELOG(RL_LOG_WARNING, "GL: Maximum anisotropic filter level supported is %iX", id, (int)RLGH.ExtSupported.maxAnisotropyLevel);
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, (float)value);
			} else
				TRACELOG(RL_LOG_WARNING, "GL: Anisotropic filtering not supported");
		} break;
		case RL_TEXTURE_MIPMAP_BIAS_RATIO: glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, value / 100.0f);
		default: break;
	}

	glBindTexture(GL_TEXTURE_2D, 0);
}

// Set cubemap parameters (wrap mode/filter mode)
void rlCubemapParameters(unsigned int id, int param, int value)
{
	glBindTexture(GL_TEXTURE_CUBE_MAP, id);

	// Reset anisotropy filter, in case it was set
	glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.0f);

	switch (param)
	{
		case RL_TEXTURE_WRAP_S:
		case RL_TEXTURE_WRAP_T:
		{
			if (value == RL_TEXTURE_WRAP_MIRROR_CLAMP)
			{
				glTexParameteri(GL_TEXTURE_CUBE_MAP, param, value);
			} else glTexParameteri(GL_TEXTURE_CUBE_MAP, param, value);

		} break;
		case RL_TEXTURE_MAG_FILTER:
		case RL_TEXTURE_MIN_FILTER: glTexParameteri(GL_TEXTURE_CUBE_MAP, param, value); break;
		case RL_TEXTURE_FILTER_ANISOTROPIC:
		{
			if (value <= RLGH.ExtSupported.maxAnisotropyLevel) glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_ANISOTROPY_EXT, (float)value);
			else if (RLGH.ExtSupported.maxAnisotropyLevel > 0.0f)
			{
				TRACELOG(RL_LOG_WARNING, "GL: Maximum anisotropic filter level supported is %iX", id, (int)RLGH.ExtSupported.maxAnisotropyLevel);
				glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_ANISOTROPY_EXT, (float)value);
			} else TRACELOG(RL_LOG_WARNING, "GL: Anisotropic filtering not supported");
		} break;
		case RL_TEXTURE_MIPMAP_BIAS_RATIO: glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_LOD_BIAS, value / 100.0f);
		default: break;
	}

	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

// Enable shader program
void rlEnableShader(unsigned int id)
{
	glUseProgram(id);
}

// Disable shader program
void rlDisableShader(void)
{
	glUseProgram(0);
}

// Enable rendering to texture (fbo)
void rlEnableFramebuffer(unsigned int id)
{
	glBindFramebuffer(GL_FRAMEBUFFER, id);
}

// Disable rendering to texture
void rlDisableFramebuffer(void)
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// Activate multiple draw color buffers
// NOTE: One color buffer is always active by default
void rlActiveDrawBuffers(int count)
{
	// NOTE: Maximum number of draw buffers supported is implementation dependant,
	// it can be queried with glGet*() but it must be at least 8
	//GLint maxDrawBuffers = 0;
	//glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxDrawBuffers);

	if (count > 0)
	{
		if (count > 8)
			TRACELOG(LOG_WARNING, "GL: Max color buffers limited to 8");
		else
		{
			unsigned int buffers[8] = {
				GL_COLOR_ATTACHMENT0,
				GL_COLOR_ATTACHMENT1,
				GL_COLOR_ATTACHMENT2,
				GL_COLOR_ATTACHMENT3,
				GL_COLOR_ATTACHMENT4,
				GL_COLOR_ATTACHMENT5,
				GL_COLOR_ATTACHMENT6,
				GL_COLOR_ATTACHMENT7,
			};

			glDrawBuffers(count, buffers);
		}
	} else
		TRACELOG(LOG_WARNING, "GL: One color buffer active by default");
}

//----------------------------------------------------------------------------------
// General render state configuration
//----------------------------------------------------------------------------------

// Enable color blending
void rlEnableColorBlend(void) { glEnable(GL_BLEND); }

// Disable color blending
void rlDisableColorBlend(void) { glDisable(GL_BLEND); }

// Enable depth test
void rlEnableDepthTest(void) { glEnable(GL_DEPTH_TEST); }

// Disable depth test
void rlDisableDepthTest(void) { glDisable(GL_DEPTH_TEST); }

// Enable depth write
void rlEnableDepthMask(void) { glDepthMask(GL_TRUE); }

// Disable depth write
void rlDisableDepthMask(void) { glDepthMask(GL_FALSE); }

// Enable backface culling
void rlEnableBackfaceCulling(void) { glEnable(GL_CULL_FACE); }

// Disable backface culling
void rlDisableBackfaceCulling(void) { glDisable(GL_CULL_FACE); }

// Set face culling mode
void rlSetCullFace(int mode)
{
	switch (mode)
	{
		case RL_CULL_FACE_BACK: glCullFace(GL_BACK); break;
		case RL_CULL_FACE_FRONT: glCullFace(GL_FRONT); break;
		default: break;
	}
}

// Enable scissor test
void rlEnableScissorTest(void) { glEnable(GL_SCISSOR_TEST); }

// Disable scissor test
void rlDisableScissorTest(void) { glDisable(GL_SCISSOR_TEST); }

// Scissor test
void rlScissor(int x, int y, int width, int height) { glScissor(x, y, width, height); }

// Enable wire mode
void rlEnableWireMode(void)
{
	// NOTE: glPolygonMode() not available on OpenGL ES
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
}

// Disable wire mode
void rlDisableWireMode(void)
{
	// NOTE: glPolygonMode() not available on OpenGL ES
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

// Set the line drawing width
void rlSetLineWidth(float width) { glLineWidth(width); }

// Get the line drawing width
float rlGetLineWidth(void)
{
	float width = 0;
	glGetFloatv(GL_LINE_WIDTH, &width);
	return width;
}

// Enable line aliasing
void rlEnableSmoothLines(void)
{
	glEnable(GL_LINE_SMOOTH);
}

// Disable line aliasing
void rlDisableSmoothLines(void)
{
	glDisable(GL_LINE_SMOOTH);
}

// Enable stereo rendering
void rlEnableStereoRender(void)
{
	RLGH.State.stereoRender = true;
}

// Disable stereo rendering
void rlDisableStereoRender(void)
{
	RLGH.State.stereoRender = false;
}

// Check if stereo render is enabled
bool rlIsStereoRenderEnabled(void)
{
	return RLGH.State.stereoRender;
}

// Clear color buffer with color
void rlClearColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
	// Color values clamp to 0.0f(0) and 1.0f(255)
	float cr = (float)r / 255;
	float cg = (float)g / 255;
	float cb = (float)b / 255;
	float ca = (float)a / 255;

	glClearColor(cr, cg, cb, ca);
}

// Clear used screen buffers (color and depth)
void rlClearScreenBuffers(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);     // Clear used buffers: Color and Depth (Depth is used for 3D)
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);     // Stencil buffer not used...
}

// Check and log OpenGL error codes
void rlCheckErrors()
{
	//int check = 1;
	//while (check)
	//{
	//    const GLenum err = glGetError();
	//    switch (err)
	//    {
	//        case GL_NO_ERROR: check = 0; break;
	//        case 0x0500: TRACELOG(RL_LOG_WARNING, "GL: Error detected: GL_INVALID_ENUM"); break;
	//        case 0x0501: TRACELOG(RL_LOG_WARNING, "GL: Error detected: GL_INVALID_VALUE"); break;
	//        case 0x0502: TRACELOG(RL_LOG_WARNING, "GL: Error detected: GL_INVALID_OPERATION"); break;
	//        case 0x0503: TRACELOG(RL_LOG_WARNING, "GL: Error detected: GL_STACK_OVERFLOW"); break;
	//        case 0x0504: TRACELOG(RL_LOG_WARNING, "GL: Error detected: GL_STACK_UNDERFLOW"); break;
	//        case 0x0505: TRACELOG(RL_LOG_WARNING, "GL: Error detected: GL_OUT_OF_MEMORY"); break;
	//        case 0x0506: TRACELOG(RL_LOG_WARNING, "GL: Error detected: GL_INVALID_FRAMEBUFFER_OPERATION"); break;
	//        default: TRACELOG(RL_LOG_WARNING, "GL: Error detected: Unknown error code: %x", err); break;
	//    }
	//}
}

// Set blend mode
void rlSetBlendMode(int mode)
{
	if ((RLGH.State.currentBlendMode != mode) || ((mode == RL_BLEND_CUSTOM || mode == RL_BLEND_CUSTOM_SEPARATE) && RLGH.State.glCustomBlendModeModified))
	{
		rlDrawRenderBatch(RLGH.currentBatch);

		switch (mode)
		{
			case RL_BLEND_ALPHA: glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); glBlendEquation(GL_FUNC_ADD); break;
			case RL_BLEND_ADDITIVE: glBlendFunc(GL_SRC_ALPHA, GL_ONE); glBlendEquation(GL_FUNC_ADD); break;
			case RL_BLEND_MULTIPLIED: glBlendFunc(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA); glBlendEquation(GL_FUNC_ADD); break;
			case RL_BLEND_ADD_COLORS: glBlendFunc(GL_ONE, GL_ONE); glBlendEquation(GL_FUNC_ADD); break;
			case RL_BLEND_SUBTRACT_COLORS: glBlendFunc(GL_ONE, GL_ONE); glBlendEquation(GL_FUNC_SUBTRACT); break;
			case RL_BLEND_ALPHA_PREMULTIPLY: glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); glBlendEquation(GL_FUNC_ADD); break;
			case RL_BLEND_CUSTOM:
			{
				// NOTE: Using GL blend src/dst factors and GL equation configured with rlSetBlendFactors()
				glBlendFunc(RLGH.State.glBlendSrcFactor, RLGH.State.glBlendDstFactor); glBlendEquation(RLGH.State.glBlendEquation);

			} break;
			case RL_BLEND_CUSTOM_SEPARATE:
			{
				// NOTE: Using GL blend src/dst factors and GL equation configured with rlSetBlendFactorsSeparate()
				glBlendFuncSeparate(RLGH.State.glBlendSrcFactorRGB, RLGH.State.glBlendDestFactorRGB, RLGH.State.glBlendSrcFactorAlpha, RLGH.State.glBlendDestFactorAlpha);
				glBlendEquationSeparate(RLGH.State.glBlendEquationRGB, RLGH.State.glBlendEquationAlpha);

			} break;
			default: break;
		}

		RLGH.State.currentBlendMode = mode;
		RLGH.State.glCustomBlendModeModified = false;
	}
}

// Set blending mode factor and equation
void rlSetBlendFactors(int glSrcFactor, int glDstFactor, int glEquation)
{
	if ((RLGH.State.glBlendSrcFactor != glSrcFactor) || (RLGH.State.glBlendDstFactor != glDstFactor) || (RLGH.State.glBlendEquation != glEquation))
	{
		RLGH.State.glBlendSrcFactor = glSrcFactor;
		RLGH.State.glBlendDstFactor = glDstFactor;
		RLGH.State.glBlendEquation = glEquation;

		RLGH.State.glCustomBlendModeModified = true;
	}
}

// Set blending mode factor and equation separately for RGB and alpha
void rlSetBlendFactorsSeparate(int glSrcRGB, int glDstRGB, int glSrcAlpha, int glDstAlpha, int glEqRGB, int glEqAlpha)
{
	if ((RLGH.State.glBlendSrcFactorRGB != glSrcRGB) || (RLGH.State.glBlendDestFactorRGB != glDstRGB) ||
		(RLGH.State.glBlendSrcFactorAlpha != glSrcAlpha) || (RLGH.State.glBlendDestFactorAlpha != glDstAlpha) ||
		(RLGH.State.glBlendEquationRGB != glEqRGB) || (RLGH.State.glBlendEquationAlpha != glEqAlpha))
	{
		RLGH.State.glBlendSrcFactorRGB = glSrcRGB;
		RLGH.State.glBlendDestFactorRGB = glDstRGB;
		RLGH.State.glBlendSrcFactorAlpha = glSrcAlpha;
		RLGH.State.glBlendDestFactorAlpha = glDstAlpha;
		RLGH.State.glBlendEquationRGB = glEqRGB;
		RLGH.State.glBlendEquationAlpha = glEqAlpha;

		RLGH.State.glCustomBlendModeModified = true;
	}
}

//----------------------------------------------------------------------------------
// Module Functions Definition - OpenGL Debug
//----------------------------------------------------------------------------------
#if defined(RLGL_ENABLE_OPENGL_DEBUG_CONTEXT)
static void GLAPIENTRY rlDebugMessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
{
	// Ignore non-significant error/warning codes (NVidia drivers)
	// NOTE: Here there are the details with a sample output:
	// - #131169 - Framebuffer detailed info: The driver allocated storage for renderbuffer 2. (severity: low)
	// - #131185 - Buffer detailed info: Buffer object 1 (bound to GL_ELEMENT_ARRAY_BUFFER_ARB, usage hint is GL_ENUM_88e4)
	//             will use VIDEO memory as the source for buffer object operations. (severity: low)
	// - #131218 - Program/shader state performance warning: Vertex shader in program 7 is being recompiled based on GL state. (severity: medium)
	// - #131204 - Texture state usage warning: The texture object (0) bound to texture image unit 0 does not have
	//             a defined base level and cannot be used for texture mapping. (severity: low)
	if ((id == 131169) || (id == 131185) || (id == 131218) || (id == 131204)) return;

	const char *msgSource = NULL;
	switch (source)
	{
		case GL_DEBUG_SOURCE_API: msgSource = "API"; break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM: msgSource = "WINDOW_SYSTEM"; break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER: msgSource = "SHADER_COMPILER"; break;
		case GL_DEBUG_SOURCE_THIRD_PARTY: msgSource = "THIRD_PARTY"; break;
		case GL_DEBUG_SOURCE_APPLICATION: msgSource = "APPLICATION"; break;
		case GL_DEBUG_SOURCE_OTHER: msgSource = "OTHER"; break;
		default: break;
	}

	const char *msgType = NULL;
	switch (type)
	{
		case GL_DEBUG_TYPE_ERROR: msgType = "ERROR"; break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: msgType = "DEPRECATED_BEHAVIOR"; break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: msgType = "UNDEFINED_BEHAVIOR"; break;
		case GL_DEBUG_TYPE_PORTABILITY: msgType = "PORTABILITY"; break;
		case GL_DEBUG_TYPE_PERFORMANCE: msgType = "PERFORMANCE"; break;
		case GL_DEBUG_TYPE_MARKER: msgType = "MARKER"; break;
		case GL_DEBUG_TYPE_PUSH_GROUP: msgType = "PUSH_GROUP"; break;
		case GL_DEBUG_TYPE_POP_GROUP: msgType = "POP_GROUP"; break;
		case GL_DEBUG_TYPE_OTHER: msgType = "OTHER"; break;
		default: break;
	}

	const char *msgSeverity = "DEFAULT";
	switch (severity)
	{
		case GL_DEBUG_SEVERITY_LOW: msgSeverity = "LOW"; break;
		case GL_DEBUG_SEVERITY_MEDIUM: msgSeverity = "MEDIUM"; break;
		case GL_DEBUG_SEVERITY_HIGH: msgSeverity = "HIGH"; break;
		case GL_DEBUG_SEVERITY_NOTIFICATION: msgSeverity = "NOTIFICATION"; break;
		default: break;
	}

	TRACELOG(LOG_WARNING, "GL: OpenGL debug message: %s", message);
	TRACELOG(LOG_WARNING, "    > Type: %s", msgType);
	TRACELOG(LOG_WARNING, "    > Source = %s", msgSource);
	TRACELOG(LOG_WARNING, "    > Severity = %s", msgSeverity);
}
#endif

//----------------------------------------------------------------------------------
// Module Functions Definition - rlgl functionality
//----------------------------------------------------------------------------------

// Initialize rlgl: OpenGL extensions, default buffers/shaders/textures, OpenGL states
void rlglInit(int width, int height)
{
	UINT flags = 0;

	// Enable OpenGL debug context if required
	#if defined(RLGL_ENABLE_OPENGL_DEBUG_CONTEXT)
	if ((glDebugMessageCallback != NULL) && (glDebugMessageControl != NULL))
	{
		//glDebugMessageCallback(rlDebugMessageCallback, 0);
		//// glDebugMessageControl(GL_DEBUG_SOURCE_API, GL_DEBUG_TYPE_ERROR, GL_DEBUG_SEVERITY_HIGH, 0, 0, GL_TRUE);
		//
		//// Debug context options:
		////  - GL_DEBUG_OUTPUT - Faster version but not useful for breakpoints
		////  - GL_DEBUG_OUTPUT_SYNCHRONUS - Callback is in sync with errors, so a breakpoint can be placed on the callback in order to get a stacktrace for the GL error
		//glEnable(GL_DEBUG_OUTPUT);
		//glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

		flags |= D3D11_CREATE_DEVICE_DEBUG;
	}
	#endif

	// Init default white texture
	unsigned char pixels[4] = { 255, 255, 255, 255 };   // 1 pixel RGBA (4 bytes)
	RLGH.State.defaultTexture = rlLoadTexture(pixels, 1, 1, RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, 1);

	if (RLGH.State.defaultTexture != 0)
		TRACELOG(RL_LOG_INFO, "TEXTURE: [ID %i] Default texture loaded successfully", RLGH.State.defaultTexture);
	else
		TRACELOG(RL_LOG_WARNING, "TEXTURE: Failed to load default texture");

	// Init default Shader (customized for GL 3.3 and ES2)
	// Loaded: RLGH.State.defaultShaderId + RLGH.State.defaultShaderLocs
	rlLoadShaderDefault();
	RLGH.State.currentShaderId = RLGH.State.defaultShaderId;
	RLGH.State.currentShaderLocs = RLGH.State.defaultShaderLocs;

	// Init default vertex arrays buffers
	RLGH.defaultBatch = rlLoadRenderBatch(RL_DEFAULT_BATCH_BUFFERS, RL_DEFAULT_BATCH_BUFFER_ELEMENTS);
	RLGH.currentBatch = &RLGH.defaultBatch;

	// Init stack matrices (emulating OpenGL 1.1)
	for (int i = 0; i < RL_MAX_MATRIX_STACK_SIZE; i++) RLGH.State.stack[i] = rlMatrixIdentity();

	// Init internal matrices
	RLGH.State.transform = rlMatrixIdentity();
	RLGH.State.projection = rlMatrixIdentity();
	RLGH.State.modelview = rlMatrixIdentity();
	RLGH.State.currentMatrix = &RLGH.State.modelview;

	// Initialize OpenGL default states
	//----------------------------------------------------------
	// Init state: Depth test
	glDepthFunc(GL_LEQUAL);                                 // Type of depth testing to apply
	glDisable(GL_DEPTH_TEST);                               // Disable depth testing for 2D (only used for 3D)

	// Init state: Blending mode
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);      // Color blending function (how colors are mixed)
	glEnable(GL_BLEND);                                     // Enable color blending (required to work with transparencies)

	// Init state: Culling
	// NOTE: All shapes/models triangles are drawn CCW
	glCullFace(GL_BACK);                                    // Cull the back face (default)
	glFrontFace(GL_CCW);                                    // Front face are defined counter clockwise (default)
	glEnable(GL_CULL_FACE);                                 // Enable backface culling

	// Init state: Cubemap seamless
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);                 // Seamless cubemaps (not supported on OpenGL ES 2.0)

	// Store screen size into global variables
	RLGH.State.framebufferWidth = width;
	RLGH.State.framebufferHeight = height;

	TRACELOG(RL_LOG_INFO, "RLGH: Default OpenGL state initialized successfully");
	//----------------------------------------------------------

	// Init state: Color/Depth buffers clear
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);                   // Set clear color (black)
	glClearDepth(1.0f);                                     // Set clear depth value (default)
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);     // Clear color and depth buffers (depth buffer required for 3D)
}

// Vertex Buffer Object deinitialization (memory free)
void rlglClose(void)
{
	rlUnloadRenderBatch(RLGH.defaultBatch);

	rlUnloadShaderDefault();          // Unload default shader

	glDeleteTextures(1, &RLGH.State.defaultTexture); // Unload default texture
	TRACELOG(RL_LOG_INFO, "TEXTURE: [ID %i] Default texture unloaded successfully", RLGH.State.defaultTexture);
}

// Load OpenGL extensions
// NOTE: External loader function must be provided
void rlLoadExtensions(void *loader)
{
	const D3D_FEATURE_LEVEL feature_levels[] = { D3D_FEATURE_LEVEL_11_0 };
	if (D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, feature_levels, 1, D3D11_SDK_VERSION, &RLGH.device, &RLGH.feature_level, &RLGH.context) != S_OK)
		TRACELOG(RL_LOG_FATAL, "rlglInit(): Failed to initialize D3D11\n");

	// NOTE: glad is generated and contains only required OpenGL 3.3 Core extensions (and lower versions)
	if (gladLoadGL((GLADloadfunc)loader) == 0)
		TRACELOG(RL_LOG_WARNING, "GLAD: Cannot load OpenGL extensions");
	else
		TRACELOG(RL_LOG_INFO, "GLAD: OpenGL extensions loaded successfully");

	// Get number of supported extensions
	GLint numExt = 0;
	glGetIntegerv(GL_NUM_EXTENSIONS, &numExt);
	TRACELOG(RL_LOG_INFO, "GL: Supported extensions count: %i", numExt);

	#if defined(RLGL_SHOW_GL_DETAILS_INFO)
	// Get supported extensions list
	// WARNING: glGetStringi() not available on OpenGL 2.1
	TRACELOG(RL_LOG_INFO, "GL: OpenGL extensions:");
	for (int i = 0; i < numExt; i++) TRACELOG(RL_LOG_INFO, "    %s", glGetStringi(GL_EXTENSIONS, i));
	#endif

	// Register supported extensions flags
	// OpenGL 3.3 extensions supported by default (core)
	RLGH.ExtSupported.texFloat32 = true;
	RLGH.ExtSupported.maxDepthBits = 32;

	// Optional OpenGL 3.3 extensions
	RLGH.ExtSupported.texCompASTC = GLAD_GL_KHR_texture_compression_astc_hdr && GLAD_GL_KHR_texture_compression_astc_ldr;
	RLGH.ExtSupported.texCompDXT = GLAD_GL_EXT_texture_compression_s3tc;  // Texture compression: DXT
	RLGH.ExtSupported.texCompETC2 = GLAD_GL_ARB_ES3_compatibility;        // Texture compression: ETC2/EAC
	RLGH.ExtSupported.computeShader = GLAD_GL_ARB_compute_shader;

	// Check OpenGL information and capabilities
	//------------------------------------------------------------------------------
	// Show current OpenGL and GLSL version
	TRACELOG(RL_LOG_INFO, "GL: OpenGL device information:");
	TRACELOG(RL_LOG_INFO, "    > Vendor:   %s", glGetString(GL_VENDOR));
	TRACELOG(RL_LOG_INFO, "    > Renderer: %s", glGetString(GL_RENDERER));
	TRACELOG(RL_LOG_INFO, "    > Version:  %s", glGetString(GL_VERSION));
	TRACELOG(RL_LOG_INFO, "    > GLSL:     %s", glGetString(GL_SHADING_LANGUAGE_VERSION));

	// NOTE: Anisotropy levels capability is an extension
	#ifndef GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT
	#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
	#endif
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &RLGH.ExtSupported.maxAnisotropyLevel);

	#if defined(RLGL_SHOW_GL_DETAILS_INFO)
	// Show some OpenGL GPU capabilities
	TRACELOG(RL_LOG_INFO, "GL: OpenGL capabilities:");
	GLint capability = 0;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &capability);
	TRACELOG(RL_LOG_INFO, "    GL_MAX_TEXTURE_SIZE: %i", capability);
	glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE, &capability);
	TRACELOG(RL_LOG_INFO, "    GL_MAX_CUBE_MAP_TEXTURE_SIZE: %i", capability);
	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &capability);
	TRACELOG(RL_LOG_INFO, "    GL_MAX_TEXTURE_IMAGE_UNITS: %i", capability);
	glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &capability);
	TRACELOG(RL_LOG_INFO, "    GL_MAX_VERTEX_ATTRIBS: %i", capability);
	glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &capability);
	TRACELOG(RL_LOG_INFO, "    GL_MAX_UNIFORM_BLOCK_SIZE: %i", capability);
	glGetIntegerv(GL_MAX_DRAW_BUFFERS, &capability);
	TRACELOG(RL_LOG_INFO, "    GL_MAX_DRAW_BUFFERS: %i", capability);
	if (RLGH.ExtSupported.texAnisoFilter) TRACELOG(RL_LOG_INFO, "    GL_MAX_TEXTURE_MAX_ANISOTROPY: %.0f", RLGH.ExtSupported.maxAnisotropyLevel);
	glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS, &capability);
	TRACELOG(RL_LOG_INFO, "    GL_NUM_COMPRESSED_TEXTURE_FORMATS: %i", capability);
	GLint *compFormats = (GLint *)RL_CALLOC(capability, sizeof(GLint));
	glGetIntegerv(GL_COMPRESSED_TEXTURE_FORMATS, compFormats);
	for (int i = 0; i < capability; i++) TRACELOG(RL_LOG_INFO, "        %s", rlGetCompressedFormatName(compFormats[i]));
	RL_FREE(compFormats);

	glGetIntegerv(GL_MAX_VERTEX_ATTRIB_BINDINGS, &capability);
	TRACELOG(RL_LOG_INFO, "    GL_MAX_VERTEX_ATTRIB_BINDINGS: %i", capability);
	glGetIntegerv(GL_MAX_UNIFORM_LOCATIONS, &capability);
	TRACELOG(RL_LOG_INFO, "    GL_MAX_UNIFORM_LOCATIONS: %i", capability);
	#else   // RLGL_SHOW_GL_DETAILS_INFO

	// Show some basic info about GL supported features
	if (RLGH.ExtSupported.texCompDXT) TRACELOG(RL_LOG_INFO, "GL: DXT compressed textures supported");
	if (RLGH.ExtSupported.texCompETC1) TRACELOG(RL_LOG_INFO, "GL: ETC1 compressed textures supported");
	if (RLGH.ExtSupported.texCompETC2) TRACELOG(RL_LOG_INFO, "GL: ETC2/EAC compressed textures supported");
	if (RLGH.ExtSupported.texCompPVRT) TRACELOG(RL_LOG_INFO, "GL: PVRT compressed textures supported");
	if (RLGH.ExtSupported.texCompASTC) TRACELOG(RL_LOG_INFO, "GL: ASTC compressed textures supported");
	if (RLGH.ExtSupported.computeShader) TRACELOG(RL_LOG_INFO, "GL: Compute shaders supported");
	#endif  // RLGL_SHOW_GL_DETAILS_INFO
}

// Get current API
int rlGetVersion(void)
{
	return RL_DIRECT3D_11_0;
}

// Set current framebuffer width
void rlSetFramebufferWidth(int width)
{
	RLGH.State.framebufferWidth = width;
}

// Set current framebuffer height
void rlSetFramebufferHeight(int height)
{
	RLGH.State.framebufferHeight = height;
}

// Get default framebuffer width
int rlGetFramebufferWidth(void)
{
	return RLGH.State.framebufferWidth;
}

// Get default framebuffer height
int rlGetFramebufferHeight(void)
{
	return RLGH.State.framebufferHeight;
}

// Get default internal texture (white texture)
// NOTE: Default texture is a 1x1 pixel UNCOMPRESSED_R8G8B8A8
unsigned int rlGetTextureIdDefault(void)
{
	return RLGH.State.defaultTexture;
}

// Get default shader id
unsigned int rlGetShaderIdDefault(void)
{
	return RLGH.State.defaultShaderId;
}

// Get default shader locs
int *rlGetShaderLocsDefault(void)
{
	return RLGH.State.defaultShaderLocs;
}

// Render batch management
//------------------------------------------------------------------------------------------------
// Load render batch
rlRenderBatch rlLoadRenderBatch(int numBuffers, int bufferElements)
{
	rlRenderBatch batch = { 0 };

	// Initialize CPU (RAM) vertex buffers (position, texcoord, color data and indexes)
	//--------------------------------------------------------------------------------------------
	batch.vertexBuffer = (rlVertexBuffer *)RL_MALLOC(numBuffers * sizeof(rlVertexBuffer));

	for (int i = 0; i < numBuffers; i++)
	{
		batch.vertexBuffer[i].elementCount = bufferElements;

		batch.vertexBuffer[i].vertices = (float *)RL_MALLOC(bufferElements * 3 * 4 * sizeof(float));        // 3 float by vertex, 4 vertex by quad
		batch.vertexBuffer[i].texcoords = (float *)RL_MALLOC(bufferElements * 2 * 4 * sizeof(float));       // 2 float by texcoord, 4 texcoord by quad
		batch.vertexBuffer[i].colors = (unsigned char *)RL_MALLOC(bufferElements * 4 * 4 * sizeof(unsigned char));   // 4 float by color, 4 colors by quad
		batch.vertexBuffer[i].indices = (unsigned int *)RL_MALLOC(bufferElements * 6 * sizeof(unsigned int));      // 6 int by quad (indices)

		for (int j = 0; j < (3 * 4 * bufferElements); j++) batch.vertexBuffer[i].vertices[j] = 0.0f;
		for (int j = 0; j < (2 * 4 * bufferElements); j++) batch.vertexBuffer[i].texcoords[j] = 0.0f;
		for (int j = 0; j < (4 * 4 * bufferElements); j++) batch.vertexBuffer[i].colors[j] = 0;

		int k = 0;

		// Indices can be initialized right now
		for (int j = 0; j < (6 * bufferElements); j += 6)
		{
			batch.vertexBuffer[i].indices[j] = 4 * k;
			batch.vertexBuffer[i].indices[j + 1] = 4 * k + 1;
			batch.vertexBuffer[i].indices[j + 2] = 4 * k + 2;
			batch.vertexBuffer[i].indices[j + 3] = 4 * k;
			batch.vertexBuffer[i].indices[j + 4] = 4 * k + 2;
			batch.vertexBuffer[i].indices[j + 5] = 4 * k + 3;

			k++;
		}

		RLGH.State.vertexCounter = 0;
	}

	TRACELOG(RL_LOG_INFO, "RLGH: Render batch vertex buffers loaded successfully in RAM (CPU)");
	//--------------------------------------------------------------------------------------------

	// Upload to GPU (VRAM) vertex data and initialize VAOs/VBOs
	//--------------------------------------------------------------------------------------------
	for (int i = 0; i < numBuffers; i++)
	{
		// Initialize Quads VAO
		glGenVertexArrays(1, &batch.vertexBuffer[i].vaoId);
		glBindVertexArray(batch.vertexBuffer[i].vaoId);

		// Quads - Vertex buffers binding and attributes enable
		// Vertex position buffer (shader-location = 0)
		glGenBuffers(1, &batch.vertexBuffer[i].vboId[0]);
		glBindBuffer(GL_ARRAY_BUFFER, batch.vertexBuffer[i].vboId[0]);
		glBufferData(GL_ARRAY_BUFFER, bufferElements * 3 * 4 * sizeof(float), batch.vertexBuffer[i].vertices, GL_DYNAMIC_DRAW);
		glEnableVertexAttribArray(RLGH.State.currentShaderLocs[RL_SHADER_LOC_VERTEX_POSITION]);
		glVertexAttribPointer(RLGH.State.currentShaderLocs[RL_SHADER_LOC_VERTEX_POSITION], 3, GL_FLOAT, 0, 0, 0);

		// Vertex texcoord buffer (shader-location = 1)
		glGenBuffers(1, &batch.vertexBuffer[i].vboId[1]);
		glBindBuffer(GL_ARRAY_BUFFER, batch.vertexBuffer[i].vboId[1]);
		glBufferData(GL_ARRAY_BUFFER, bufferElements * 2 * 4 * sizeof(float), batch.vertexBuffer[i].texcoords, GL_DYNAMIC_DRAW);
		glEnableVertexAttribArray(RLGH.State.currentShaderLocs[RL_SHADER_LOC_VERTEX_TEXCOORD01]);
		glVertexAttribPointer(RLGH.State.currentShaderLocs[RL_SHADER_LOC_VERTEX_TEXCOORD01], 2, GL_FLOAT, 0, 0, 0);

		// Vertex color buffer (shader-location = 3)
		glGenBuffers(1, &batch.vertexBuffer[i].vboId[2]);
		glBindBuffer(GL_ARRAY_BUFFER, batch.vertexBuffer[i].vboId[2]);
		glBufferData(GL_ARRAY_BUFFER, bufferElements * 4 * 4 * sizeof(unsigned char), batch.vertexBuffer[i].colors, GL_DYNAMIC_DRAW);
		glEnableVertexAttribArray(RLGH.State.currentShaderLocs[RL_SHADER_LOC_VERTEX_COLOR]);
		glVertexAttribPointer(RLGH.State.currentShaderLocs[RL_SHADER_LOC_VERTEX_COLOR], 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, 0);

		// Fill index buffer
		glGenBuffers(1, &batch.vertexBuffer[i].vboId[3]);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, batch.vertexBuffer[i].vboId[3]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, bufferElements * 6 * sizeof(int), batch.vertexBuffer[i].indices, GL_STATIC_DRAW);
	}

	TRACELOG(RL_LOG_INFO, "RLGH: Render batch vertex buffers loaded successfully in VRAM (GPU)");

	// Unbind the current VAO
	glBindVertexArray(0);
	//--------------------------------------------------------------------------------------------

	// Init draw calls tracking system
	//--------------------------------------------------------------------------------------------
	batch.draws = (rlDrawCall *)RL_MALLOC(RL_DEFAULT_BATCH_DRAWCALLS * sizeof(rlDrawCall));

	for (int i = 0; i < RL_DEFAULT_BATCH_DRAWCALLS; i++)
	{
		batch.draws[i].mode = RL_QUADS;
		batch.draws[i].vertexCount = 0;
		batch.draws[i].vertexAlignment = 0;
		//batch.draws[i].vaoId = 0;
		//batch.draws[i].shaderId = 0;
		batch.draws[i].textureId = RLGH.State.defaultTexture;
		//batch.draws[i].RLGH.State.projection = rlMatrixIdentity();
		//batch.draws[i].RLGH.State.modelview = rlMatrixIdentity();
	}

	batch.bufferCount = numBuffers;    // Record buffer count
	batch.drawCounter = 1;             // Reset draws counter
	batch.currentDepth = -1.0f;         // Reset depth value
	//--------------------------------------------------------------------------------------------

	return batch;
}

// Unload default internal buffers vertex data from CPU and GPU
void rlUnloadRenderBatch(rlRenderBatch batch)
{
	// Unbind everything
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	// Unload all vertex buffers data
	for (int i = 0; i < batch.bufferCount; i++)
	{
		// Unbind VAO attribs data
		glBindVertexArray(batch.vertexBuffer[i].vaoId);
		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);
		glDisableVertexAttribArray(3);
		glBindVertexArray(0);

		// Delete VBOs from GPU (VRAM)
		glDeleteBuffers(1, &batch.vertexBuffer[i].vboId[0]);
		glDeleteBuffers(1, &batch.vertexBuffer[i].vboId[1]);
		glDeleteBuffers(1, &batch.vertexBuffer[i].vboId[2]);
		glDeleteBuffers(1, &batch.vertexBuffer[i].vboId[3]);

		// Delete VAOs from GPU (VRAM)
		glDeleteVertexArrays(1, &batch.vertexBuffer[i].vaoId);

		// Free vertex arrays memory from CPU (RAM)
		RL_FREE(batch.vertexBuffer[i].vertices);
		RL_FREE(batch.vertexBuffer[i].texcoords);
		RL_FREE(batch.vertexBuffer[i].colors);
		RL_FREE(batch.vertexBuffer[i].indices);
	}

	// Unload arrays
	RL_FREE(batch.vertexBuffer);
	RL_FREE(batch.draws);
}

// Draw render batch
// NOTE: We require a pointer to reset batch and increase current buffer (multi-buffer)
void rlDrawRenderBatch(rlRenderBatch *batch)
{
	// Update batch vertex buffers
	//------------------------------------------------------------------------------------------------------------
	// NOTE: If there is not vertex data, buffers doesn't need to be updated (vertexCount > 0)
	// TODO: If no data changed on the CPU arrays --> No need to re-update GPU arrays (use a change detector flag?)
	if (RLGH.State.vertexCounter > 0)
	{
		// Activate elements VAO
		glBindVertexArray(batch->vertexBuffer[batch->currentBuffer].vaoId);

		// Vertex positions buffer
		glBindBuffer(GL_ARRAY_BUFFER, batch->vertexBuffer[batch->currentBuffer].vboId[0]);
		glBufferSubData(GL_ARRAY_BUFFER, 0, RLGH.State.vertexCounter * 3 * sizeof(float), batch->vertexBuffer[batch->currentBuffer].vertices);
		//glBufferData(GL_ARRAY_BUFFER, sizeof(float)*3*4*batch->vertexBuffer[batch->currentBuffer].elementCount, batch->vertexBuffer[batch->currentBuffer].vertices, GL_DYNAMIC_DRAW);  // Update all buffer

		// Texture coordinates buffer
		glBindBuffer(GL_ARRAY_BUFFER, batch->vertexBuffer[batch->currentBuffer].vboId[1]);
		glBufferSubData(GL_ARRAY_BUFFER, 0, RLGH.State.vertexCounter * 2 * sizeof(float), batch->vertexBuffer[batch->currentBuffer].texcoords);
		//glBufferData(GL_ARRAY_BUFFER, sizeof(float)*2*4*batch->vertexBuffer[batch->currentBuffer].elementCount, batch->vertexBuffer[batch->currentBuffer].texcoords, GL_DYNAMIC_DRAW); // Update all buffer

		// Colors buffer
		glBindBuffer(GL_ARRAY_BUFFER, batch->vertexBuffer[batch->currentBuffer].vboId[2]);
		glBufferSubData(GL_ARRAY_BUFFER, 0, RLGH.State.vertexCounter * 4 * sizeof(unsigned char), batch->vertexBuffer[batch->currentBuffer].colors);
		//glBufferData(GL_ARRAY_BUFFER, sizeof(float)*4*4*batch->vertexBuffer[batch->currentBuffer].elementCount, batch->vertexBuffer[batch->currentBuffer].colors, GL_DYNAMIC_DRAW);    // Update all buffer

		// NOTE: glMapBuffer() causes sync issue.
		// If GPU is working with this buffer, glMapBuffer() will wait(stall) until GPU to finish its job.
		// To avoid waiting (idle), you can call first glBufferData() with NULL pointer before glMapBuffer().
		// If you do that, the previous data in PBO will be discarded and glMapBuffer() returns a new
		// allocated pointer immediately even if GPU is still working with the previous data.

		// Another option: map the buffer object into client's memory
		// Probably this code could be moved somewhere else...
		// batch->vertexBuffer[batch->currentBuffer].vertices = (float *)glMapBuffer(GL_ARRAY_BUFFER, GL_READ_WRITE);
		// if (batch->vertexBuffer[batch->currentBuffer].vertices)
		// {
			// Update vertex data
		// }
		// glUnmapBuffer(GL_ARRAY_BUFFER);

		// Unbind the current VAO
		glBindVertexArray(0);
	}
	//------------------------------------------------------------------------------------------------------------

	// Draw batch vertex buffers (considering VR stereo if required)
	//------------------------------------------------------------------------------------------------------------
	Matrix matProjection = RLGH.State.projection;
	Matrix matModelView = RLGH.State.modelview;

	int eyeCount = 1;
	if (RLGH.State.stereoRender)
		eyeCount = 2;

	for (int eye = 0; eye < eyeCount; eye++)
	{
		if (eyeCount == 2)
		{
			// Setup current eye viewport (half screen width)
			rlViewport(eye * RLGH.State.framebufferWidth / 2, 0, RLGH.State.framebufferWidth / 2, RLGH.State.framebufferHeight);

			// Set current eye view offset to modelview matrix
			rlSetMatrixModelview(rlMatrixMultiply(matModelView, RLGH.State.viewOffsetStereo[eye]));
			// Set current eye projection matrix
			rlSetMatrixProjection(RLGH.State.projectionStereo[eye]);
		}

		// Draw buffers
		if (RLGH.State.vertexCounter > 0)
		{
			// Set current shader and upload current MVP matrix
			glUseProgram(RLGH.State.currentShaderId);

			// Create modelview-projection matrix and upload to shader
			Matrix matMVP = rlMatrixMultiply(RLGH.State.modelview, RLGH.State.projection);
			float matMVPfloat[16] = {
				matMVP.m0, matMVP.m1, matMVP.m2, matMVP.m3,
				matMVP.m4, matMVP.m5, matMVP.m6, matMVP.m7,
				matMVP.m8, matMVP.m9, matMVP.m10, matMVP.m11,
				matMVP.m12, matMVP.m13, matMVP.m14, matMVP.m15
			};
			glUniformMatrix4fv(RLGH.State.currentShaderLocs[RL_SHADER_LOC_MATRIX_MVP], 1, false, matMVPfloat);

			glBindVertexArray(batch->vertexBuffer[batch->currentBuffer].vaoId);

			// Setup some default shader values
			glUniform4f(RLGH.State.currentShaderLocs[RL_SHADER_LOC_COLOR_DIFFUSE], 1.0f, 1.0f, 1.0f, 1.0f);
			glUniform1i(RLGH.State.currentShaderLocs[RL_SHADER_LOC_MAP_DIFFUSE], 0);  // Active default sampler2D: texture0

			// Activate additional sampler textures
			// Those additional textures will be common for all draw calls of the batch
			for (int i = 0; i < RL_DEFAULT_BATCH_MAX_TEXTURE_UNITS; i++)
			{
				if (RLGH.State.activeTextureId[i] > 0)
				{
					glActiveTexture(GL_TEXTURE0 + 1 + i);
					glBindTexture(GL_TEXTURE_2D, RLGH.State.activeTextureId[i]);
				}
			}

			// Activate default sampler2D texture0 (one texture is always active for default batch shader)
			// NOTE: Batch system accumulates calls by texture0 changes, additional textures are enabled for all the draw calls
			glActiveTexture(GL_TEXTURE0);

			for (int i = 0, vertexOffset = 0; i < batch->drawCounter; i++)
			{
				// Bind current draw call texture, activated as GL_TEXTURE0 and Bound to sampler2D texture0 by default
				glBindTexture(GL_TEXTURE_2D, batch->draws[i].textureId);

				if ((batch->draws[i].mode == RL_LINES) || (batch->draws[i].mode == RL_TRIANGLES))
					glDrawArrays(batch->draws[i].mode, vertexOffset, batch->draws[i].vertexCount);
				else
				{
					// We need to define the number of indices to be processed: elementCount*6
					// NOTE: The final parameter tells the GPU the offset in bytes from the
					// start of the index buffer to the location of the first index to process
					glDrawElements(GL_TRIANGLES, batch->draws[i].vertexCount / 4 * 6, GL_UNSIGNED_INT, (GLvoid *)(vertexOffset / 4 * 6 * sizeof(GLuint)));
				}

				vertexOffset += (batch->draws[i].vertexCount + batch->draws[i].vertexAlignment);
			}

			glBindTexture(GL_TEXTURE_2D, 0);    // Unbind textures
		}

		glBindVertexArray(0); // Unbind VAO

		glUseProgram(0);    // Unbind shader program
	}

	// Restore viewport to default measures
	if (eyeCount == 2)
		rlViewport(0, 0, RLGH.State.framebufferWidth, RLGH.State.framebufferHeight);
	//------------------------------------------------------------------------------------------------------------

	// Reset batch buffers
	//------------------------------------------------------------------------------------------------------------
	// Reset vertex counter for next frame
	RLGH.State.vertexCounter = 0;

	// Reset depth for next draw
	batch->currentDepth = -1.0f;

	// Restore projection/modelview matrices
	RLGH.State.projection = matProjection;
	RLGH.State.modelview = matModelView;

	// Reset RLGH.currentBatch->draws array
	for (int i = 0; i < RL_DEFAULT_BATCH_DRAWCALLS; i++)
	{
		batch->draws[i].mode = RL_QUADS;
		batch->draws[i].vertexCount = 0;
		batch->draws[i].textureId = RLGH.State.defaultTexture;
	}

	// Reset active texture units for next batch
	for (int i = 0; i < RL_DEFAULT_BATCH_MAX_TEXTURE_UNITS; i++)
		RLGH.State.activeTextureId[i] = 0;

	// Reset draws counter to one draw for the batch
	batch->drawCounter = 1;
	//------------------------------------------------------------------------------------------------------------

	// Change to next buffer in the list (in case of multi-buffering)
	batch->currentBuffer++;
	if (batch->currentBuffer >= batch->bufferCount)
		batch->currentBuffer = 0;
}

// Set the active render batch for rlgl
void rlSetRenderBatchActive(rlRenderBatch *batch)
{
	rlDrawRenderBatch(RLGH.currentBatch);

	if (batch != NULL)
		RLGH.currentBatch = batch;
	else
		RLGH.currentBatch = &RLGH.defaultBatch;
}

// Update and draw internal render batch
void rlDrawRenderBatchActive(void)
{
	rlDrawRenderBatch(RLGH.currentBatch);    // NOTE: Stereo rendering is checked inside
}

// Check internal buffer overflow for a given number of vertex
// and force a rlRenderBatch draw call if required
bool rlCheckRenderBatchLimit(int vCount)
{
	bool overflow = false;

	if ((RLGH.State.vertexCounter + vCount) >= (vertex_buffer->elementCount * 4))
	{
		overflow = true;

		rlDrawCall *drawcall = &RLGH.currentBatch->draws[RLGH.currentBatch->drawCounter - 1];

		// Store current primitive drawing mode and texture id
		int currentMode = drawcall->mode;
		int currentTexture = drawcall->textureId;

		rlDrawRenderBatch(RLGH.currentBatch);    // NOTE: Stereo rendering is checked inside

		// Restore state of last batch so we can continue adding vertices
		drawcall->mode = currentMode;
		drawcall->textureId = currentTexture;
	}

	return overflow;
}

// Textures data management
//-----------------------------------------------------------------------------------------
// Convert image data to OpenGL texture (returns OpenGL valid Id)
ID3D11Texture2D *rlLoadTexture(const void *data, int width, int height, int format, int mipmapCount)
{
	unsigned int id = 0;

	glBindTexture(GL_TEXTURE_2D, 0);    // Free any old binding

	if ((!RLGH.ExtSupported.texCompDXT) && ((format == RL_PIXELFORMAT_COMPRESSED_DXT1_RGB) || (format == RL_PIXELFORMAT_COMPRESSED_DXT1_RGBA) ||
		(format == RL_PIXELFORMAT_COMPRESSED_DXT3_RGBA) || (format == RL_PIXELFORMAT_COMPRESSED_DXT5_RGBA)))
	{
		TRACELOG(RL_LOG_WARNING, "GL: DXT compressed texture format not supported");
		return id;
	}
	if ((!RLGH.ExtSupported.texCompETC1) && (format == RL_PIXELFORMAT_COMPRESSED_ETC1_RGB))
	{
		TRACELOG(RL_LOG_WARNING, "GL: ETC1 compressed texture format not supported");
		return id;
	}

	if ((!RLGH.ExtSupported.texCompETC2) && ((format == RL_PIXELFORMAT_COMPRESSED_ETC2_RGB) || (format == RL_PIXELFORMAT_COMPRESSED_ETC2_EAC_RGBA)))
	{
		TRACELOG(RL_LOG_WARNING, "GL: ETC2 compressed texture format not supported");
		return id;
	}

	if ((!RLGH.ExtSupported.texCompPVRT) && ((format == RL_PIXELFORMAT_COMPRESSED_PVRT_RGB) || (format == RL_PIXELFORMAT_COMPRESSED_PVRT_RGBA)))
	{
		TRACELOG(RL_LOG_WARNING, "GL: PVRT compressed texture format not supported");
		return id;
	}

	if ((!RLGH.ExtSupported.texCompASTC) && ((format == RL_PIXELFORMAT_COMPRESSED_ASTC_4x4_RGBA) || (format == RL_PIXELFORMAT_COMPRESSED_ASTC_8x8_RGBA)))
	{
		TRACELOG(RL_LOG_WARNING, "GL: ASTC compressed texture format not supported");
		return id;
	}

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glGenTextures(1, &id);              // Generate texture id

	glBindTexture(GL_TEXTURE_2D, id);

	int mipWidth = width;
	int mipHeight = height;
	int mipOffset = 0;          // Mipmap data offset

	// Load the different mipmap levels
	for (int i = 0; i < mipmapCount; i++)
	{
		unsigned int mipSize = rlGetPixelDataSize(mipWidth, mipHeight, format);

		unsigned int glInternalFormat, glFormat, glType;
		rlGetGlTextureFormats(format, &glInternalFormat, &glFormat, &glType);

		TRACELOGD("TEXTURE: Load mipmap level %i (%i x %i), size: %i, offset: %i", i, mipWidth, mipHeight, mipSize, mipOffset);

		if (glInternalFormat != -1)
		{
			if (format < RL_PIXELFORMAT_COMPRESSED_DXT1_RGB)
				glTexImage2D(GL_TEXTURE_2D, i, glInternalFormat, mipWidth, mipHeight, 0, glFormat, glType, (unsigned char *)data + mipOffset);
			else
				glCompressedTexImage2D(GL_TEXTURE_2D, i, glInternalFormat, mipWidth, mipHeight, 0, mipSize, (unsigned char *)data + mipOffset);

			if (format == RL_PIXELFORMAT_UNCOMPRESSED_GRAYSCALE)
			{
				GLint swizzleMask[] = { GL_RED, GL_RED, GL_RED, GL_ONE };
				glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
			} else if (format == RL_PIXELFORMAT_UNCOMPRESSED_GRAY_ALPHA)
			{
				GLint swizzleMask[] = { GL_RED, GL_RED, GL_RED, GL_GREEN };
				glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
			}
		}

		mipWidth /= 2;
		mipHeight /= 2;
		mipOffset += mipSize;

		// Security check for NPOT textures
		if (mipWidth < 1) mipWidth = 1;
		if (mipHeight < 1) mipHeight = 1;
	}

	// Texture parameters configuration
	// NOTE: glTexParameteri does NOT affect texture uploading, just the way it's used
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);       // Set texture to repeat on x-axis
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);       // Set texture to repeat on y-axis

	// Magnification and minification filters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);  // Alternative: GL_LINEAR
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);  // Alternative: GL_LINEAR

	if (mipmapCount > 1)
	{
		// Activate Trilinear filtering if mipmaps are available
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	}

	// At this point we have the texture loaded in GPU and texture parameters configured

	// NOTE: If mipmaps were not in data, they are not generated automatically

	// Unbind current texture
	glBindTexture(GL_TEXTURE_2D, 0);

	if (id > 0)
		TRACELOG(RL_LOG_INFO, "TEXTURE: [ID %i] Texture loaded successfully (%ix%i | %s | %i mipmaps)", id, width, height, rlGetPixelFormatName(format), mipmapCount);
	else
		TRACELOG(RL_LOG_WARNING, "TEXTURE: Failed to load texture");

	return id;
}

// Load depth texture/renderbuffer (to be attached to fbo)
// WARNING: OpenGL ES 2.0 requires GL_OES_depth_texture and WebGL requires WEBGL_depth_texture extensions
unsigned int rlLoadTextureDepth(int width, int height, bool useRenderBuffer)
{
	unsigned int id = 0;

	// NOTE: We let the implementation to choose the best bit-depth
	// Possible formats: GL_DEPTH_COMPONENT16, GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT32 and GL_DEPTH_COMPONENT32F
	unsigned int glInternalFormat = GL_DEPTH_COMPONENT;

	if (!useRenderBuffer && RLGH.ExtSupported.texDepth)
	{
		glGenTextures(1, &id);
		glBindTexture(GL_TEXTURE_2D, id);
		glTexImage2D(GL_TEXTURE_2D, 0, glInternalFormat, width, height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glBindTexture(GL_TEXTURE_2D, 0);

		TRACELOG(RL_LOG_INFO, "TEXTURE: Depth texture loaded successfully");
	} else
	{
		// Create the renderbuffer that will serve as the depth attachment for the framebuffer
		// NOTE: A renderbuffer is simpler than a texture and could offer better performance on embedded devices
		glGenRenderbuffers(1, &id);
		glBindRenderbuffer(GL_RENDERBUFFER, id);
		glRenderbufferStorage(GL_RENDERBUFFER, glInternalFormat, width, height);

		glBindRenderbuffer(GL_RENDERBUFFER, 0);

		TRACELOG(RL_LOG_INFO, "TEXTURE: [ID %i] Depth renderbuffer loaded successfully (%i bits)", id, (RLGH.ExtSupported.maxDepthBits >= 24) ? RLGH.ExtSupported.maxDepthBits : 16);
	}

	return id;
}

// Load texture cubemap
// NOTE: Cubemap data is expected to be 6 images in a single data array (one after the other),
// expected the following convention: +X, -X, +Y, -Y, +Z, -Z
unsigned int rlLoadTextureCubemap(const void *data, int size, int format)
{
	unsigned int id = 0;

	unsigned int dataSize = rlGetPixelDataSize(size, size, format);

	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_CUBE_MAP, id);

	unsigned int glInternalFormat, glFormat, glType;
	rlGetGlTextureFormats(format, &glInternalFormat, &glFormat, &glType);

	if (glInternalFormat != -1)
	{
		// Load cubemap faces
		for (unsigned int i = 0; i < 6; i++)
		{
			if (data == NULL)
			{
				if (format < RL_PIXELFORMAT_COMPRESSED_DXT1_RGB)
				{
					if (format == RL_PIXELFORMAT_UNCOMPRESSED_R32G32B32)
					{
						// Instead of using a sized internal texture format (GL_RGB16F, GL_RGB32F), we let the driver to choose the better format for us (GL_RGB)
						if (RLGH.ExtSupported.texFloat32) glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, size, size, 0, GL_RGB, GL_FLOAT, NULL);
						else TRACELOG(RL_LOG_WARNING, "TEXTURES: Cubemap requested format not supported");
					} else if ((format == RL_PIXELFORMAT_UNCOMPRESSED_R32) || (format == RL_PIXELFORMAT_UNCOMPRESSED_R32G32B32A32)) TRACELOG(RL_LOG_WARNING, "TEXTURES: Cubemap requested format not supported");
					else glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, glInternalFormat, size, size, 0, glFormat, glType, NULL);
				} else TRACELOG(RL_LOG_WARNING, "TEXTURES: Empty cubemap creation does not support compressed format");
			} else
			{
				if (format < RL_PIXELFORMAT_COMPRESSED_DXT1_RGB) glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, glInternalFormat, size, size, 0, glFormat, glType, (unsigned char *)data + i * dataSize);
				else glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, glInternalFormat, size, size, 0, dataSize, (unsigned char *)data + i * dataSize);
			}

			if (format == RL_PIXELFORMAT_UNCOMPRESSED_GRAYSCALE)
			{
				GLint swizzleMask[] = { GL_RED, GL_RED, GL_RED, GL_ONE };
				glTexParameteriv(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
			} else if (format == RL_PIXELFORMAT_UNCOMPRESSED_GRAY_ALPHA)
			{
				GLint swizzleMask[] = { GL_RED, GL_RED, GL_RED, GL_GREEN };
				glTexParameteriv(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
			}
		}
	}

	// Set cubemap texture sampling parameters
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);  // Flag not supported on OpenGL ES 2.0

	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

	if (id > 0)
		TRACELOG(RL_LOG_INFO, "TEXTURE: [ID %i] Cubemap texture loaded successfully (%ix%i)", id, size, size);
	else
		TRACELOG(RL_LOG_WARNING, "TEXTURE: Failed to load cubemap texture");

	return id;
}

// Update already loaded texture in GPU with new data
// NOTE: We don't know safely if internal texture format is the expected one...
void rlUpdateTexture(unsigned int id, int offsetX, int offsetY, int width, int height, int format, const void *data)
{
	glBindTexture(GL_TEXTURE_2D, id);

	unsigned int glInternalFormat, glFormat, glType;
	rlGetGlTextureFormats(format, &glInternalFormat, &glFormat, &glType);

	if ((glInternalFormat != -1) && (format < RL_PIXELFORMAT_COMPRESSED_DXT1_RGB))
	{
		glTexSubImage2D(GL_TEXTURE_2D, 0, offsetX, offsetY, width, height, glFormat, glType, data);
	} else
		TRACELOG(RL_LOG_WARNING, "TEXTURE: [ID %i] Failed to update for current texture format (%i)", id, format);
}

// Get OpenGL internal formats and data type from raylib PixelFormat
void rlGetGlTextureFormats(int format, unsigned int *glInternalFormat, unsigned int *glFormat, unsigned int *glType)
{
	*glInternalFormat = 0;
	*glFormat = 0;
	*glType = 0;

	switch (format)
	{
		case RL_PIXELFORMAT_UNCOMPRESSED_GRAYSCALE: *glInternalFormat = GL_R8; *glFormat = GL_RED; *glType = GL_UNSIGNED_BYTE; break;
		case RL_PIXELFORMAT_UNCOMPRESSED_GRAY_ALPHA: *glInternalFormat = GL_RG8; *glFormat = GL_RG; *glType = GL_UNSIGNED_BYTE; break;
		case RL_PIXELFORMAT_UNCOMPRESSED_R5G6B5: *glInternalFormat = GL_RGB565; *glFormat = GL_RGB; *glType = GL_UNSIGNED_SHORT_5_6_5; break;
		case RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8: *glInternalFormat = GL_RGB8; *glFormat = GL_RGB; *glType = GL_UNSIGNED_BYTE; break;
		case RL_PIXELFORMAT_UNCOMPRESSED_R5G5B5A1: *glInternalFormat = GL_RGB5_A1; *glFormat = GL_RGBA; *glType = GL_UNSIGNED_SHORT_5_5_5_1; break;
		case RL_PIXELFORMAT_UNCOMPRESSED_R4G4B4A4: *glInternalFormat = GL_RGBA4; *glFormat = GL_RGBA; *glType = GL_UNSIGNED_SHORT_4_4_4_4; break;
		case RL_PIXELFORMAT_UNCOMPRESSED_R9G9B9E5: *glInternalFormat = GL_RGBA4; *glFormat = GL_RGBA; *glType = GL_UNSIGNED_SHORT_4_4_4_4; break;
		case RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8: *glInternalFormat = GL_RGBA8; *glFormat = GL_RGBA; *glType = GL_UNSIGNED_BYTE; break;
		case RL_PIXELFORMAT_UNCOMPRESSED_R32: *glInternalFormat = GL_R32F; *glFormat = GL_RED; *glType = GL_FLOAT; break;
		case RL_PIXELFORMAT_UNCOMPRESSED_R32G32B32: *glInternalFormat = GL_RGB32F; *glFormat = GL_RGB; *glType = GL_FLOAT; break;
		case RL_PIXELFORMAT_UNCOMPRESSED_R32G32B32A32: *glInternalFormat = GL_RGBA32F; *glFormat = GL_RGBA; *glType = GL_FLOAT; break;
		case RL_PIXELFORMAT_COMPRESSED_DXT1_RGB: if (RLGH.ExtSupported.texCompDXT) *glInternalFormat = GL_COMPRESSED_RGB_S3TC_DXT1_EXT; break;
		case RL_PIXELFORMAT_COMPRESSED_DXT1_RGBA: if (RLGH.ExtSupported.texCompDXT) *glInternalFormat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT; break;
		case RL_PIXELFORMAT_COMPRESSED_DXT3_RGBA: if (RLGH.ExtSupported.texCompDXT) *glInternalFormat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT; break;
		case RL_PIXELFORMAT_COMPRESSED_DXT5_RGBA: if (RLGH.ExtSupported.texCompDXT) *glInternalFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT; break;
		case RL_PIXELFORMAT_COMPRESSED_ETC1_RGB: if (RLGH.ExtSupported.texCompETC1) *glInternalFormat = GL_ETC1_RGB8_OES; break;                      // NOTE: Requires OpenGL ES 2.0 or OpenGL 4.3
		case RL_PIXELFORMAT_COMPRESSED_ETC2_RGB: if (RLGH.ExtSupported.texCompETC2) *glInternalFormat = GL_COMPRESSED_RGB8_ETC2; break;               // NOTE: Requires OpenGL ES 3.0 or OpenGL 4.3
		case RL_PIXELFORMAT_COMPRESSED_ETC2_EAC_RGBA: if (RLGH.ExtSupported.texCompETC2) *glInternalFormat = GL_COMPRESSED_RGBA8_ETC2_EAC; break;     // NOTE: Requires OpenGL ES 3.0 or OpenGL 4.3
		case RL_PIXELFORMAT_COMPRESSED_PVRT_RGB: if (RLGH.ExtSupported.texCompPVRT) *glInternalFormat = GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG; break;    // NOTE: Requires PowerVR GPU
		case RL_PIXELFORMAT_COMPRESSED_PVRT_RGBA: if (RLGH.ExtSupported.texCompPVRT) *glInternalFormat = GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG; break;  // NOTE: Requires PowerVR GPU
		case RL_PIXELFORMAT_COMPRESSED_ASTC_4x4_RGBA: if (RLGH.ExtSupported.texCompASTC) *glInternalFormat = GL_COMPRESSED_RGBA_ASTC_4x4_KHR; break;  // NOTE: Requires OpenGL ES 3.1 or OpenGL 4.3
		case RL_PIXELFORMAT_COMPRESSED_ASTC_8x8_RGBA: if (RLGH.ExtSupported.texCompASTC) *glInternalFormat = GL_COMPRESSED_RGBA_ASTC_8x8_KHR; break;  // NOTE: Requires OpenGL ES 3.1 or OpenGL 4.3
		default: TRACELOG(RL_LOG_WARNING, "TEXTURE: Current format not supported (%i)", format); break;
	}
}

// Unload texture from GPU memory
void rlUnloadTexture(const rlTexture *texture)
{
	if (texture->dsv)
		texture->dsv->Release();

	if (texture->rtv)
		texture->rtv->Release();

	if (texture->srv)
		texture->srv->Release();

	if (texture->tex2D)
		texture->tex2D->Release();
}

// Generate mipmap data for selected texture
// NOTE: Only supports GPU mipmap generation
void rlGenTextureMipmaps(const rlTexture *texture, int width, int height, int format, int *mipmaps)
{
	RLGH.context->GenerateMips(texture->srv);
}


// Read texture pixel data
void *rlReadTexturePixels(unsigned int id, int width, int height, int format)
{
	void *pixels = NULL;

	glBindTexture(GL_TEXTURE_2D, id);

	// NOTE: Using texture id, we can retrieve some texture info (but not on OpenGL ES 2.0)
	// Possible texture info: GL_TEXTURE_RED_SIZE, GL_TEXTURE_GREEN_SIZE, GL_TEXTURE_BLUE_SIZE, GL_TEXTURE_ALPHA_SIZE
	//int width, height, format;
	//glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
	//glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
	//glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &format);

	// NOTE: Each row written to or read from by OpenGL pixel operations like glGetTexImage are aligned to a 4 byte boundary by default, which may add some padding.
	// Use glPixelStorei to modify padding with the GL_[UN]PACK_ALIGNMENT setting.
	// GL_PACK_ALIGNMENT affects operations that read from OpenGL memory (glReadPixels, glGetTexImage, etc.)
	// GL_UNPACK_ALIGNMENT affects operations that write to OpenGL memory (glTexImage, etc.)
	glPixelStorei(GL_PACK_ALIGNMENT, 1);

	unsigned int glInternalFormat, glFormat, glType;
	rlGetGlTextureFormats(format, &glInternalFormat, &glFormat, &glType);
	unsigned int size = rlGetPixelDataSize(width, height, format);

	if ((glInternalFormat != -1) && (format < RL_PIXELFORMAT_COMPRESSED_DXT1_RGB))
	{
		pixels = RL_MALLOC(size);
		glGetTexImage(GL_TEXTURE_2D, 0, glFormat, glType, pixels);
	} else TRACELOG(RL_LOG_WARNING, "TEXTURE: [ID %i] Data retrieval not suported for pixel format (%i)", id, format);

	glBindTexture(GL_TEXTURE_2D, 0);

	return pixels;
}

// Read screen pixel data (color buffer)
unsigned char *rlReadScreenPixels(int width, int height)
{
	unsigned char *screenData = (unsigned char *)RL_CALLOC(width * height * 4, sizeof(unsigned char));

	// NOTE 1: glReadPixels returns image flipped vertically -> (0,0) is the bottom left corner of the framebuffer
	// NOTE 2: We are getting alpha channel! Be careful, it can be transparent if not cleared properly!
	glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, screenData);

	// Flip image vertically!
	unsigned char *imgData = (unsigned char *)RL_MALLOC(width * height * 4 * sizeof(unsigned char));

	for (int y = height - 1; y >= 0; y--)
	{
		for (int x = 0; x < (width * 4); x++)
		{
			imgData[((height - 1) - y) * width * 4 + x] = screenData[(y * width * 4) + x];  // Flip line

			// Set alpha component value to 255 (no trasparent image retrieval)
			// NOTE: Alpha value has already been applied to RGB in framebuffer, we don't need it!
			if (((x + 1) % 4) == 0) imgData[((height - 1) - y) * width * 4 + x] = 255;
		}
	}

	RL_FREE(screenData);

	return imgData;     // NOTE: image data should be freed
}

// Framebuffer management (fbo)
//-----------------------------------------------------------------------------------------
// Load a framebuffer to be used for rendering
// NOTE: No textures attached
unsigned int rlLoadFramebuffer(int width, int height)
{
	unsigned int fboId = 0;

	glGenFramebuffers(1, &fboId);       // Create the framebuffer object
	glBindFramebuffer(GL_FRAMEBUFFER, 0);   // Unbind any framebuffer

	return fboId;
}

// Attach color buffer texture to an fbo (unloads previous attachment)
// NOTE: Attach type: 0-Color, 1-Depth renderbuffer, 2-Depth texture
void rlFramebufferAttach(unsigned int fboId, unsigned int texId, int attachType, int texType, int mipLevel)
{
	glBindFramebuffer(GL_FRAMEBUFFER, fboId);

	switch (attachType)
	{
		case RL_ATTACHMENT_COLOR_CHANNEL0:
		case RL_ATTACHMENT_COLOR_CHANNEL1:
		case RL_ATTACHMENT_COLOR_CHANNEL2:
		case RL_ATTACHMENT_COLOR_CHANNEL3:
		case RL_ATTACHMENT_COLOR_CHANNEL4:
		case RL_ATTACHMENT_COLOR_CHANNEL5:
		case RL_ATTACHMENT_COLOR_CHANNEL6:
		case RL_ATTACHMENT_COLOR_CHANNEL7:
		{
			if (texType == RL_ATTACHMENT_TEXTURE2D) glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + attachType, GL_TEXTURE_2D, texId, mipLevel);
			else if (texType == RL_ATTACHMENT_RENDERBUFFER) glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + attachType, GL_RENDERBUFFER, texId);
			else if (texType >= RL_ATTACHMENT_CUBEMAP_POSITIVE_X) glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + attachType, GL_TEXTURE_CUBE_MAP_POSITIVE_X + texType, texId, mipLevel);

		} break;
		case RL_ATTACHMENT_DEPTH:
		{
			if (texType == RL_ATTACHMENT_TEXTURE2D) glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texId, mipLevel);
			else if (texType == RL_ATTACHMENT_RENDERBUFFER)  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, texId);

		} break;
		case RL_ATTACHMENT_STENCIL:
		{
			if (texType == RL_ATTACHMENT_TEXTURE2D) glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, texId, mipLevel);
			else if (texType == RL_ATTACHMENT_RENDERBUFFER)  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, texId);

		} break;
		default: break;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// Verify render texture is complete
bool rlFramebufferComplete(unsigned int id)
{
	bool result = false;

	glBindFramebuffer(GL_FRAMEBUFFER, id);

	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		switch (status)
		{
			case GL_FRAMEBUFFER_UNSUPPORTED: TRACELOG(RL_LOG_WARNING, "FBO: [ID %i] Framebuffer is unsupported", id); break;
			case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: TRACELOG(RL_LOG_WARNING, "FBO: [ID %i] Framebuffer has incomplete attachment", id); break;
			case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: TRACELOG(RL_LOG_WARNING, "FBO: [ID %i] Framebuffer has a missing attachment", id); break;
			default: break;
		}
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	result = (status == GL_FRAMEBUFFER_COMPLETE);

	return result;
}

// Unload framebuffer from GPU memory
// NOTE: All attached textures/cubemaps/renderbuffers are also deleted
void rlUnloadFramebuffer(unsigned int id)
{

	// Query depth attachment to automatically delete texture/renderbuffer
	int depthType = 0, depthId = 0;
	glBindFramebuffer(GL_FRAMEBUFFER, id);   // Bind framebuffer to query depth texture type
	glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &depthType);
	glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &depthId);

	unsigned int depthIdU = (unsigned int)depthId;
	if (depthType == GL_RENDERBUFFER) glDeleteRenderbuffers(1, &depthIdU);
	else if (depthType == GL_TEXTURE) glDeleteTextures(1, &depthIdU);

	// NOTE: If a texture object is deleted while its image is attached to the *currently bound* framebuffer,
	// the texture image is automatically detached from the currently bound framebuffer.

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDeleteFramebuffers(1, &id);

	TRACELOG(RL_LOG_INFO, "FBO: [ID %i] Unloaded framebuffer from VRAM (GPU)", id);
}

// Vertex data management
//-----------------------------------------------------------------------------------------
// Load a new attributes buffer
unsigned int rlLoadVertexBuffer(const void *buffer, int size, bool dynamic)
{
	unsigned int id = 0;

	glGenBuffers(1, &id);
	glBindBuffer(GL_ARRAY_BUFFER, id);
	glBufferData(GL_ARRAY_BUFFER, size, buffer, dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);

	return id;
}

// Load a new attributes element buffer
unsigned int rlLoadVertexBufferElement(const void *buffer, int size, bool dynamic)
{
	unsigned int id = 0;

	glGenBuffers(1, &id);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, buffer, dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);

	return id;
}

// Enable vertex buffer (VBO)
void rlEnableVertexBuffer(unsigned int id)
{
	glBindBuffer(GL_ARRAY_BUFFER, id);
}

// Disable vertex buffer (VBO)
void rlDisableVertexBuffer(void)
{
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// Enable vertex buffer element (VBO element)
void rlEnableVertexBufferElement(unsigned int id)
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
}

// Disable vertex buffer element (VBO element)
void rlDisableVertexBufferElement(void)
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

// Update vertex buffer with new data
// NOTE: dataSize and offset must be provided in bytes
void rlUpdateVertexBuffer(unsigned int id, const void *data, int dataSize, int offset)
{
	glBindBuffer(GL_ARRAY_BUFFER, id);
	glBufferSubData(GL_ARRAY_BUFFER, offset, dataSize, data);
}

// Update vertex buffer elements with new data
// NOTE: dataSize and offset must be provided in bytes
void rlUpdateVertexBufferElements(unsigned int id, const void *data, int dataSize, int offset)
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, offset, dataSize, data);
}

// Enable vertex array object (VAO)
bool rlEnableVertexArray(unsigned int vaoId)
{
	bool result = false;
	glBindVertexArray(vaoId);
	result = true;
	return result;
}

// Disable vertex array object (VAO)
void rlDisableVertexArray(void)
{
	glBindVertexArray(0);
}

// Enable vertex attribute index
void rlEnableVertexAttribute(unsigned int index)
{
	glEnableVertexAttribArray(index);
}

// Disable vertex attribute index
void rlDisableVertexAttribute(unsigned int index)
{
	glDisableVertexAttribArray(index);
}

// Draw vertex array
void rlDrawVertexArray(int offset, int count)
{
	glDrawArrays(GL_TRIANGLES, offset, count);
}

// Draw vertex array elements
void rlDrawVertexArrayElements(int offset, int count, const void *buffer)
{
	glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_SHORT, (const unsigned short *)buffer + offset);
}

// Draw vertex array instanced
void rlDrawVertexArrayInstanced(int offset, int count, int instances)
{
	glDrawArraysInstanced(GL_TRIANGLES, 0, count, instances);
}

// Draw vertex array elements instanced
void rlDrawVertexArrayElementsInstanced(int offset, int count, const void *buffer, int instances)
{
	glDrawElementsInstanced(GL_TRIANGLES, count, GL_UNSIGNED_SHORT, (const unsigned short *)buffer + offset, instances);
}

// Load vertex array object (VAO)
unsigned int rlLoadVertexArray(void)
{
	unsigned int vaoId = 0;
	glGenVertexArrays(1, &vaoId);
	return vaoId;
}

// Set vertex attribute
void rlSetVertexAttribute(unsigned int index, int compSize, int type, bool normalized, int stride, const void *pointer)
{
	glVertexAttribPointer(index, compSize, type, normalized, stride, pointer);
}

// Set vertex attribute divisor
void rlSetVertexAttributeDivisor(unsigned int index, int divisor)
{
	glVertexAttribDivisor(index, divisor);
}

// Unload vertex array object (VAO)
void rlUnloadVertexArray(unsigned int vaoId)
{
	glBindVertexArray(0);
	glDeleteVertexArrays(1, &vaoId);
	TRACELOG(RL_LOG_INFO, "VAO: [ID %i] Unloaded vertex array data from VRAM (GPU)", vaoId);
}

// Unload vertex buffer (VBO)
void rlUnloadVertexBuffer(unsigned int vboId)
{
	glDeleteBuffers(1, &vboId);
	//TRACELOG(RL_LOG_INFO, "VBO: Unloaded vertex data from VRAM (GPU)");
}

// Shaders management
//-----------------------------------------------------------------------------------------------
// Load shader from code strings
// NOTE: If shader string is NULL, using default vertex/fragment shaders
unsigned int rlLoadShaderCode(const char *vsCode, const char *fsCode)
{
	unsigned int id = 0;

	unsigned int vertexShaderId = 0;
	unsigned int fragmentShaderId = 0;

	// Compile vertex shader (if provided)
	if (vsCode != NULL) vertexShaderId = rlCompileShader(vsCode, GL_VERTEX_SHADER);
	// In case no vertex shader was provided or compilation failed, we use default vertex shader
	if (vertexShaderId == 0) vertexShaderId = RLGH.State.defaultVShaderId;

	// Compile fragment shader (if provided)
	if (fsCode != NULL) fragmentShaderId = rlCompileShader(fsCode, GL_FRAGMENT_SHADER);
	// In case no fragment shader was provided or compilation failed, we use default fragment shader
	if (fragmentShaderId == 0) fragmentShaderId = RLGH.State.defaultFShaderId;

	// In case vertex and fragment shader are the default ones, no need to recompile, we can just assign the default shader program id
	if ((vertexShaderId == RLGH.State.defaultVShaderId) && (fragmentShaderId == RLGH.State.defaultFShaderId)) id = RLGH.State.defaultShaderId;
	else
	{
		// One of or both shader are new, we need to compile a new shader program
		id = rlLoadShaderProgram(vertexShaderId, fragmentShaderId);

		// We can detach and delete vertex/fragment shaders (if not default ones)
		// NOTE: We detach shader before deletion to make sure memory is freed
		if (vertexShaderId != RLGH.State.defaultVShaderId)
		{
			// WARNING: Shader program linkage could fail and returned id is 0
			if (id > 0) glDetachShader(id, vertexShaderId);
			glDeleteShader(vertexShaderId);
		}
		if (fragmentShaderId != RLGH.State.defaultFShaderId)
		{
			// WARNING: Shader program linkage could fail and returned id is 0
			if (id > 0) glDetachShader(id, fragmentShaderId);
			glDeleteShader(fragmentShaderId);
		}

		// In case shader program loading failed, we assign default shader
		if (id == 0)
		{
			// In case shader loading fails, we return the default shader
			TRACELOG(RL_LOG_WARNING, "SHADER: Failed to load custom shader code, using default shader");
			id = RLGH.State.defaultShaderId;
		}
		/*
		else
		{
			// Get available shader uniforms
			// NOTE: This information is useful for debug...
			int uniformCount = -1;
			glGetProgramiv(id, GL_ACTIVE_UNIFORMS, &uniformCount);

			for (int i = 0; i < uniformCount; i++)
			{
				int namelen = -1;
				int num = -1;
				char name[256] = { 0 };     // Assume no variable names longer than 256
				GLenum type = GL_ZERO;

				// Get the name of the uniforms
				glGetActiveUniform(id, i, sizeof(name) - 1, &namelen, &num, &type, name);

				name[namelen] = 0;
				TRACELOGD("SHADER: [ID %i] Active uniform (%s) set at location: %i", id, name, glGetUniformLocation(id, name));
			}
		}
		*/
	}

	return id;
}

// Compile custom shader and return shader id
unsigned int rlCompileShader(const char *shaderCode, int type)
{
	unsigned int shader = 0;

	shader = glCreateShader(type);
	glShaderSource(shader, 1, &shaderCode, NULL);

	GLint success = 0;
	glCompileShader(shader);
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

	if (success == GL_FALSE)
	{
		switch (type)
		{
			case GL_VERTEX_SHADER: TRACELOG(RL_LOG_WARNING, "SHADER: [ID %i] Failed to compile vertex shader code", shader); break;
			case GL_FRAGMENT_SHADER: TRACELOG(RL_LOG_WARNING, "SHADER: [ID %i] Failed to compile fragment shader code", shader); break;
				//case GL_GEOMETRY_SHADER:
			case GL_COMPUTE_SHADER: TRACELOG(RL_LOG_WARNING, "SHADER: [ID %i] Failed to compile compute shader code", shader); break;
			default: break;
		}

		int maxLength = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

		if (maxLength > 0)
		{
			int length = 0;
			char *log = (char *)RL_CALLOC(maxLength, sizeof(char));
			glGetShaderInfoLog(shader, maxLength, &length, log);
			TRACELOG(RL_LOG_WARNING, "SHADER: [ID %i] Compile error: %s", shader, log);
			RL_FREE(log);
		}
	} else
	{
		switch (type)
		{
			case GL_VERTEX_SHADER: TRACELOG(RL_LOG_INFO, "SHADER: [ID %i] Vertex shader compiled successfully", shader); break;
			case GL_FRAGMENT_SHADER: TRACELOG(RL_LOG_INFO, "SHADER: [ID %i] Fragment shader compiled successfully", shader); break;
				//case GL_GEOMETRY_SHADER:
			case GL_COMPUTE_SHADER: TRACELOG(RL_LOG_INFO, "SHADER: [ID %i] Compute shader compiled successfully", shader); break;
			default: break;
		}
	}

	return shader;
}

// Load custom shader strings and return program id
unsigned int rlLoadShaderProgram(unsigned int vShaderId, unsigned int fShaderId)
{
	unsigned int program = 0;

	GLint success = 0;
	program = glCreateProgram();

	glAttachShader(program, vShaderId);
	glAttachShader(program, fShaderId);

	// NOTE: Default attribute shader locations must be Bound before linking
	glBindAttribLocation(program, 0, RL_DEFAULT_SHADER_ATTRIB_NAME_POSITION);
	glBindAttribLocation(program, 1, RL_DEFAULT_SHADER_ATTRIB_NAME_TEXCOORD);
	glBindAttribLocation(program, 2, RL_DEFAULT_SHADER_ATTRIB_NAME_NORMAL);
	glBindAttribLocation(program, 3, RL_DEFAULT_SHADER_ATTRIB_NAME_COLOR);
	glBindAttribLocation(program, 4, RL_DEFAULT_SHADER_ATTRIB_NAME_TANGENT);
	glBindAttribLocation(program, 5, RL_DEFAULT_SHADER_ATTRIB_NAME_TEXCOORD2);

	// NOTE: If some attrib name is no found on the shader, it locations becomes -1

	glLinkProgram(program);

	// NOTE: All uniform variables are intitialised to 0 when a program links

	glGetProgramiv(program, GL_LINK_STATUS, &success);

	if (success == GL_FALSE)
	{
		TRACELOG(RL_LOG_WARNING, "SHADER: [ID %i] Failed to link shader program", program);

		int maxLength = 0;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

		if (maxLength > 0)
		{
			int length = 0;
			char *log = (char *)RL_CALLOC(maxLength, sizeof(char));
			glGetProgramInfoLog(program, maxLength, &length, log);
			TRACELOG(RL_LOG_WARNING, "SHADER: [ID %i] Link error: %s", program, log);
			RL_FREE(log);
		}

		glDeleteProgram(program);

		program = 0;
	} else
	{
		// Get the size of compiled shader program (not available on OpenGL ES 2.0)
		// NOTE: If GL_LINK_STATUS is GL_FALSE, program binary length is zero.
		//GLint binarySize = 0;
		//glGetProgramiv(id, GL_PROGRAM_BINARY_LENGTH, &binarySize);

		TRACELOG(RL_LOG_INFO, "SHADER: [ID %i] Program shader loaded successfully", program);
	}

	return program;
}

// Unload shader program
void rlUnloadShaderProgram(unsigned int id)
{
	glDeleteProgram(id);

	TRACELOG(RL_LOG_INFO, "SHADER: [ID %i] Unloaded shader program data from VRAM (GPU)", id);
}

// Get shader location uniform
int rlGetLocationUniform(unsigned int shaderId, const char *uniformName)
{
	//if (location == -1) TRACELOG(RL_LOG_WARNING, "SHADER: [ID %i] Failed to find shader uniform: %s", shaderId, uniformName);
	//else TRACELOG(RL_LOG_INFO, "SHADER: [ID %i] Shader uniform (%s) set at location: %i", shaderId, uniformName, location);
	return glGetUniformLocation(shaderId, uniformName);
}

// Get shader location attribute
int rlGetLocationAttrib(unsigned int shaderId, const char *attribName)
{
	//if (location == -1) TRACELOG(RL_LOG_WARNING, "SHADER: [ID %i] Failed to find shader attribute: %s", shaderId, attribName);
	//else TRACELOG(RL_LOG_INFO, "SHADER: [ID %i] Shader attribute (%s) set at location: %i", shaderId, attribName, location);
	return glGetAttribLocation(shaderId, attribName);
}

// Set shader value uniform
void rlSetUniform(int locIndex, const void *value, int uniformType, int count)
{
	switch (uniformType)
	{
		case RL_SHADER_UNIFORM_FLOAT: glUniform1fv(locIndex, count, (float *)value); break;
		case RL_SHADER_UNIFORM_VEC2: glUniform2fv(locIndex, count, (float *)value); break;
		case RL_SHADER_UNIFORM_VEC3: glUniform3fv(locIndex, count, (float *)value); break;
		case RL_SHADER_UNIFORM_VEC4: glUniform4fv(locIndex, count, (float *)value); break;
		case RL_SHADER_UNIFORM_INT: glUniform1iv(locIndex, count, (int *)value); break;
		case RL_SHADER_UNIFORM_IVEC2: glUniform2iv(locIndex, count, (int *)value); break;
		case RL_SHADER_UNIFORM_IVEC3: glUniform3iv(locIndex, count, (int *)value); break;
		case RL_SHADER_UNIFORM_IVEC4: glUniform4iv(locIndex, count, (int *)value); break;
		case RL_SHADER_UNIFORM_SAMPLER2D: glUniform1iv(locIndex, count, (int *)value); break;
		default: TRACELOG(RL_LOG_WARNING, "SHADER: Failed to set uniform value, data type not recognized");
	}
}

// Set shader value attribute
void rlSetVertexAttributeDefault(int locIndex, const void *value, int attribType, int count)
{
	switch (attribType)
	{
		case RL_SHADER_ATTRIB_FLOAT: if (count == 1) glVertexAttrib1fv(locIndex, (float *)value); break;
		case RL_SHADER_ATTRIB_VEC2: if (count == 2) glVertexAttrib2fv(locIndex, (float *)value); break;
		case RL_SHADER_ATTRIB_VEC3: if (count == 3) glVertexAttrib3fv(locIndex, (float *)value); break;
		case RL_SHADER_ATTRIB_VEC4: if (count == 4) glVertexAttrib4fv(locIndex, (float *)value); break;
		default: TRACELOG(RL_LOG_WARNING, "SHADER: Failed to set attrib default value, data type not recognized");
	}
}

// Set shader value uniform matrix
void rlSetUniformMatrix(int locIndex, Matrix mat)
{
	float matfloat[16] = {
		mat.m0, mat.m1, mat.m2, mat.m3,
		mat.m4, mat.m5, mat.m6, mat.m7,
		mat.m8, mat.m9, mat.m10, mat.m11,
		mat.m12, mat.m13, mat.m14, mat.m15
	};
	glUniformMatrix4fv(locIndex, 1, false, matfloat);
}

// Set shader value uniform sampler
void rlSetUniformSampler(int locIndex, unsigned int textureId)
{
	// Check if texture is already active
	for (int i = 0; i < RL_DEFAULT_BATCH_MAX_TEXTURE_UNITS; i++) if (RLGH.State.activeTextureId[i] == textureId) return;

	// Register a new active texture for the internal batch system
	// NOTE: Default texture is always activated as GL_TEXTURE0
	for (int i = 0; i < RL_DEFAULT_BATCH_MAX_TEXTURE_UNITS; i++)
	{
		if (RLGH.State.activeTextureId[i] == 0)
		{
			glUniform1i(locIndex, 1 + i);              // Activate new texture unit
			RLGH.State.activeTextureId[i] = textureId; // Save texture id for binding on drawing
			break;
		}
	}
}

// Set shader currently active (id and locations)
void rlSetShader(unsigned int id, int *locs)
{
	if (RLGH.State.currentShaderId != id)
	{
		rlDrawRenderBatch(RLGH.currentBatch);
		RLGH.State.currentShaderId = id;
		RLGH.State.currentShaderLocs = locs;
	}
}

// Load compute shader program
unsigned int rlLoadComputeShaderProgram(unsigned int shaderId)
{
	unsigned int program = 0;

	GLint success = 0;
	program = glCreateProgram();
	glAttachShader(program, shaderId);
	glLinkProgram(program);

	// NOTE: All uniform variables are intitialised to 0 when a program links

	glGetProgramiv(program, GL_LINK_STATUS, &success);

	if (success == GL_FALSE)
	{
		TRACELOG(RL_LOG_WARNING, "SHADER: [ID %i] Failed to link compute shader program", program);

		int maxLength = 0;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

		if (maxLength > 0)
		{
			int length = 0;
			char *log = (char *)RL_CALLOC(maxLength, sizeof(char));
			glGetProgramInfoLog(program, maxLength, &length, log);
			TRACELOG(RL_LOG_WARNING, "SHADER: [ID %i] Link error: %s", program, log);
			RL_FREE(log);
		}

		glDeleteProgram(program);

		program = 0;
	} else
	{
		// Get the size of compiled shader program (not available on OpenGL ES 2.0)
		// NOTE: If GL_LINK_STATUS is GL_FALSE, program binary length is zero.
		//GLint binarySize = 0;
		//glGetProgramiv(id, GL_PROGRAM_BINARY_LENGTH, &binarySize);

		TRACELOG(RL_LOG_INFO, "SHADER: [ID %i] Compute shader program loaded successfully", program);
	}

	return program;
}

// Dispatch compute shader (equivalent to *draw* for graphics pilepine)
void rlComputeShaderDispatch(unsigned int groupX, unsigned int groupY, unsigned int groupZ)
{
	glDispatchCompute(groupX, groupY, groupZ);
}

// Load shader storage buffer object (SSBO)
unsigned int rlLoadShaderBuffer(unsigned int size, const void *data, int usageHint)
{
	unsigned int ssbo = 0;

	glGenBuffers(1, &ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, size, data, usageHint ? usageHint : RL_STREAM_COPY);
	glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R8UI, GL_RED_INTEGER, GL_UNSIGNED_BYTE, 0);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	return ssbo;
}

// Unload shader storage buffer object (SSBO)
void rlUnloadShaderBuffer(unsigned int ssboId)
{
	glDeleteBuffers(1, &ssboId);
}

// Update SSBO buffer data
void rlUpdateShaderBuffer(unsigned int id, const void *data, unsigned int dataSize, unsigned int offset)
{
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, id);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, dataSize, data);
}

// Get SSBO buffer size
unsigned int rlGetShaderBufferSize(unsigned int id)
{
	long long size = 0;

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, id);
	glGetInteger64v(GL_SHADER_STORAGE_BUFFER_SIZE, &size);

	return (size > 0) ? (unsigned int)size : 0;
}

// Read SSBO buffer data (GPU->CPU)
void rlReadShaderBuffer(unsigned int id, void *dest, unsigned int count, unsigned int offset)
{
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, id);
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, count, dest);
}

// Bind SSBO buffer
void rlBindShaderBuffer(unsigned int id, unsigned int index)
{
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, id);
}

// Copy SSBO buffer data
void rlCopyShaderBuffer(unsigned int destId, unsigned int srcId, unsigned int destOffset, unsigned int srcOffset, unsigned int count)
{
	glBindBuffer(GL_COPY_READ_BUFFER, srcId);
	glBindBuffer(GL_COPY_WRITE_BUFFER, destId);
	glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, srcOffset, destOffset, count);
}

// Bind image texture
void rlBindImageTexture(unsigned int id, unsigned int index, int format, bool readonly)
{
	unsigned int glInternalFormat = 0, glFormat = 0, glType = 0;

	rlGetGlTextureFormats(format, &glInternalFormat, &glFormat, &glType);
	glBindImageTexture(index, id, 0, 0, 0, readonly ? GL_READ_ONLY : GL_READ_WRITE, glInternalFormat);
}

// Matrix state management
//-----------------------------------------------------------------------------------------
// Get internal modelview matrix
Matrix rlGetMatrixModelview(void)
{
	return RLGH.State.modelview;
}

// Get internal projection matrix
Matrix rlGetMatrixProjection(void)
{
	return RLGH.State.projection;
}

// Get internal accumulated transform matrix
Matrix rlGetMatrixTransform(void)
{
	return RLGH.State.transform;
}

// Get internal projection matrix for stereo render (selected eye)
RLAPI Matrix rlGetMatrixProjectionStereo(int eye)
{
	return RLGH.State.projectionStereo[eye];
}

// Get internal view offset matrix for stereo render (selected eye)
RLAPI Matrix rlGetMatrixViewOffsetStereo(int eye)
{
	return RLGH.State.viewOffsetStereo[eye];
}

// Set a custom modelview matrix (replaces internal modelview matrix)
void rlSetMatrixModelview(Matrix view)
{
	RLGH.State.modelview = view;
}

// Set a custom projection matrix (replaces internal projection matrix)
void rlSetMatrixProjection(Matrix projection)
{
	RLGH.State.projection = projection;
}

// Set eyes projection matrices for stereo rendering
void rlSetMatrixProjectionStereo(Matrix right, Matrix left)
{
	RLGH.State.projectionStereo[0] = right;
	RLGH.State.projectionStereo[1] = left;
}

// Set eyes view offsets matrices for stereo rendering
void rlSetMatrixViewOffsetStereo(Matrix right, Matrix left)
{
	RLGH.State.viewOffsetStereo[0] = right;
	RLGH.State.viewOffsetStereo[1] = left;
}

// Load and draw a quad in NDC
void rlLoadDrawQuad(void)
{
	unsigned int quadVAO = 0;
	unsigned int quadVBO = 0;

	float vertices[] = {
		// Positions         Texcoords
	   -1.0f,  1.0f, 0.0f,   0.0f, 1.0f,
	   -1.0f, -1.0f, 0.0f,   0.0f, 0.0f,
		1.0f,  1.0f, 0.0f,   1.0f, 1.0f,
		1.0f, -1.0f, 0.0f,   1.0f, 0.0f,
	};

	// Gen VAO to contain VBO
	glGenVertexArrays(1, &quadVAO);
	glBindVertexArray(quadVAO);

	// Gen and fill vertex buffer (VBO)
	glGenBuffers(1, &quadVBO);
	glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices, GL_STATIC_DRAW);

	// Bind vertex attributes (position, texcoords)
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0); // Positions
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(3 * sizeof(float))); // Texcoords

	// Draw quad
	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);

	// Delete buffers (VBO and VAO)
	glDeleteBuffers(1, &quadVBO);
	glDeleteVertexArrays(1, &quadVAO);
}

// Load and draw a cube in NDC
void rlLoadDrawCube(void)
{
	unsigned int cubeVAO = 0;
	unsigned int cubeVBO = 0;

	float vertices[] = {
		// Positions          Normals               Texcoords
	   -1.0f, -1.0f, -1.0f,   0.0f,  0.0f, -1.0f,   0.0f, 0.0f,
		1.0f,  1.0f, -1.0f,   0.0f,  0.0f, -1.0f,   1.0f, 1.0f,
		1.0f, -1.0f, -1.0f,   0.0f,  0.0f, -1.0f,   1.0f, 0.0f,
		1.0f,  1.0f, -1.0f,   0.0f,  0.0f, -1.0f,   1.0f, 1.0f,
	   -1.0f, -1.0f, -1.0f,   0.0f,  0.0f, -1.0f,   0.0f, 0.0f,
	   -1.0f,  1.0f, -1.0f,   0.0f,  0.0f, -1.0f,   0.0f, 1.0f,
	   -1.0f, -1.0f,  1.0f,   0.0f,  0.0f,  1.0f,   0.0f, 0.0f,
		1.0f, -1.0f,  1.0f,   0.0f,  0.0f,  1.0f,   1.0f, 0.0f,
		1.0f,  1.0f,  1.0f,   0.0f,  0.0f,  1.0f,   1.0f, 1.0f,
		1.0f,  1.0f,  1.0f,   0.0f,  0.0f,  1.0f,   1.0f, 1.0f,
	   -1.0f,  1.0f,  1.0f,   0.0f,  0.0f,  1.0f,   0.0f, 1.0f,
	   -1.0f, -1.0f,  1.0f,   0.0f,  0.0f,  1.0f,   0.0f, 0.0f,
	   -1.0f,  1.0f,  1.0f,  -1.0f,  0.0f,  0.0f,   1.0f, 0.0f,
	   -1.0f,  1.0f, -1.0f,  -1.0f,  0.0f,  0.0f,   1.0f, 1.0f,
	   -1.0f, -1.0f, -1.0f,  -1.0f,  0.0f,  0.0f,   0.0f, 1.0f,
	   -1.0f, -1.0f, -1.0f,  -1.0f,  0.0f,  0.0f,   0.0f, 1.0f,
	   -1.0f, -1.0f,  1.0f,  -1.0f,  0.0f,  0.0f,   0.0f, 0.0f,
	   -1.0f,  1.0f,  1.0f,  -1.0f,  0.0f,  0.0f,   1.0f, 0.0f,
		1.0f,  1.0f,  1.0f,   1.0f,  0.0f,  0.0f,   1.0f, 0.0f,
		1.0f, -1.0f, -1.0f,   1.0f,  0.0f,  0.0f,   0.0f, 1.0f,
		1.0f,  1.0f, -1.0f,   1.0f,  0.0f,  0.0f,   1.0f, 1.0f,
		1.0f, -1.0f, -1.0f,   1.0f,  0.0f,  0.0f,   0.0f, 1.0f,
		1.0f,  1.0f,  1.0f,   1.0f,  0.0f,  0.0f,   1.0f, 0.0f,
		1.0f, -1.0f,  1.0f,   1.0f,  0.0f,  0.0f,   0.0f, 0.0f,
	   -1.0f, -1.0f, -1.0f,   0.0f, -1.0f,  0.0f,   0.0f, 1.0f,
		1.0f, -1.0f, -1.0f,   0.0f, -1.0f,  0.0f,   1.0f, 1.0f,
		1.0f, -1.0f,  1.0f,   0.0f, -1.0f,  0.0f,   1.0f, 0.0f,
		1.0f, -1.0f,  1.0f,   0.0f, -1.0f,  0.0f,   1.0f, 0.0f,
	   -1.0f, -1.0f,  1.0f,   0.0f, -1.0f,  0.0f,   0.0f, 0.0f,
	   -1.0f, -1.0f, -1.0f,   0.0f, -1.0f,  0.0f,   0.0f, 1.0f,
	   -1.0f,  1.0f, -1.0f,   0.0f,  1.0f,  0.0f,   0.0f, 1.0f,
		1.0f,  1.0f,  1.0f,   0.0f,  1.0f,  0.0f,   1.0f, 0.0f,
		1.0f,  1.0f, -1.0f,   0.0f,  1.0f,  0.0f,   1.0f, 1.0f,
		1.0f,  1.0f,  1.0f,   0.0f,  1.0f,  0.0f,   1.0f, 0.0f,
	   -1.0f,  1.0f, -1.0f,   0.0f,  1.0f,  0.0f,   0.0f, 1.0f,
	   -1.0f,  1.0f,  1.0f,   0.0f,  1.0f,  0.0f,   0.0f, 0.0f
	};

	// Gen VAO to contain VBO
	glGenVertexArrays(1, &cubeVAO);
	glBindVertexArray(cubeVAO);

	// Gen and fill vertex buffer (VBO)
	glGenBuffers(1, &cubeVBO);
	glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// Bind vertex attributes (position, normals, texcoords)
	glBindVertexArray(cubeVAO);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0); // Positions
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float))); // Normals
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(6 * sizeof(float))); // Texcoords
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	// Draw cube
	glBindVertexArray(cubeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);

	// Delete VBO and VAO
	glDeleteBuffers(1, &cubeVBO);
	glDeleteVertexArrays(1, &cubeVAO);
}

// Get name string for pixel format
const char *rlGetPixelFormatName(unsigned int format)
{
	switch (format)
	{
		case RL_PIXELFORMAT_UNCOMPRESSED_GRAYSCALE: return "GRAYSCALE"; break;         // 8 bit per pixel (no alpha)
		case RL_PIXELFORMAT_UNCOMPRESSED_GRAY_ALPHA: return "GRAY_ALPHA"; break;       // 8*2 bpp (2 channels)
		case RL_PIXELFORMAT_UNCOMPRESSED_R5G6B5: return "R5G6B5"; break;               // 16 bpp
		case RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8: return "R8G8B8"; break;               // 24 bpp
		case RL_PIXELFORMAT_UNCOMPRESSED_R5G5B5A1: return "R5G5B5A1"; break;           // 16 bpp (1 bit alpha)
		case RL_PIXELFORMAT_UNCOMPRESSED_R4G4B4A4: return "R4G4B4A4"; break;           // 16 bpp (4 bit alpha)
		case RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8: return "R8G8B8A8"; break;           // 32 bpp
		case RL_PIXELFORMAT_UNCOMPRESSED_R32: return "R32"; break;                     // 32 bpp (1 channel - float)
		case RL_PIXELFORMAT_UNCOMPRESSED_R32G32B32: return "R32G32B32"; break;         // 32*3 bpp (3 channels - float)
		case RL_PIXELFORMAT_UNCOMPRESSED_R32G32B32A32: return "R32G32B32A32"; break;   // 32*4 bpp (4 channels - float)
		case RL_PIXELFORMAT_COMPRESSED_DXT1_RGB: return "DXT1_RGB"; break;             // 4 bpp (no alpha)
		case RL_PIXELFORMAT_COMPRESSED_DXT1_RGBA: return "DXT1_RGBA"; break;           // 4 bpp (1 bit alpha)
		case RL_PIXELFORMAT_COMPRESSED_DXT3_RGBA: return "DXT3_RGBA"; break;           // 8 bpp
		case RL_PIXELFORMAT_COMPRESSED_DXT5_RGBA: return "DXT5_RGBA"; break;           // 8 bpp
		case RL_PIXELFORMAT_COMPRESSED_ETC1_RGB: return "ETC1_RGB"; break;             // 4 bpp
		case RL_PIXELFORMAT_COMPRESSED_ETC2_RGB: return "ETC2_RGB"; break;             // 4 bpp
		case RL_PIXELFORMAT_COMPRESSED_ETC2_EAC_RGBA: return "ETC2_RGBA"; break;       // 8 bpp
		case RL_PIXELFORMAT_COMPRESSED_PVRT_RGB: return "PVRT_RGB"; break;             // 4 bpp
		case RL_PIXELFORMAT_COMPRESSED_PVRT_RGBA: return "PVRT_RGBA"; break;           // 4 bpp
		case RL_PIXELFORMAT_COMPRESSED_ASTC_4x4_RGBA: return "ASTC_4x4_RGBA"; break;   // 8 bpp
		case RL_PIXELFORMAT_COMPRESSED_ASTC_8x8_RGBA: return "ASTC_8x8_RGBA"; break;   // 2 bpp
		default: return "UNKNOWN"; break;
	}
}

//----------------------------------------------------------------------------------
// Module specific Functions Definition
//----------------------------------------------------------------------------------
// Load default shader (just vertex positioning and texture coloring)
// NOTE: This shader program is used for internal buffers
// NOTE: Loaded: RLGH.State.defaultShaderId, RLGH.State.defaultShaderLocs
static void rlLoadShaderDefault(void)
{
	RLGH.State.defaultShaderLocs = (int *)RL_CALLOC(RL_MAX_SHADER_LOCATIONS, sizeof(int));

	// NOTE: All locations must be reseted to -1 (no location)
	for (int i = 0; i < RL_MAX_SHADER_LOCATIONS; i++) RLGH.State.defaultShaderLocs[i] = -1;

	// Vertex shader directly defined, no external file required
	const char *defaultVShaderCode =
		"#version 330                       \n"
		"in vec3 vertexPosition;            \n"
		"in vec2 vertexTexCoord;            \n"
		"in vec4 vertexColor;               \n"
		"out vec2 fragTexCoord;             \n"
		"out vec4 fragColor;                \n"
		"uniform mat4 mvp;                  \n"
		"void main()                        \n"
		"{                                  \n"
		"    fragTexCoord = vertexTexCoord; \n"
		"    fragColor = vertexColor;       \n"
		"    gl_Position = mvp*vec4(vertexPosition, 1.0); \n"
		"}                                  \n";

	// Fragment shader directly defined, no external file required
	const char *defaultFShaderCode =
		"#version 330       \n"
		"in vec2 fragTexCoord;              \n"
		"in vec4 fragColor;                 \n"
		"out vec4 finalColor;               \n"
		"uniform sampler2D texture0;        \n"
		"uniform vec4 colDiffuse;           \n"
		"void main()                        \n"
		"{                                  \n"
		"    vec4 texelColor = texture(texture0, fragTexCoord);   \n"
		"    finalColor = texelColor*colDiffuse*fragColor;        \n"
		"}                                  \n";

	// NOTE: Compiled vertex/fragment shaders are not deleted,
	// they are kept for re-use as default shaders in case some shader loading fails
	RLGH.State.defaultVShaderId = rlCompileShader(defaultVShaderCode, GL_VERTEX_SHADER);     // Compile default vertex shader
	RLGH.State.defaultFShaderId = rlCompileShader(defaultFShaderCode, GL_FRAGMENT_SHADER);   // Compile default fragment shader

	RLGH.State.defaultShaderId = rlLoadShaderProgram(RLGH.State.defaultVShaderId, RLGH.State.defaultFShaderId);

	if (RLGH.State.defaultShaderId > 0)
	{
		TRACELOG(RL_LOG_INFO, "SHADER: [ID %i] Default shader loaded successfully", RLGH.State.defaultShaderId);

		// Set default shader locations: attributes locations
		RLGH.State.defaultShaderLocs[RL_SHADER_LOC_VERTEX_POSITION] = glGetAttribLocation(RLGH.State.defaultShaderId, "vertexPosition");
		RLGH.State.defaultShaderLocs[RL_SHADER_LOC_VERTEX_TEXCOORD01] = glGetAttribLocation(RLGH.State.defaultShaderId, "vertexTexCoord");
		RLGH.State.defaultShaderLocs[RL_SHADER_LOC_VERTEX_COLOR] = glGetAttribLocation(RLGH.State.defaultShaderId, "vertexColor");

		// Set default shader locations: uniform locations
		RLGH.State.defaultShaderLocs[RL_SHADER_LOC_MATRIX_MVP] = glGetUniformLocation(RLGH.State.defaultShaderId, "mvp");
		RLGH.State.defaultShaderLocs[RL_SHADER_LOC_COLOR_DIFFUSE] = glGetUniformLocation(RLGH.State.defaultShaderId, "colDiffuse");
		RLGH.State.defaultShaderLocs[RL_SHADER_LOC_MAP_DIFFUSE] = glGetUniformLocation(RLGH.State.defaultShaderId, "texture0");
	} else TRACELOG(RL_LOG_WARNING, "SHADER: [ID %i] Failed to load default shader", RLGH.State.defaultShaderId);
}

// Unload default shader
// NOTE: Unloads: RLGH.State.defaultShaderId, RLGH.State.defaultShaderLocs
static void rlUnloadShaderDefault(void)
{
	glUseProgram(0);

	glDetachShader(RLGH.State.defaultShaderId, RLGH.State.defaultVShaderId);
	glDetachShader(RLGH.State.defaultShaderId, RLGH.State.defaultFShaderId);
	glDeleteShader(RLGH.State.defaultVShaderId);
	glDeleteShader(RLGH.State.defaultFShaderId);

	glDeleteProgram(RLGH.State.defaultShaderId);

	RL_FREE(RLGH.State.defaultShaderLocs);

	TRACELOG(RL_LOG_INFO, "SHADER: [ID %i] Default shader unloaded successfully", RLGH.State.defaultShaderId);
}

#if defined(RLGL_SHOW_GL_DETAILS_INFO)
// Get compressed format official GL identifier name
static char *rlGetCompressedFormatName(int format)
{
	switch (format)
	{
		// GL_EXT_texture_compression_s3tc
		case 0x83F0: return "GL_COMPRESSED_RGB_S3TC_DXT1_EXT"; break;
		case 0x83F1: return "GL_COMPRESSED_RGBA_S3TC_DXT1_EXT"; break;
		case 0x83F2: return "GL_COMPRESSED_RGBA_S3TC_DXT3_EXT"; break;
		case 0x83F3: return "GL_COMPRESSED_RGBA_S3TC_DXT5_EXT"; break;
			// GL_3DFX_texture_compression_FXT1
		case 0x86B0: return "GL_COMPRESSED_RGB_FXT1_3DFX"; break;
		case 0x86B1: return "GL_COMPRESSED_RGBA_FXT1_3DFX"; break;
			// GL_IMG_texture_compression_pvrtc
		case 0x8C00: return "GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG"; break;
		case 0x8C01: return "GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG"; break;
		case 0x8C02: return "GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG"; break;
		case 0x8C03: return "GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG"; break;
			// GL_OES_compressed_ETC1_RGB8_texture
		case 0x8D64: return "GL_ETC1_RGB8_OES"; break;
			// GL_ARB_texture_compression_rgtc
		case 0x8DBB: return "GL_COMPRESSED_RED_RGTC1"; break;
		case 0x8DBC: return "GL_COMPRESSED_SIGNED_RED_RGTC1"; break;
		case 0x8DBD: return "GL_COMPRESSED_RG_RGTC2"; break;
		case 0x8DBE: return "GL_COMPRESSED_SIGNED_RG_RGTC2"; break;
			// GL_ARB_texture_compression_bptc
		case 0x8E8C: return "GL_COMPRESSED_RGBA_BPTC_UNORM_ARB"; break;
		case 0x8E8D: return "GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB"; break;
		case 0x8E8E: return "GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT_ARB"; break;
		case 0x8E8F: return "GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_ARB"; break;
			// GL_ARB_ES3_compatibility
		case 0x9274: return "GL_COMPRESSED_RGB8_ETC2"; break;
		case 0x9275: return "GL_COMPRESSED_SRGB8_ETC2"; break;
		case 0x9276: return "GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2"; break;
		case 0x9277: return "GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2"; break;
		case 0x9278: return "GL_COMPRESSED_RGBA8_ETC2_EAC"; break;
		case 0x9279: return "GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC"; break;
		case 0x9270: return "GL_COMPRESSED_R11_EAC"; break;
		case 0x9271: return "GL_COMPRESSED_SIGNED_R11_EAC"; break;
		case 0x9272: return "GL_COMPRESSED_RG11_EAC"; break;
		case 0x9273: return "GL_COMPRESSED_SIGNED_RG11_EAC"; break;
			// GL_KHR_texture_compression_astc_hdr
		case 0x93B0: return "GL_COMPRESSED_RGBA_ASTC_4x4_KHR"; break;
		case 0x93B1: return "GL_COMPRESSED_RGBA_ASTC_5x4_KHR"; break;
		case 0x93B2: return "GL_COMPRESSED_RGBA_ASTC_5x5_KHR"; break;
		case 0x93B3: return "GL_COMPRESSED_RGBA_ASTC_6x5_KHR"; break;
		case 0x93B4: return "GL_COMPRESSED_RGBA_ASTC_6x6_KHR"; break;
		case 0x93B5: return "GL_COMPRESSED_RGBA_ASTC_8x5_KHR"; break;
		case 0x93B6: return "GL_COMPRESSED_RGBA_ASTC_8x6_KHR"; break;
		case 0x93B7: return "GL_COMPRESSED_RGBA_ASTC_8x8_KHR"; break;
		case 0x93B8: return "GL_COMPRESSED_RGBA_ASTC_10x5_KHR"; break;
		case 0x93B9: return "GL_COMPRESSED_RGBA_ASTC_10x6_KHR"; break;
		case 0x93BA: return "GL_COMPRESSED_RGBA_ASTC_10x8_KHR"; break;
		case 0x93BB: return "GL_COMPRESSED_RGBA_ASTC_10x10_KHR"; break;
		case 0x93BC: return "GL_COMPRESSED_RGBA_ASTC_12x10_KHR"; break;
		case 0x93BD: return "GL_COMPRESSED_RGBA_ASTC_12x12_KHR"; break;
		case 0x93D0: return "GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR"; break;
		case 0x93D1: return "GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR"; break;
		case 0x93D2: return "GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR"; break;
		case 0x93D3: return "GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR"; break;
		case 0x93D4: return "GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR"; break;
		case 0x93D5: return "GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR"; break;
		case 0x93D6: return "GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR"; break;
		case 0x93D7: return "GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR"; break;
		case 0x93D8: return "GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR"; break;
		case 0x93D9: return "GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR"; break;
		case 0x93DA: return "GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR"; break;
		case 0x93DB: return "GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR"; break;
		case 0x93DC: return "GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR"; break;
		case 0x93DD: return "GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR"; break;
		default: return "GL_COMPRESSED_UNKNOWN"; break;
	}
}
#endif  // RLGL_SHOW_GL_DETAILS_INFO

// Get pixel data size in bytes (image or texture)
// NOTE: Size depends on pixel format
static int rlGetPixelDataSize(int width, int height, int format)
{
	int dataSize = 0;       // Size in bytes
	int bpp = 0;            // Bits per pixel

	switch (format)
	{
		case RL_PIXELFORMAT_UNCOMPRESSED_GRAYSCALE: bpp = 8; break;
		case RL_PIXELFORMAT_UNCOMPRESSED_GRAY_ALPHA:
		case RL_PIXELFORMAT_UNCOMPRESSED_R5G6B5:
		case RL_PIXELFORMAT_UNCOMPRESSED_R5G5B5A1:
		case RL_PIXELFORMAT_UNCOMPRESSED_R4G4B4A4: bpp = 16; break;
		case RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8: bpp = 32; break;
		case RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8: bpp = 24; break;
		case RL_PIXELFORMAT_UNCOMPRESSED_R32: bpp = 32; break;
		case RL_PIXELFORMAT_UNCOMPRESSED_R32G32B32: bpp = 32 * 3; break;
		case RL_PIXELFORMAT_UNCOMPRESSED_R32G32B32A32: bpp = 32 * 4; break;
		case RL_PIXELFORMAT_COMPRESSED_DXT1_RGB:
		case RL_PIXELFORMAT_COMPRESSED_DXT1_RGBA:
		case RL_PIXELFORMAT_COMPRESSED_ETC1_RGB:
		case RL_PIXELFORMAT_COMPRESSED_ETC2_RGB:
		case RL_PIXELFORMAT_COMPRESSED_PVRT_RGB:
		case RL_PIXELFORMAT_COMPRESSED_PVRT_RGBA: bpp = 4; break;
		case RL_PIXELFORMAT_COMPRESSED_DXT3_RGBA:
		case RL_PIXELFORMAT_COMPRESSED_DXT5_RGBA:
		case RL_PIXELFORMAT_COMPRESSED_ETC2_EAC_RGBA:
		case RL_PIXELFORMAT_COMPRESSED_ASTC_4x4_RGBA: bpp = 8; break;
		case RL_PIXELFORMAT_COMPRESSED_ASTC_8x8_RGBA: bpp = 2; break;
		default: break;
	}

	dataSize = width * height * bpp / 8;  // Total data size in bytes

	// Most compressed formats works on 4x4 blocks,
	// if texture is smaller, minimum dataSize is 8 or 16
	if ((width < 4) && (height < 4))
	{
		if ((format >= RL_PIXELFORMAT_COMPRESSED_DXT1_RGB) && (format < RL_PIXELFORMAT_COMPRESSED_DXT3_RGBA)) dataSize = 8;
		else if ((format >= RL_PIXELFORMAT_COMPRESSED_DXT3_RGBA) && (format < RL_PIXELFORMAT_COMPRESSED_ASTC_8x8_RGBA)) dataSize = 16;
	}

	return dataSize;
}

// Auxiliar math functions

// Get identity matrix
static Matrix rlMatrixIdentity(void)
{
	Matrix result = {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};

	return result;
}

// Get two matrix multiplication
// NOTE: When multiplying matrices... the order matters!
static Matrix rlMatrixMultiply(Matrix left, Matrix right)
{
	Matrix result = { 0 };

	result.m0 = left.m0 * right.m0 + left.m1 * right.m4 + left.m2 * right.m8 + left.m3 * right.m12;
	result.m1 = left.m0 * right.m1 + left.m1 * right.m5 + left.m2 * right.m9 + left.m3 * right.m13;
	result.m2 = left.m0 * right.m2 + left.m1 * right.m6 + left.m2 * right.m10 + left.m3 * right.m14;
	result.m3 = left.m0 * right.m3 + left.m1 * right.m7 + left.m2 * right.m11 + left.m3 * right.m15;
	result.m4 = left.m4 * right.m0 + left.m5 * right.m4 + left.m6 * right.m8 + left.m7 * right.m12;
	result.m5 = left.m4 * right.m1 + left.m5 * right.m5 + left.m6 * right.m9 + left.m7 * right.m13;
	result.m6 = left.m4 * right.m2 + left.m5 * right.m6 + left.m6 * right.m10 + left.m7 * right.m14;
	result.m7 = left.m4 * right.m3 + left.m5 * right.m7 + left.m6 * right.m11 + left.m7 * right.m15;
	result.m8 = left.m8 * right.m0 + left.m9 * right.m4 + left.m10 * right.m8 + left.m11 * right.m12;
	result.m9 = left.m8 * right.m1 + left.m9 * right.m5 + left.m10 * right.m9 + left.m11 * right.m13;
	result.m10 = left.m8 * right.m2 + left.m9 * right.m6 + left.m10 * right.m10 + left.m11 * right.m14;
	result.m11 = left.m8 * right.m3 + left.m9 * right.m7 + left.m10 * right.m11 + left.m11 * right.m15;
	result.m12 = left.m12 * right.m0 + left.m13 * right.m4 + left.m14 * right.m8 + left.m15 * right.m12;
	result.m13 = left.m12 * right.m1 + left.m13 * right.m5 + left.m14 * right.m9 + left.m15 * right.m13;
	result.m14 = left.m12 * right.m2 + left.m13 * right.m6 + left.m14 * right.m10 + left.m15 * right.m14;
	result.m15 = left.m12 * right.m3 + left.m13 * right.m7 + left.m14 * right.m11 + left.m15 * right.m15;

	return result;
}

#endif  // RLGL_IMPLEMENTATION
