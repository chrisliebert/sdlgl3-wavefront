#ifndef _MATERIAL_H_
#define _MATERIAL_H_

#include "Common.h"

typedef struct
{
    // Material name
    char name[MAX_MATERIAL_NAME_STRING_LENGTH];
    float ambient[3];
    float diffuse[3];
    float specular[3];
    float transmittance[3];
    float emission[3];
    float shininess;
    float ior;      // index of refraction
    float dissolve; // 1 == opaque; 0 == fully transparent
    // illumination model (see http://www.fileformat.info/format/material/)
    int illum;
    //Texture file names
    char ambientTexName[MAX_MATERIAL_NAME_STRING_LENGTH];
    char diffuseTexName[MAX_MATERIAL_NAME_STRING_LENGTH];
    char specularTexName[MAX_MATERIAL_NAME_STRING_LENGTH];
    char normalTexName[MAX_MATERIAL_NAME_STRING_LENGTH];
} Material;

typedef struct
{
	unsigned width, height;
	unsigned bpp;
	int mode;
	unsigned char* data;
} Texture;

#endif // _MATERIAL_H_
