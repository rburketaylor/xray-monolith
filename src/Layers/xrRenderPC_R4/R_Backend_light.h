#ifndef R_BACKEND_LIGHT_H_INCLUDED
#define R_BACKEND_LIGHT_H_INCLUDED

//	Handles for the constants the light accumulation passes write per light.
//	They belong to the constant table that is bound when they are resolved, so
//	they are dropped whenever another table is set (like R_LOD/R_xforms/R_hemi)
//	and resolved lazily from the table in use.
class R_light_constants
{
public:
	R_constant* c_Ldynamic_pos;
	R_constant* c_Ldynamic_color;
	R_constant* c_Ldynamic_dir;
	R_constant* c_m_texgen;
	R_constant* c_m_texgen_J;
	R_constant* c_m_shadow;
	R_constant* c_m_lmap;
	R_constant* c_vMinBounds;
	R_constant* c_vMaxBounds;
	R_constant* c_FrustumClipPlane;
	R_constant* c_sss_id;

public:
	R_light_constants() { unmap(); }

	void unmap()
	{
		c_Ldynamic_pos = 0;
		c_Ldynamic_color = 0;
		c_Ldynamic_dir = 0;
		c_m_texgen = 0;
		c_m_texgen_J = 0;
		c_m_shadow = 0;
		c_m_lmap = 0;
		c_vMinBounds = 0;
		c_vMaxBounds = 0;
		c_FrustumClipPlane = 0;
		c_sss_id = 0;
	}
};

#endif // #ifndef R_BACKEND_LIGHT_H_INCLUDED
