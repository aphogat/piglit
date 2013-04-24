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

/** \file ms-winsys-resolve.cpp
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

	FboConfig Config(num_samples, pattern_width, pattern_height);
	Config.attach_texture = true;
	multisampled_fbo_1.setup(Config);

	Config.color_internalformat = GL_SRGB_ALPHA;
	Config.num_samples = 0;
	singlesampled_fbo.setup(Config);

	test_pattern = new Triangles();
	test_pattern->compile();

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

	printf("Left: ms fbo to ss fbo to winsys fbo.\n"
	       "Right: ms fbo to winsys fbo.\n");

	/* Resolve the multisampled_fbo_1 by blitting it into the single-sampled
	 * buffer with out scaling.
	 */
	glBindFramebuffer(GL_READ_FRAMEBUFFER, multisampled_fbo_1.handle);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, singlesampled_fbo.handle);
	glBlitFramebuffer(0, 0, w, h,
			  0, 0, w, h,
			  GL_COLOR_BUFFER_BIT, GL_NEAREST);
	/* Blit the resulting image to the screen without scaling.*/
	glBindFramebuffer(GL_READ_FRAMEBUFFER, singlesampled_fbo.handle);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, piglit_winsys_fbo);
	glBlitFramebuffer(0, 0, w, h,
			  0, 0, w , h ,
			  GL_COLOR_BUFFER_BIT, GL_NEAREST);

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
