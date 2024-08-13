#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <assert.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>

#define OUTPUT_FILENAME "out.bin"

#define VERTEX_SHADER \
	"                                            \
attribute vec4 vPosition;                                               \
                                                                        \
void main(void) {                                                       \
   gl_Position = vPosition;                                             \
}                                                                       \
"

#define FRAGMENT_SHADER \
	"                                            \
precision mediump float;                                                \
                                                                        \
void main(void) {                                                       \
   gl_FragColor = vec4(0.0, 0.75, 0.65, 1.0);                           \
}                                                                       \
"

typedef struct _ContextData {
	int width;
	int height;
	GLuint program;
	EGLSurface surface;
	EGLDisplay display;
	EGLContext context;
	EGLConfig config;
} ContextData;

void dump(ContextData *data)
{
	long long buffer_size = data->width * data->height * 4;
	GLubyte *buffer = malloc(buffer_size);
	int fd;

	glReadPixels(0, 0, data->width, data->height, GL_RGBA, GL_UNSIGNED_BYTE,
		     buffer);

	fd = open(OUTPUT_FILENAME, O_CREAT | O_WRONLY | O_TRUNC,
		  S_IRUSR | S_IWUSR);
	write(fd, buffer, buffer_size);
	free(buffer);
}

void draw(ContextData *data)
{
	// clang-format off
	GLfloat vVertices[] = {
		0.0f, 0.5f, 0.0f,
		-0.5f, -0.5f, 0.0f,
		0.5f, -0.5f, 0.0f,
	};
	// clang-format on

	// Set the viewport
	glViewport(0, 0, data->width, data->height);
	// Clear the color buffer
	glClear(GL_COLOR_BUFFER_BIT);
	// Use the program object
	glUseProgram(data->program);
	// Load the vertex data
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vVertices);
	assert(glGetError() == GL_NO_ERROR);
	glEnableVertexAttribArray(0);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	eglSwapBuffers(data->display, data->surface);
}

GLuint setup_shader(ContextData *data, const char *shader_source,
		    GLint shader_type)
{
	GLuint shader = glCreateShader(shader_type);
	assert(glGetError() == GL_NO_ERROR);

	glShaderSource(shader, 1, &shader_source, NULL);
	assert(glGetError() == GL_NO_ERROR);

	glCompileShader(shader);
	assert(glGetError() == GL_NO_ERROR);

	glAttachShader(data->program, shader);
	assert(glGetError() == GL_NO_ERROR);

	return shader;
}

bool egl_create_surface(ContextData *data)
{
	// clang-format off
	EGLint surface_attribs[] = {
		EGL_WIDTH, data->width,
		EGL_HEIGHT, data->height,
		EGL_NONE,
	};
	// clang-format on

	data->surface = eglCreatePbufferSurface(data->display, data->config,
						surface_attribs);
	return (data->surface != EGL_NO_SURFACE);
}

bool egl_create_context(ContextData *data)
{
	// clang-format off
	static const EGLint context_attribs[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE,
	};
	// clang-format on

	data->context = eglCreateContext(data->display, data->config,
					 EGL_NO_CONTEXT, context_attribs);
	return (data->context != EGL_NO_CONTEXT);
}

bool egl_choose_config(ContextData *data)
{
	EGLint count;

	// clang-format off
	static const EGLint config_attribs[] = {
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_ALPHA_SIZE, 8,
		EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_NONE,
	};
	// clang-format on

	return eglChooseConfig(data->display, config_attribs, &data->config, 1,
			       &count);
}

bool egl_init(ContextData *data)
{
	/* setup egl for surfaceless rendering */
	data->display = eglGetPlatformDisplay(EGL_PLATFORM_SURFACELESS_MESA,
					      EGL_DEFAULT_DISPLAY, NULL);
	assert(data->display != NULL);

	return eglInitialize(data->display, NULL, NULL);
}

int32_t main(void)
{
	bool res;
	ContextData data = { .width = 1920, .height = 1080 };

	/* choose a display under the surfacless platform */
	res = egl_init(&data);
	assert(res);

	/* pick a known universal config to play with */
	res = egl_choose_config(&data);
	assert(res);

	/* bind the gles api to this thread */
	res = eglBindAPI(EGL_OPENGL_ES_API);
	assert(res);

	/* create a context for the given config */
	res = egl_create_context(&data);
	assert(res);

	/* create a pbuffer to render to */
	res = egl_create_surface(&data);
	assert(res);

	/* make the pbuffer the current surface */
	res = eglMakeCurrent(data.display, data.surface, data.surface,
			     data.context);
	assert(res);

	/* create a program to attach shaders to */
	data.program = glCreateProgram();

	/* setup a vertex shader and register it with the program */
	GLuint vertex_shader =
		setup_shader(&data, VERTEX_SHADER, GL_VERTEX_SHADER);

	/* setup a fragment shader and register it with the program */
	GLuint fragment_shader =
		setup_shader(&data, FRAGMENT_SHADER, GL_FRAGMENT_SHADER);

	/* bind vPosition to index 0 for our vertex shader */
	glBindAttribLocation(data.program, 0, "vPosition");
	assert(glGetError() == GL_NO_ERROR);

	/* link the program to test shaders */
	glLinkProgram(data.program);
	assert(glGetError() == GL_NO_ERROR);

	/* flag shaders for deletion */
	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);

	/* render a frame */
	draw(&data);
	assert(glGetError() == GL_NO_ERROR);

	/* dump frame contents to file */
	dump(&data);

	printf("Application dispatched and finished successfully\n");

	/* free stuff */
	glDeleteProgram(data.program);
	eglDestroyContext(data.display, data.context);
	eglTerminate(data.display);
	return 0;
}
