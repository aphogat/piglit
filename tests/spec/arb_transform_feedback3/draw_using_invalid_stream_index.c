/*
 * Copyright © 2013 Intel Corporation
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

#include "piglit-util-gl.h"
#include "xfb3_common.h"

/**
 * @file draw_using_invalid_stream_index.c
 *
 * "The error INVALID_VALUE is generated by DrawTransformFeedbackStream
 *  if <stream> is greater than or equal to the value of MAX_VERTEX_STREAMS."
 */

PIGLIT_GL_TEST_CONFIG_BEGIN

	config.supports_gl_compat_version = 32;
	config.supports_gl_core_version = 32;

PIGLIT_GL_TEST_CONFIG_END

static const char gs_text[] =
        "#version 150\n"
        "#extension GL_ARB_gpu_shader5 : enable\n"
        "layout(points, max_vertices = 1, stream = 0) out;\n"
        "\n"
        "void main() {\n"
        "    gl_Position = gl_in[0].gl_Position;\n"
        "    EmitStreamVertex(0);\n"
        "}\n";

void
piglit_init(int argc, char **argv)
{
        static const float verts[] = {
                -1.0f, -1.0f,
                 1.0f, -1.0f,
                -1.0f,  1.0f,
                 1.0f,  1.0f,
        };
	GLuint vao, buf, tfb;
	GLint max_stream;
	bool pass;

	piglit_require_extension("GL_ARB_transform_feedback3");
	piglit_require_extension("GL_ARB_gpu_shader5");

	glGetIntegerv(GL_MAX_VERTEX_STREAMS, &max_stream);
	if (!piglit_check_gl_error(GL_NO_ERROR)) {
		printf("failed to resolve the maximum number of streams\n");
		piglit_report_result(PIGLIT_FAIL);
	}

	piglit_build_simple_program_multiple_shaders(
			GL_VERTEX_SHADER, vs_pass_thru_text,
			GL_GEOMETRY_SHADER, gs_text, 0);

	/* Test is run under desktop OpenGL 3.2 -> use of VAOs is required */
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

        glGenBuffers(1, &buf);
        glBindBuffer(GL_ARRAY_BUFFER, buf);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);
        glEnableVertexAttribArray(0);

	glGenTransformFeedbacks(1, &tfb);
	glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, tfb);

	glDrawTransformFeedbackStream(GL_TRIANGLE_STRIP, tfb, max_stream);

	pass = piglit_check_gl_error(GL_INVALID_VALUE);

	glDeleteTransformFeedbacks(1, &tfb);

	piglit_report_result(pass ? PIGLIT_PASS : PIGLIT_FAIL);
}

enum piglit_result
piglit_display(void)
{
	/* Should never be reached */
	return PIGLIT_FAIL;
}
