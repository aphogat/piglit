// [config]
// expect_result: pass
// glsl_version: 3.30
// require_extensions: GL_ARB_explicit_uniform_location
// [end config]

#version 330
#extension GL_ARB_explicit_uniform_location: require

#if !defined GL_ARB_explicit_uniform_location
#  error GL_ARB_explicit_uniform_location is not defined
#elif GL_ARB_explicit_uniform_location != 1
#  error GL_ARB_explicit_uniform_location is not equal to 1
#endif

/* Some compilers generate spurious errors if a shader does not contain
 * any code or declarations.
 */
int foo(void) { return 1; }
