#include "renderer.h"

#include "io.h"
#include "font.h"
#include "utf.h"
#include "string2.h"


// TODO(RendererReorganization): This needs to go as soon as the renderer is better struktured
#include "opengl.h"



// NOTE: stupid defines somewhere...
#undef near
#undef far

M4 ortho(r32 width, r32 height) {
    r32 const near = -1.0f;
    r32 const far  =  1.0f;

    M4 proj = {};
    proj.value[ 0] = 2.0f / width;
    proj.value[ 5] = 2.0f / -height;
    proj.value[10] = -2.0f / (far - near);
    proj.value[12] = -1.0f;
    proj.value[13] = 1.0f;
    proj.value[14] = -(far + near) / (far - near);
    proj.value[15] = 1.0f;

    return proj;
}

M4 perspective_lh(r32 fov, r32 aspect, r32 near, r32 far) {
    r32 tan_half_fov = tan(fov * 0.5f);

    M4 mat = {0};
    mat.value[0]  = 1.0f / (aspect * tan_half_fov);
    mat.value[5]  = 1.0f / tan_half_fov;
    mat.value[10] = (far + near) / (far - near);
    mat.value[11] = 1.0f;
    mat.value[14] = (-2.0f * far * near) / (far - near);

    return mat;
}

M4 perspective_rh(r32 fov, r32 aspect, r32 near, r32 far) {
	r32 tan_half_fov = tan(fov * 0.5f);

	M4 mat = {};
	mat.value[0]  = 1.0f / (aspect * tan_half_fov);
	mat.value[5]  = 1.0f / tan_half_fov;
	mat.value[10] = -(far + near) / (far - near);
	mat.value[11] = -1.0f;
	mat.value[14] = (-2.0f * far * near) / (far - near);

	return mat;
}


#ifdef USE_OPENGL
#include "opengl_renderer.cpp"
#endif

