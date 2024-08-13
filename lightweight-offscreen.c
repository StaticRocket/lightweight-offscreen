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

int32_t main(int32_t argc, char *argv[])
{
	(void)argc;
	(void)argv;
	bool res;
	EGLConfig cfg;
	EGLint count;
	ContextData data = { .width = 1920, .height = 1080 };

	/* setup EGL from the GBM device */
	data.display = eglGetPlatformDisplay(EGL_PLATFORM_SURFACELESS_MESA,
					     EGL_DEFAULT_DISPLAY, NULL);
	assert(data.display != NULL);

	res = eglInitialize(data.display, NULL, NULL);
	assert(res);

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

	res = eglChooseConfig(data.display, config_attribs, &cfg, 1, &count);
	assert(res);

	res = eglBindAPI(EGL_OPENGL_ES_API);
	assert(res);

	// clang-format off
	static const EGLint context_attribs[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE,
	};
	// clang-format on

	data.context = eglCreateContext(data.display, cfg, EGL_NO_CONTEXT,
					context_attribs);
	assert(data.context != EGL_NO_CONTEXT);

	// clang-format off
	EGLint surface_attribs[] = {
		EGL_WIDTH, data.width,
		EGL_HEIGHT, data.height,
		EGL_NONE,
	};
	// clang-format on

	data.surface =
		eglCreatePbufferSurface(data.display, cfg, surface_attribs);
	assert(data.surface != EGL_NO_SURFACE);

	res = eglMakeCurrent(data.display, data.surface, data.surface,
			     data.context);
	assert(res);

	data.program = glCreateProgram();

	/* setup a vertex shader */
	GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	assert(glGetError() == GL_NO_ERROR);

	const char *shader_source = VERTEX_SHADER;
	glShaderSource(vertex_shader, 1, &shader_source, NULL);
	assert(glGetError() == GL_NO_ERROR);

	glCompileShader(vertex_shader);
	assert(glGetError() == GL_NO_ERROR);

	glAttachShader(data.program, vertex_shader);
	assert(glGetError() == GL_NO_ERROR);

	/* setup a fragment shader */
	GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	assert(glGetError() == GL_NO_ERROR);

	const char *fshader_source = FRAGMENT_SHADER;
	glShaderSource(fragment_shader, 1, &fshader_source, NULL);
	assert(glGetError() == GL_NO_ERROR);

	glCompileShader(fragment_shader);
	assert(glGetError() == GL_NO_ERROR);

	glAttachShader(data.program, fragment_shader);
	assert(glGetError() == GL_NO_ERROR);

	/* continue */
	glBindAttribLocation(data.program, 0, "vPosition");
	assert(glGetError() == GL_NO_ERROR);

	glLinkProgram(data.program);
	assert(glGetError() == GL_NO_ERROR);

	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);

	draw(&data);
	assert(glGetError() == GL_NO_ERROR);

	dump(&data);

	printf("Application dispatched and finished successfully\n");

	/* free stuff */
	glDeleteProgram(data.program);
	eglDestroyContext(data.display, data.context);
	eglTerminate(data.display);
	return 0;
}
