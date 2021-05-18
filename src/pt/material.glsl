#pragma once

//struct MT0{vec4 albedo_rgba_unorm;float ior;float metallic;float roughness;float transmission;};vec4 _rvec4_unorm8(float in_block0){return unpackUnorm4x8(floatBitsToUint(in_block0));}float _rfloat(float in_block0){return in_block0;}MT0 _lMT0(int _b){MT0 m;
//#define _G0(X) _G(_b + (X))
//float _bk0=_G0(0);vec4 par2=_rvec4_unorm8(_bk0);m.albedo_rgba_unorm=par2;float _bk1=_G0(1);float par4=_rfloat(_bk1);m.ior=par4;float _bk2=_G0(2);float par5=_rfloat(_bk2);m.metallic=par5;float _bk3=_G0(3);float par6=_rfloat(_bk3);m.roughness=par6;float _bk4=_G0(4);float par7=_rfloat(_bk4);m.transmission=par7;return m;}
//#undef _G0
//#include <pbr.glsl>
//struct MT0s_t{pbr_matinfo_t mat;bool is_incoming;float ior1;float ior2;};MT0s_t MT0s;vec3 MT0_normal(vec3 n){return MT0s.is_incoming?n:-n;}void MT0l(MT0 m){MT0s.mat.albedo_rgba_unorm=color_make(m.albedo_rgba_unorm);MT0s.mat.ior=m.ior;MT0s.mat.roughness=m.roughness;MT0s.mat.metallic=m.metallic;MT0s.mat.transmission=m.transmission;}void MT0r(vec3 point,vec2 uv,vec3 normal,vec3 towards_light,vec3 towards_viewer,out vec3 reflectance,out float pdf){MT0s.is_incoming=dot(normal,-towards_viewer)<0;MT0s.ior1=MT0s.is_incoming?1.0:MT0s.mat.ior;MT0s.ior2=MT0s.is_incoming?MT0s.mat.ior:1.0;brdf_result_t ev;pbr_eval(MT0s.mat,towards_viewer,towards_light,MT0_normal(normal),MT0s.ior1,MT0s.ior2,ev);reflectance=ev.reflectance;pdf=ev.pdf;}vec3 MT0c(vec2 random,vec3 towards_viewer,vec3 normal){MT0s.is_incoming=dot(normal,-towards_viewer)<0;MT0s.ior1=MT0s.is_incoming?1.0:MT0s.mat.ior;MT0s.ior2=MT0s.is_incoming?MT0s.mat.ior:1.0;return pbr_resample(random,MT0s.mat,towards_viewer,MT0_normal(normal),MT0s.ior1,MT0s.ior2);}int _cmt=-1;void material_load(int i){_cmt=i;switch(_MTY(_cmt)){case 0:MT0l(_lMT0(_MBO(i)));break;}}void material_sample(vec3 point,vec2 uv,vec3 normal,vec3 towards_light,vec3 towards_viewer,out vec3 reflectance,out float pdf){reflectance=vec3(1,0,0);pdf=1;switch(_MTY(_cmt)){case 0:MT0r(point,uv,normal,towards_light,towards_viewer,reflectance,pdf);break;}}vec3 material_continue_ray(vec2 r,vec3 v,vec3 n){switch(_MTY(_cmt)){case 0:return MT0c(r,v,n);}return vec3(0);}

MYRT_INJECT_MATERIAL_CODE_HERE

