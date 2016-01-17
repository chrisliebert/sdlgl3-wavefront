#ifndef _MATERIAL_H_
#define _MATERIAL_H_

#include "Common.h"

typedef struct
{
    // Material name
    char name[64];
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
    char ambientTexName[64];
    char diffuseTexName[64];
    char specularTexName[64];
    char normalTexName[64];
} Material;

#endif // _MATERIAL_H_
