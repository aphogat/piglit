/*
 * Copyright © 2012 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

/** \file ms-winsys-resolve-2.cpp
 *
 * This test verifies that a multisampled resolve blit produces same output in
 * following two rendering scenarios:
 * 1. Blit multisample buffer to window system framebuffer.
 * 2. Blit multisample buffer to a single sample buffer followed
 *    by a blit from single sample buffer to window system framebuffer with
 *    nearest filtering.
 */

#include "common.h"
#include <unistd.h>

const int pattern_width = 256; const int pattern_height = 256;
int num_samples;

PIGLIT_GL_TEST_CONFIG_BEGIN

	config.supports_gl_compat_version = 10;

	config.window_width = pattern_width*2;
	config.window_height = pattern_height;
	config.window_visual = PIGLIT_GL_VISUAL_DOUBLE | PIGLIT_GL_VISUAL_RGBA | PIGLIT_GL_VISUAL_ALPHA;

PIGLIT_GL_TEST_CONFIG_END

static Fbo multisampled_fbo_1, singlesampled_fbo;
GLuint prog, vao, vertex_buf;
static TestPattern *test_pattern;

static void
print_usage_and_exit(char *prog_name)
{
	printf("Usage: %s <num_samples>\n", prog_name);
	piglit_report_result(PIGLIT_FAIL);
}

void
compile_ms_scaled_blit_shader(void)
{
	static const char *vert =
		"#version 130\n"
		"uniform mat4 proj;\n"
		"in vec2 pos;\n"
		"in vec2 texCoord;\n"
		"out vec2 textureCoord;\n"
		"void main()\n"
		"{\n"
		"  gl_Position = proj * vec4(pos, 0.0, 1.0);\n"
		"  textureCoord = texCoord;\n"
		"}\n";

	static const char *frag =
		"#version 130\n"
		"#extension GL_ARB_texture_multisample : require\n"
		"in vec2 textureCoord;\n"
		"uniform sampler2DMS samp;\n"
		"out vec4 out_color;\n"
		"void main()\n"
		"{\n"
		"int i;\n"
		"out_color = vec4(0.0, 0.0, 0.0, 0.0);\n"
		"  for (i = 0; i < 4; i++)\n"
		"    out_color =  out_color + texelFetch(samp, ivec2(textureCoord), i);\n"
		"out_color = out_color / 4;\n"
		"}\n";

	/* Compile program */
	prog = glCreateProgram();
	GLint vs = piglit_compile_shader_text(GL_VERTEX_SHADER, vert);
	glAttachShader(prog, vs);
	piglit_check_gl_error(GL_NO_ERROR);
	GLint fs = piglit_compile_shader_text(GL_FRAGMENT_SHADER, frag);
	glAttachShader(prog, fs);
	glBindAttribLocation(prog, 0, "pos");
	glBindAttribLocation(prog, 1, "texCoord");
	glLinkProgram(prog);
	if (!piglit_link_check_status(prog)) {
		piglit_report_result(PIGLIT_FAIL);
	}

	/* Set up vertex array object */
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	/* Set up vertex input buffer */
	glGenBuffers(1, &vertex_buf);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buf);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float),
			      (void *) 0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float),
			      (void *) (2*sizeof(float)));

	/* Set up element input buffer to tesselate a quad into
	 * triangles
	 */
	unsigned int indices[6] = { 0, 1, 2, 0, 2, 3 };
	GLuint element_buf;
	glGenBuffers(1, &element_buf);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element_buf);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices,
		     GL_STATIC_DRAW);
}

void
do_multisample_scaled_blit(const Fbo *src_fbo)
{
	const float proj[4][4] = {
		{ 1, 0, 0, 0 },
		{ 0, 1, 0, 0 },
		{ 0, 0, 1, 0 },
		{ 0, 0, 0, 1 }};

	float w = pattern_width;
	float h = pattern_height;

	float vertex_data[4][4] = {
		{ -1, -1, 0, 0 },
		{ -1,  1, 0, h },
		{  1,  1, w, h },
		{  1, -1, w, 0 }};

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, src_fbo->color_tex);

	glUseProgram(prog);
	glBindVertexArray(vao);

	/* Set up uniforms */
	glUseProgram(prog);
	glUniformMatrix4fv(glGetUniformLocation(prog, "proj"), 1, GL_TRUE, &proj[0][0]);
	glUniform1i(glGetUniformLocation(prog, "samp"), 0);

	glBindBuffer(GL_ARRAY_BUFFER, vertex_buf);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data), vertex_data,
		     GL_STREAM_DRAW);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void *) 0);
}



void
piglit_init(int argc, char **argv)
{
	if (argc != 2)
		print_usage_and_exit(argv[0]);

	/* 1st arg: num_samples */
	char *endptr = NULL;
	num_samples = strtol(argv[1], &endptr, 0);
	if (endptr != argv[1] + strlen(argv[1]))
		print_usage_and_exit(argv[0]);

	piglit_require_gl_version(21);
	piglit_require_extension("GL_ARB_framebuffer_object");
	piglit_require_extension("GL_ARB_vertex_array_object");

	/* Skip the test if num_samples > GL_MAX_SAMPLES */
	GLint max_samples;
	glGetIntegerv(GL_MAX_SAMPLES, &max_samples);
	if (num_samples > max_samples)
		piglit_report_result(PIGLIT_SKIP);

	singlesampled_fbo.setup(FboConfig(0,
					  pattern_width,
					  pattern_height));
	FboConfig msConfig(num_samples, pattern_width, pattern_height);
	msConfig.attach_texture = true;
	multisampled_fbo_1.setup(msConfig);

	test_pattern = new Triangles();
	test_pattern->compile();

	compile_ms_scaled_blit_shader();
	if (!piglit_check_gl_error(GL_NO_ERROR)) {
		piglit_report_result(PIGLIT_FAIL);
	}
}

enum piglit_result
piglit_display()
{
	bool pass = true;
	float w = pattern_width;
	float h = pattern_height;

	/* Draw the test pattern into the framebuffer with multisample
	 * texture attachment.
	 */
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, multisampled_fbo_1.handle);
	glViewport(0, 0, w, h);
	test_pattern->draw(TestPattern::no_projection);

	printf("Left: ms fbo to winsys fbo using shader.\n"
	       "Right: ms fbo to winsys fbo.\n");

	/* Use shader program to blit in to winsys framebuffer.*/
	glBindFramebuffer(GL_READ_FRAMEBUFFER, multisampled_fbo_1.handle);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, piglit_winsys_fbo);
	glViewport(0, 0, w , h );
	do_multisample_scaled_blit(&multisampled_fbo_1);

	/* Do resolve of multisampled_fbo_1 to piglit_winsys_fbo */
	glBindFramebuffer(GL_READ_FRAMEBUFFER, multisampled_fbo_1.handle);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, piglit_winsys_fbo);
	glBlitFramebuffer(0, 0, w, h,
			  pattern_width, 0, pattern_width + w , h ,
			  GL_COLOR_BUFFER_BIT, GL_NEAREST);

	pass = piglit_check_gl_error(GL_NO_ERROR) && pass;
	glBindFramebuffer(GL_READ_FRAMEBUFFER, piglit_winsys_fbo);
	piglit_present_results();
	pass = piglit_probe_rect_halves_equal_rgba(0, 0, 2 * pattern_width,
						   pattern_height) && pass;
	glClear(GL_COLOR_BUFFER_BIT);
	return pass ? PIGLIT_PASS : PIGLIT_FAIL;
}
