/*
 * Copyright © 2013 LunarG, Inc.
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
 *
 * Author: Jon Ashburn <jon@lunarg.com>
 */

/**
 * Tests GL_ARB_viewport_array validity for indices.
 * Use both valid and invalid parameters (index, first, count)
 * for these new API entry points:
 * glScissorArrayv, glScissorIndexed, glScissorIndexedv
 *
 * Also test SCISSOR_TEST indices with glEnablei and others.
 */

#include "piglit-util-gl.h"
#include <stdarg.h>

PIGLIT_GL_TEST_CONFIG_BEGIN

	config.supports_gl_compat_version = 32;
	config.supports_gl_core_version = 32;
	config.supports_gl_es_version = 31;

	config.window_visual = PIGLIT_GL_VISUAL_RGBA | PIGLIT_GL_VISUAL_DOUBLE;

PIGLIT_GL_TEST_CONFIG_END

/**
 * Test that ScissorArrayv, ScissorIndexed(v), GetIntegeri_v give the
 * "expected_error" gl error.  Given the values for "first" and "count"
 * or "index" in range [first, first+count).
 */
static bool
check_sc_index(GLuint first, GLsizei count, GLenum expected_error)
{
	static const GLint sv[] = {0, 10, 20, 35};
	GLint *mv, svGet[4];
	unsigned int i;
	bool pass = true;
	const unsigned int numIterate = (expected_error == GL_NO_ERROR)
		? count : 1;

	mv = malloc(sizeof(GLint) * 4 * count);
	if (mv == NULL)
		return false;
	for (i = 0; i < count; i++) {
		mv[i * 4] = sv[0];
		mv[i * 4 + 1] = sv[1];
		mv[i * 4 + 2] = sv[2];
		mv[i * 4 + 3] = sv[3];
	}
	glScissorArrayv(first, count, mv);
	free(mv);
	pass = piglit_check_gl_error(expected_error) && pass;

	/* only iterate multiple indices for no error case */
	for (i = count; i > count - numIterate; i--) {
		glScissorIndexed(first + i - 1, sv[0], sv[1], sv[2], sv[3]);
		pass = piglit_check_gl_error(expected_error) && pass;

		glGetIntegeri_v(GL_SCISSOR_BOX, first + i - 1, svGet);
		pass = piglit_check_gl_error(expected_error) && pass;

		glEnablei(GL_SCISSOR_TEST, first + i - 1);
		pass = piglit_check_gl_error(expected_error) && pass;

		glDisablei(GL_SCISSOR_TEST, first + i - 1);
		pass = piglit_check_gl_error(expected_error) && pass;

		glIsEnabledi(GL_SCISSOR_TEST, first + i - 1);
		pass = piglit_check_gl_error(expected_error) && pass;
	}

	return pass;
}

/**
 * Test first + count or index valid invalid values.
 * Valid range is 0 thru (MAX_VIEWPORTS-1).
 * Also test the Enable, Disable, IsEnabled  with invalid index.
 */
static bool
test_scissor_indices(GLint maxVP)
{
	bool pass = true;

	/**
	 * valid largest range Scissor index
	 * OpenGL Core 4.3 Spec, section 17.3.2 ref:
	 *     "ScissorArrayv defines a set of scissor rectangles that are
	 *     each applied to the corresponding viewport (see section 13.6.1).
	 *     first specifies the index of the first scissor rectangle to
	 *     modify, and count specifies the number of scissor rectangles."
	 */
	if (!check_sc_index(0, maxVP, GL_NO_ERROR)) {
		printf("Got error for valid scissor range, max=%u\n", maxVP);
		pass = false;
	}
	/**
	 * invalid count + first index for Scissor index
	 * OpenGL Core 4.3 Spec, section 17.3.2 ref:
	 *     "An INVALID_VALUE error is generated by ScissorArrayv if
	 *     first +count is greater than the value of MAX_VIEWPORTS."
	 *     "If the viewport index specified to Enablei, Disablei or
	 *     IsEnabledi is greater or equal to the value of
	 *     MAX_VIEWPORTS, then an INVALID_VALUE error is generated."
	 */
	if (!check_sc_index(maxVP - 4, 5, GL_INVALID_VALUE)) {
		printf("Wrong error for invalid Scissor first index\n");
		pass = false;
	}

	return pass;
}

enum piglit_result
piglit_display(void)
{
	return PIGLIT_FAIL;
}

void
piglit_init(int argc, char **argv)
{
	bool pass = true;
	GLint maxVP;

#ifdef PIGLIT_USE_OPENGL
	piglit_require_extension("GL_ARB_viewport_array");
#else
	piglit_require_extension("GL_OES_viewport_array");
#endif

	glGetIntegerv(GL_MAX_VIEWPORTS, &maxVP);
	if (!piglit_check_gl_error(GL_NO_ERROR)) {
		printf("GL error prior to ScissorArray testing\n");
		piglit_report_result(PIGLIT_FAIL);
	} else {
		pass = test_scissor_indices(maxVP) && pass;
		pass = piglit_check_gl_error(GL_NO_ERROR) && pass;
		piglit_report_result(pass ? PIGLIT_PASS : PIGLIT_FAIL);
	}
}
