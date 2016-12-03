#include "Common.h"
#include "Renderer.h"

void _checkForGLError(const char *file, int line)
{
	GLenum err (glGetError());
	static int numErrors = 0;
	while(err!=GL_NO_ERROR)
	{
		std::string error;
		numErrors++;
		switch(err)
		{
		case GL_INVALID_OPERATION:
			error="INVALID_OPERATION";
			break;
		case GL_INVALID_ENUM:
			error="INVALID_ENUM";
			break;
		case GL_INVALID_VALUE:
			error="INVALID_VALUE";
			break;
		case GL_OUT_OF_MEMORY:
			error="OUT_OF_MEMORY";
			break;
		case GL_INVALID_FRAMEBUFFER_OPERATION:
			error="INVALID_FRAMEBUFFER_OPERATION";
			break;
		}

		std::cerr << "GL_" << error.c_str() <<" - "<< file << ":" << line << std::endl;

		if(numErrors > 10)
		{
			exit(1);
		}

		err=glGetError();
	}
}

void checkForGLSLError(GLuint programId) {
	GLint result;
	glGetProgramiv(programId, GL_LINK_STATUS, &result);
	if(result == GL_FALSE)
	{
		GLint length;
		char *log;
		// get the program info log
		glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &length);
		log = new char[length];
		glGetProgramInfoLog(programId, length, &result, log);
		std::cout << "Unable to link shader: " << log << std::endl;
		delete log;
	}
}

// GLAD_DEBUG is only defined if the c-debug generator was used
#ifdef GLAD_DEBUG
// logs every gl call to the console
void pre_gl_call(const char *name, void *funcptr, int len_args, ...) {
    std::cout << "Calling: " << name << " (" << len_args << " arguments)" << std::endl;
}
#endif

void parseLine(std::string& line, std::map<std::string, std::string>& vars)
{
	const char* l = line.c_str();
	bool foundEquals = false;
	std::string lhs, rhs;

	// basic syntax check
	if(l[0] == '=') {
		std::cerr << "Invalid config line: " << line << std::endl;
		return;
	}

	for(int i=0; i<line.length(); i++) {
		char c = l[i];
		if(c == '#') break;
		if(c == '=') {
			foundEquals = true;
			continue;
		}
		if(c != ' ') {
			if(!foundEquals) {
				// save the lhs
				lhs += c;
			} else {
				// save the rhs
				rhs += c;
			}
		}
	}

	if(lhs.length() > 0) vars.insert(std::make_pair(lhs, rhs));
}

ConfigLoader::ConfigLoader(const char* _filename) {
	filename = _filename;
	std::string filePath(CONFIG_DIRECTORY);
	filePath += DIRECTORY_SEPARATOR + std::string(filename);
	std::ifstream fileStream(filePath.c_str());
	if (fileStream.is_open())
	{
		std::cout << "loaded " << filePath << std::endl;
		std::string line;
		while (std::getline(fileStream, line))
		{
			int pos;
			line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
			parseLine(line, vars);
		}
		fileStream.close();
	}
	else
	{
		std::cerr << "Unable to load shader source" << std::endl;
	}
}

ConfigLoader::~ConfigLoader() {
}

float ConfigLoader::getFloat(const char* var) {
	std::string v(var);
	return getFloat(v);
}

bool ConfigLoader::getBool(const char* var) {
	std::string v(var);
	return getBool(v);
}

bool ConfigLoader::getBool(std::string& var) {
	std::map<std::string, std::string>::iterator i = vars.find(var);
	if(i == vars.end()) {
		std::cerr << "Unable to load " << var << " from " << filename << std::endl;
		exit(5);
	}
	std::string s = i->second;

	if(s.compare("true") == 0 || s.compare("True") == 0) {
		return true;
	} else if(s.compare("false") == 0 || s.compare("False") == 0) {
		return false;
	}

	std::stringstream ss(s);
	bool val;

	while(ss >> val || !ss.eof()) {
		if(ss.fail()) {
			ss.clear();
			std::cerr << "Unable to parse variable " << var << " of " << s << " as bool." << std::endl;
			exit(6);
		}
	}

	return val;
}

float ConfigLoader::getFloat(std::string& var) {
	std::map<std::string, std::string>::iterator i = vars.find(var);
	if(i == vars.end()) {
		std::cerr << "Unable to load " << var << " from " << filename << std::endl;
		exit(5);
	}
	std::string s = i->second;
	std::stringstream ss(s);
	float val;

	while(ss >> val || !ss.eof()) {
		if(ss.fail()) {
			ss.clear();
			std::cerr << "Unable to parse variable " << var << " of " << s << " as float." << std::endl;
			exit(6);
		}
	}

	return val;
}

int ConfigLoader::getInt(const char* var) {
	std::string v(var);
	return getInt(v);
}

int ConfigLoader::getInt(std::string& var) {
	std::map<std::string, std::string>::iterator i = vars.find(var);
	if(i == vars.end()) {
		std::cerr << "Unable to load " << var << " from " << filename << std::endl;
		exit(5);
	}
	std::string s = i->second;
	std::stringstream ss(s);
	int val;

	while(ss >> val || !ss.eof()) {
		if(ss.fail()) {
			ss.clear();
			std::cerr << "Unable to parse variable " << var << " of " << s << " as integer." << std::endl;
			exit(6);
		}
	}

	return val;
}

std::string& ConfigLoader::getVar(const char* var)
{
	std::string v(var);
	return getVar(v);
}

std::string& ConfigLoader::getVar(std::string& var)
{
	std::map<std::string, std::string>::iterator i = vars.find(var);
	if(i == vars.end()) {
		std::cerr << "Unable to load " << var << " from " << filename << std::endl;
		exit(5);
	}
	return i->second;
}

bool ConfigLoader::hasVar(const char* var)
{
	std::string s(var);
	return hasVar(s);
}

bool ConfigLoader::hasVar(std::string& var) {
	return vars.find(var) != vars.end();
}

std::ostream& operator<<(std::ostream& os, ConfigLoader& dt)
{
	os << "|------- " << dt.filename << " -------|" << std::endl;
	std::map<std::string, std::string>::iterator it;
	for (it=dt.vars.begin(); it!=dt.vars.end(); ++it) {
		os << it->first << " <- " << it->second << '\n';
	}
	os << "|--------";
	for(unsigned i=0; i<strlen(dt.filename); i++) {
		os << "-";
	}
	os << "--------|" << std::endl;;
	return os;
}

std::ostream& operator<<(std::ostream& os, ConfigLoader* dt)
{
	// dereference pointer and call function with reference
	os << *dt;
	return os;
}

Renderer::Renderer()
{
	startPosition = 0;
	vao = vbo = ibo = 0;
	gpuProgram = 0;
	shadowProgram = 0;
	depthMapFBO = 0;
	shadowMap = 0;
	configLoader = new ConfigLoader("renderer.cfg");
	shadowsEnabled = configLoader->getBool("shadow.enabled");
	shadowWidth = configLoader->getInt("shadow.width");
	shadowHeight = configLoader->getInt("shadow.height");
	binCacheWriterThread = 0;

	int flags = IMG_INIT_JPG | IMG_INIT_PNG | IMG_INIT_TIF;
	int initted = IMG_Init(flags);
	if ((initted&flags) != flags)
	{
		std::cerr << IMG_GetError() << std::endl;
	}
}

Renderer::~Renderer()
{
	// Wait for cache writer thread to exit
	int cacheWriterThreadStatus = 0;
	SDL_WaitThread(binCacheWriterThread, &cacheWriterThreadStatus);
	if(cacheWriterThreadStatus != 0) {
		std::cerr << "Binary cache writer thread failed with status " << cacheWriterThreadStatus << std::endl;
	}

	if(sceneNodes.size() > 0)
	{
		for(int i=0; i<sceneNodes.size(); i++) {
			glDeleteTextures(1, &sceneNodes[i].diffuseTextureId);
		}
		glDeleteBuffers(1, &vbo);
		glDeleteBuffers(1, &ibo);
		glDeleteVertexArrays(1, &vao);
	}

	/* causes error on non-cached scene.
	for(std::map<std::string, Texture>::iterator it=textures.begin(); it!=textures.end(); ++it) {
		if(it->second.data) delete[] &*it->second.data;
	} */

	IMG_Quit();
	if(shadowProgram != NULL) delete shadowProgram;
	if(gpuProgram != NULL) delete gpuProgram;
	delete configLoader;
}

void Renderer::addMaterial(Material* material)
{
	materials[material->name] = *material;
}

void Renderer::addSceneNode(SceneNode* sceneNode)
{
	if(!sceneNode)
	{
		std::cerr << "Unable to add null sceneNode" << std::endl;
	}
	else
	{
		sceneNodes.push_back(*sceneNode);
	}
}

// Used to check file extension
bool hasEnding (std::string const &fullString, std::string const &ending)
{
	if (fullString.length() >= ending.length())
	{
		return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
	}
	else
	{
		return false;
	}
}

int getTextureMode(SDL_Surface* image)
{
	//todo: support BGR, BGRA images
	int mode = GL_RGB;
	if(image->format->BytesPerPixel == 4)
	{
		mode = GL_RGBA;
		//todo: set alpha flag
	}
	return mode;
}

Texture* textureFromSurface(SDL_Surface* image) {
	Texture* texture = new Texture;
	//TODO: delete this memory
		/*
		// This is still not working correctly (tga images still have wrong colors)
		std::string tga(".tga");
		if(hasEnding(fileNameStr, tga))
		{
			mode = GL_BGR;
			if(image->format->BytesPerPixel == 4)
			{
				mode = GL_BGRA;
			}
		}
		//todo: change to GL_BRG on .tga images
		 * */

		texture->mode = getTextureMode(image);
		texture->width = image->w;
		texture->height = image->h;
		texture->data = (unsigned char*) image->pixels;

		return texture;
}

void Renderer::addTexture(const char* textureFileName, GLuint* textureId, SDL_Surface* image) {
	Texture* texture = 0;

	// If texture not loaded and image is null, try to load the default blank texture
	if(!image)
	{
		//std::cerr << "Unable to load texture: " << textureFileName << std::endl;

		std::string bfileNameStr(TEXTURE_DIRECTORY);
		bfileNameStr += DIRECTORY_SEPARATOR;
		bfileNameStr += std::string("DEFAULT_BLANK_TEXTURE.png");

		if(textures.find(bfileNameStr) == textures.end()) {
			image = IMG_Load(bfileNameStr.c_str());
			if(!image) {
				std::cerr << "Error loading default blank texture DEFAULT_BLANK_TEXTURE.png" << std::endl;
				return;
			} else {
				std::cerr << "DEFAULT_BLANK_TEXTURE.png" << std::endl;//
				texture = textureFromSurface(image);
				textures[textureFileName] = *texture;
				textures[bfileNameStr] = *texture;
			}
		} else {
			// don't re-load the blank texture if it is already loaded
			texture = &textures[bfileNameStr];
		}
	} else {
		// image != null
		texture = textureFromSurface(image);
		textures[textureFileName] = *texture;
	}

	addTexture(textureFileName, textureId, texture);
}

void Renderer::addTexture(const char* textureFileName, GLuint* textureId, Texture* texture)
{
	glGenTextures(1, textureId);
	glBindTexture(GL_TEXTURE_2D, *textureId);
	glTexImage2D(GL_TEXTURE_2D, 0, texture->mode, texture->width, texture->height, 0, texture->mode, GL_UNSIGNED_BYTE, texture->data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void Renderer::addTexture(const char* textureFileName, GLuint* textureId)
{
	std::map<std::string, Texture>::iterator it = textures.find(std::string(textureFileName));
	if(it == textures.end()) {
		// if texture is not found in cache, load from file
		std::string fileNameStr(TEXTURE_DIRECTORY);
		std::string textureFileNameStr = std::string(textureFileName);
		fileNameStr += DIRECTORY_SEPARATOR;
		fileNameStr += textureFileName;
		SDL_Surface* image = IMG_Load(fileNameStr.c_str());
		addTexture(textureFileName, textureId, image);
	} else {
		addTexture(textureFileName, textureId, &it->second);
	}
}

// Used for debugging, TODO: replace with << operator
void printVertex(Vertex& v)
{
	std::cerr << "v " << v.vertex[0] << ", " << v.vertex[1] << ", " << v.vertex[2]
			<< '\t' << " n " << v.normal[0] << ", " << v.normal[1] << ", " << v.normal[2]
			<< '\t' << " t " << v.textureCoordinate[0] << ", " << v.textureCoordinate[1] << std::endl;
}

void Renderer::addWavefront(const char* fileName, glm::mat4 matrix)
{
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string modelDirectory(MODEL_DIRECTORY);
	modelDirectory += DIRECTORY_SEPARATOR;
	std::string fileNameStr(modelDirectory);
	fileNameStr += fileName;
	std::string err;
	bool noError = (tinyobj::LoadObj(shapes, materials, err, fileNameStr.c_str(), modelDirectory.c_str()), true, true);
	if(!noError)
	{
		std::cerr << err << std::endl;
		return;
	}

	for(size_t i=0; i<materials.size(); i++)
	{
		Material m;
		memcpy((void*)& m.ambient, (void*)& materials[i].ambient[0], sizeof(float)*3);
		memcpy((void*)& m.diffuse, (void*)& materials[i].diffuse[0], sizeof(float)*3);
		memcpy((void*)& m.emission, (void*)& materials[i].emission[0], sizeof(float)*3);
		memcpy((void*)& m.specular, (void*)& materials[i].specular[0], sizeof(float)*3);
		memcpy((void*)& m.transmittance, (void*) &materials[i].transmittance[0], sizeof(float)*3);
		memcpy((void*)& m.illum, (void*)& materials[i].illum, sizeof(int));
		memcpy((void*)& m.ior, (void*)& materials[i].ior, sizeof(float));
		memcpy((void*)& m.shininess, (void*)& materials[i].shininess, sizeof(float));
		memcpy((void*)& m.dissolve, (void*)& materials[i].dissolve, sizeof(float));

		strncpy(m.name, materials[i].name.c_str(), MAX_MATERIAL_NAME_STRING_LENGTH);
		strncpy(m.ambientTexName, materials[i].ambient_texname.c_str(), MAX_MATERIAL_NAME_STRING_LENGTH);
		strncpy(m.diffuseTexName, materials[i].diffuse_texname.c_str(), MAX_MATERIAL_NAME_STRING_LENGTH);
		strncpy(m.normalTexName, materials[i].specular_highlight_texname.c_str(), MAX_MATERIAL_NAME_STRING_LENGTH);
		strncpy(m.specularTexName, materials[i].specular_texname.c_str(), MAX_MATERIAL_NAME_STRING_LENGTH);

		addMaterial(&m);
	}

	for (size_t i = 0; i < shapes.size(); i++)
	{
		std::vector<Vertex> mVertexData;

		unsigned int materialId, lastMaterialId = 0;
		if(shapes[i].mesh.material_ids.size() > 0)
		{
			materialId = lastMaterialId = shapes[i].mesh.material_ids[0];
		}

		for(int j=0; j <shapes[i].mesh.indices.size(); j++)
		{
			if((j%3) == 0)
			{
				lastMaterialId = materialId;
				materialId = shapes[i].mesh.material_ids[j/3];

				if(materialId != lastMaterialId)
				{
					//new node
					SceneNode sceneNode;
					strncpy(&sceneNode.name[0], shapes[i].name.c_str(), MAX_NODE_NAME_STRING_LENGTH);
					strncpy(&sceneNode.material[0], materials[lastMaterialId].name.c_str(), MAX_NODE_NAME_STRING_LENGTH);

					sceneNode.vertexDataSize = mVertexData.size();
					sceneNode.vertexData = new Vertex[sceneNode.vertexDataSize];
					memcpy((void*) sceneNode.vertexData, (void*) mVertexData.data(), sizeof(Vertex) * sceneNode.vertexDataSize);
					sceneNode.startPosition = startPosition;
					startPosition += (GLuint) sceneNode.vertexDataSize;
					sceneNode.endPosition = sceneNode.startPosition + (GLuint) sceneNode.vertexDataSize;
					sceneNode.primativeMode = GL_TRIANGLES;
					sceneNode.diffuseTextureId = 0;
					sceneNode.modelViewMatrix = matrix;
					addSceneNode(&sceneNode);
					mVertexData.clear();
				}
			}

			Vertex v;
			memcpy((void*)& v.vertex, (void*)& shapes[i].mesh.positions[ shapes[i].mesh.indices[j] * 3 ], sizeof(float) * 3);

			if((shapes[i].mesh.indices[j] * 3) >= shapes[i].mesh.normals.size())
			{
				// TODO: generate normals
				std::cerr << "Unable to put normal in " << fileName << std::endl;
				//return;
			} else {
				memcpy((void*)& v.normal, (void*)& shapes[i].mesh.normals[ (shapes[i].mesh.indices[j] * 3) ], sizeof(float) * 3);
			}

			tinyobj::mesh_t* m = &shapes[i].mesh;

			if((shapes[i].mesh.indices[j] * 2) >= shapes[i].mesh.texcoords.size())
			{
				//std::cerr << "Unable to put texcoord in " << shapes[i].name << std::endl;
				v.textureCoordinate[0] = 0.f;
				v.textureCoordinate[1] = 0.f;
			} else {
				v.textureCoordinate[0] = m->texcoords[(int)m->indices[j]*2];
				v.textureCoordinate[1] = 1 - m->texcoords[(int)m->indices[j]*2+1]; // Account for wavefront to opengl coordinate system conversion
			}

			mVertexData.push_back(v);
			if(j == shapes[i].mesh.indices.size() - 1)
			{
				SceneNode sceneNode;
				strncpy(&sceneNode.name[0], shapes[i].name.c_str(), MAX_NODE_NAME_STRING_LENGTH);
				strncpy(&sceneNode.material[0], materials[lastMaterialId].name.c_str(), MAX_MATERIAL_NAME_STRING_LENGTH);

				sceneNode.vertexDataSize = mVertexData.size();
				sceneNode.vertexData = new Vertex[sceneNode.vertexDataSize];
				memcpy((void*) sceneNode.vertexData, (void*) mVertexData.data(), sizeof(Vertex) * sceneNode.vertexDataSize);
				sceneNode.startPosition = startPosition;
				sceneNode.endPosition = sceneNode.startPosition + (GLuint) sceneNode.vertexDataSize;
				startPosition += (GLuint) sceneNode.vertexDataSize;
				sceneNode.primativeMode = GL_TRIANGLES;
				sceneNode.diffuseTextureId = 0;
				sceneNode.modelViewMatrix = matrix;
				addSceneNode(&sceneNode);
			}
		}
	}
}

/*	Binary cache file format:
 * 	[------numMaterials------]
 * 	[------numSceneNodes-----]
 * 	[------numTextures-------]
 * 	[------------------------]
 * 	[------material array----]
 * 	[----scene node array----]
 * 	[----vertex data array---]
 * 	[---texture data array---]
 */
typedef struct BinCacheFileHeader {
	size_t numMaterials;
	size_t numSceneNodes;
	size_t numVertices;
	size_t numTextures;
} BinCacheFileHeader;


// Thread callback to store geometry in a binary file
static int CreateBinCache(void *rendererPtr)
{
	Renderer* renderer = (Renderer*)rendererPtr;
	const char* filename = renderer->cacheFileName.c_str();
	std::ofstream binFile (filename, std::ios::binary | std::ios::trunc);
	if(!binFile.is_open()) {
		std::cerr << "Unable to open " << filename << " for writing" << std::endl;
		return -1;
	}

	BinCacheFileHeader header;
	header.numMaterials = renderer->materials.size();
	header.numSceneNodes = renderer->sceneNodes.size();
	header.numVertices = renderer->vertexData.size();
	header.numTextures = renderer->textures.size();
	binFile.write((char*)&header, sizeof(BinCacheFileHeader));

	// Write material array
	std::map<std::string, Material>::iterator it;
	for(it=renderer->materials.begin(); it!=renderer->materials.end(); ++it) {
		Material* m = &it->second;
		binFile.write((char*)m, sizeof(Material));
	}

	// Write scene node array
	std::vector<SceneNode>::iterator it2;
	for(it2=renderer->sceneNodes.begin(); it2!=renderer->sceneNodes.end(); ++it2) {
		SceneNode* sn = &*it2;
		binFile.write((char*)sn, sizeof(SceneNode));
	}

	// Write vertex array
	Vertex* v = 0;
	for(size_t i=0; i<renderer->vertexData.size(); i++) {
		v = &renderer->vertexData[i];
		binFile.write((char*)v, sizeof(Vertex));
	}

	// Write textures to array at end of file
	std::map<std::string, Texture>::iterator it3;
	char name[MAX_MATERIAL_NAME_STRING_LENGTH];
	for(it3=renderer->textures.begin(); it3!=renderer->textures.end(); ++it3) {
		Texture* texture = &it3->second;
		strncpy(&name[0], it3->first.c_str(), MAX_MATERIAL_NAME_STRING_LENGTH);
		binFile.write(&name[0], sizeof(char) * MAX_MATERIAL_NAME_STRING_LENGTH);
		binFile.write((char*) &texture->bpp, sizeof(unsigned));
		binFile.write((char*) &texture->mode, sizeof(int));
		binFile.write((char*) &texture->width, sizeof(unsigned));
		binFile.write((char*) &texture->height, sizeof(unsigned));
		size_t dataSize = (size_t) texture->width * texture->height * texture->bpp;
		binFile.write((char*) &texture->data, dataSize);
	}

	binFile.close();

	if(renderer->configLoader->getBool("renderer.verbose"))
		std::cout << "saved cache to " << filename << std::endl;

	return 0;
}

bool Renderer::checkScene() {

	if(sceneNodes.size() == 0)
	{
		std::cout << "building empty scene" << std::endl;
		return false;
	}

	if(configLoader->getBool("renderer.verbose")) {
		std::cout << "num scene nodes: " << sceneNodes.size() << std::endl;
		std::cout << "num vertices: " << vertexData.size() << std::endl;
		std::cout << "num indices: " << indices.size() << std::endl;
	}
	return true;
}

bool Renderer::buildScene(Camera& camera)
{
	// populate vertexData and indices from sceneNodes
	for(int i=0; i<sceneNodes.size(); i++)
	{
		for(int j=0; j<sceneNodes[i].vertexDataSize; j++)
		{
			Vertex v;
			memcpy((void*) &v, (void*) &sceneNodes[i].vertexData[j], sizeof(Vertex));
			vertexData.push_back(v);
			indices.push_back((unsigned int) indices.size());
		}
	}

	//Calculate Bounding Sphere radius
	for(int i=0; i<sceneNodes.size(); i++)
	{
		// local origin/center of object (center of bounding sphere)
		float lx = 0.f, ly = 0.f, lz = 0.f;
		float r = 0.f;

		int vertexDataSize = (int) sceneNodes[i].vertexDataSize;
		//Calculate local origin
		for(int j=0; j<vertexDataSize; j++)
		{
			lx += sceneNodes[i].vertexData[j].vertex[0];
			ly += sceneNodes[i].vertexData[j].vertex[1];
			lz += sceneNodes[i].vertexData[j].vertex[2];
		}
		lx /= (double)vertexDataSize;
		ly /= (double)vertexDataSize;
		lz /= (double)vertexDataSize;
		sceneNodes[i].lx = lx;
		sceneNodes[i].ly = ly;
		sceneNodes[i].lz = lz;

		for(int j=0; j<sceneNodes[i].vertexDataSize; j++)
		{
			float x = sceneNodes[i].vertexData[j].vertex[0];
			float y = sceneNodes[i].vertexData[j].vertex[1];
			float z = sceneNodes[i].vertexData[j].vertex[2];

			double nx = x - lx;
			double ny = y - ly;
			double nz = z - lz;

			float r2 = (float) sqrt(nx*nx + ny*ny + nz*nz);

			if(r2 > r)
			{
				r = r2;
			}
			//std::cerr << "Boundingsphere for " << sceneNodes[i].name << " = " <<  r << std::endl;

		}
		if(r == 0)
		{
			//std::cerr << "Warning, bounding sphere radius = 0 for " << sceneNodes[i].name << std::endl;
			r = 0.1f;
		}
		sceneNodes[i].boundingSphere = r;
	}

	// Free vertex data in sceneNodes
	for(size_t i = 0; i<sceneNodes.size(); i++) {
		delete[] sceneNodes[i].vertexData;
	}

	return checkScene();
}

// Load a scene from a binary file cache
bool Renderer::buildScene(Camera& camera, const char* filename)
{

	if(configLoader->getBool("renderer.verbose"))
		std::cout << "loading cached geometry" << std::endl;

	std::ifstream binFile(filename, std::ios::in | std::ios::binary);
	if(!binFile.is_open()) {
		if(configLoader->getBool("renderer.verbose"))
			std::cout << "Unable to open " << filename << " for reading" << std::endl;

		return false;
	}

	Material m;
	SceneNode sn;
	Vertex v;
	BinCacheFileHeader header;
	// Load header
	binFile.read((char*)&header, sizeof(BinCacheFileHeader));

	// Load materials
	for(size_t i=0; i<header.numMaterials; i++) {
		binFile.read((char*)&m, sizeof(Material));
		materials[m.name] = m;
	}

	// Load scene nodes
	for(size_t i=0; i<header.numSceneNodes; i++) {
		binFile.read((char*)&sn, sizeof(SceneNode));
		sn.diffuseTextureId = 0;
		addSceneNode(&sn);
	}

	// Load vertex data
	for(size_t i=0; i<header.numVertices; i++) {
		binFile.read((char*)&v, sizeof(Vertex));
		vertexData.push_back(v);
		indices.push_back((unsigned int) indices.size());
	}

	size_t numTexturesLoaded = 0;
	// Load textures from the end of the file
	char textureFileName[MAX_MATERIAL_NAME_STRING_LENGTH];
	while(numTexturesLoaded < header.numTextures) {
		textureFileName[0] = '\0';
		Texture texture;
		binFile.read((char*) &textureFileName[0], MAX_MATERIAL_NAME_STRING_LENGTH);
		binFile.read((char*) &texture.bpp, sizeof(unsigned));
		binFile.read((char*) &texture.mode, sizeof(int));
		binFile.read((char*) &texture.width, sizeof(unsigned));
		binFile.read((char*) &texture.height, sizeof(unsigned));
		size_t imageSize = texture.width * texture.height * texture.bpp;
		if(imageSize == 0) {
			std::cerr << "Unable to load image size of 0: " << textureFileName << std::endl;
			exit(9);
		}
		texture.data = new unsigned char[imageSize];
		binFile.read((char*)texture.data, imageSize);
		textures[std::string(&textureFileName[0])] = texture;
		numTexturesLoaded++;
	}

	binFile.close();
	if(configLoader->getBool("renderer.verbose")) std::cout << "loaded cached scene" << std::endl;

	return checkScene();
}

void Renderer::bufferToGpu(Camera& camera, bool loadCachedScene)
{
	if(configLoader->getBool("renderer.verbose")) std::cout << "Buffering to GPU" << std::endl;
	// Load textures
	checkForGLError();
	for(int i=0; i <sceneNodes.size(); i++)
	{
		if(materials.find(sceneNodes[i].material) == materials.end()  )
		{
			std::cerr << "Material " << sceneNodes[i].material << " was not loaded" << std::endl;
		}
		else
		{
			if(strlen(materials[sceneNodes[i].material].diffuseTexName) > 0)
				addTexture(materials[sceneNodes[i].material].diffuseTexName, &sceneNodes[i].diffuseTextureId);
		}
	}

	if(configLoader->getBool("renderer.verbose")) std::cout << "buffered textures" << std::endl;

	checkForGLError();

	//Allocate and assign a Vertex Array Object to our handle
	glGenVertexArrays(1, &vao);
	checkForGLError();
	// Bind our Vertex Array Object as the current used object
	glBindVertexArray(vao);
	checkForGLError();

	//Triangle Vertices
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vertexData.size(), &vertexData[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)0);                       //send positions on pipe 0
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)(sizeof(float)*3));       //send normals on pipe 1
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)(sizeof(float)*6));     //send texcoords on pipe 2

	// Spawn thread to save scene to binary cache
	if(!loadCachedScene && configLoader->getBool("renderer.createBinObj")) {
		binCacheWriterThread = SDL_CreateThread(CreateBinCache, "BinCacheWriterThread", (void *)this);
	}

	checkForGLError();

	glGenBuffers(1, &ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * indices.size(), &indices[0], GL_STATIC_DRAW);
	checkForGLError();

	if(configLoader->getBool("renderer.verbose")) std::cout << "buffered geometry" << std::endl;

	glEnable(GL_DEPTH_TEST);

	checkForGLError();
	gpuProgram = new GpuProgram();
	shadowProgram = new GpuProgram();
	checkForGLError();

	std::string shadowVertShaderPath(SHADER_DIRECTORY);
	std::string shadowFragShaderPath(SHADER_DIRECTORY);
	shadowVertShaderPath += DIRECTORY_SEPARATOR + configLoader->getVar("shader.depth.vert");
	shadowFragShaderPath += DIRECTORY_SEPARATOR + configLoader->getVar("shader.depth.frag");
	VertexShader shadowVertShader(shadowVertShaderPath);
	FragmentShader shadowFragShader(shadowFragShaderPath);

	shadowProgram->attachShader(shadowVertShader);
	shadowProgram->attachShader(shadowFragShader);
	checkForGLError();
	glLinkProgram(shadowProgram->getId());

	std::string vertShaderPath(SHADER_DIRECTORY);
	std::string fragShaderPath(SHADER_DIRECTORY);
	vertShaderPath += DIRECTORY_SEPARATOR + configLoader->getVar("shader.vert");
	fragShaderPath += DIRECTORY_SEPARATOR + configLoader->getVar("shader.frag");
	VertexShader vertShader(vertShaderPath);
	FragmentShader fragShader(fragShaderPath);

	gpuProgram->attachShader(vertShader);
	gpuProgram->attachShader(fragShader);

	glLinkProgram(gpuProgram->getId());

	// Check for link errors
	checkForGLError();
	checkForGLSLError(gpuProgram->getId());
	checkForGLSLError(shadowProgram->getId());
	checkForGLError();

	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	checkForGLError();

	glGenFramebuffers(1, &depthMapFBO);

	// Set uniforms
	GLuint programId = gpuProgram->getId();
	GLuint depthProgramId = shadowProgram->getId();
	glm::mat4 lightProjection, lightView;
	glm::mat4 lightSpaceMatrix;
	glm::mat4 model;

	// light positioned above and in front of camera
	glm::vec3 lightPos = camera.position + glm::vec3(0.0, 100.0, 0.0);
	lightProjection = glm::ortho(
			configLoader->getFloat("shadow.ortho.left"),
			configLoader->getFloat("shadow.ortho.right"),
			configLoader->getFloat("shadow.ortho.bottom"),
			configLoader->getFloat("shadow.ortho.top"),
			configLoader->getFloat("shadow.ortho.near"),
			configLoader->getFloat("shadow.ortho.far")
	);
	//lightProjection = glm::perspective(60.0f,
	//		(GLfloat) shadowWidth / (GLfloat) shadowHeight, near_plane,
	//		far_plane); // Note that if you use a perspective projection matrix you'll have to change the light position as the current light position isn't enough to reflect the whole scene.
	lightView = glm::lookAt(lightPos, glm::vec3(0.0f),
			glm::vec3(0.0, 1.0, 0.0));
	lightSpaceMatrix = lightProjection * lightView;

	// Set shadow program uniforms: uniform mat4 lightSpaceMatrix; uniform mat4 model;
	shadowProgram->uniformLoader->addUniform("lightSpaceMatrix",
			new UniformMat4(lightSpaceMatrix));

	shadowProgram->uniformLoader->addUniform("model", new UniformMat4(model));

	// Set rendering shader uniforms: uniform mat4 projection; uniform mat4 view; uniform mat4 model; uniform mat4 lightSpaceMatrix;

	gpuProgram->uniformLoader->addUniform("projection",	new UniformMat4(camera.projectionMatrix));
	gpuProgram->uniformLoader->addUniform("view", new UniformMat4(camera.modelViewMatrix));
	gpuProgram->uniformLoader->addUniform("model", new UniformMat4(model));

	gpuProgram->uniformLoader->addUniform("lightSpaceMatrix",
			new UniformMat4(lightSpaceMatrix));

	gpuProgram->uniformLoader->addUniform("lightPos",
			new UniformVec3(lightPos));

	gpuProgram->uniformLoader->addUniform("viewPos",
			new UniformVec3(camera.position));

	//uniform sampler2D diffuseTexture; 	uniform sampler2D shadowMap;

	gpuProgram->uniformLoader->addUniform("diffuseTexture", new UniformInt(0));
	gpuProgram->uniformLoader->addUniform("shadowMap", new UniformInt(1));
	gpuProgram->uniformLoader->addUniform("shadows", new UniformInt(shadowsEnabled ? 1 : 0));
}

void Renderer::enableShadows()
{
	UniformInt* u = (UniformInt*) gpuProgram->uniformLoader->get("shadows");
	u->set(1);
	shadowsEnabled = 1;
}

void Renderer::disableShadows()
{
	UniformInt* u = (UniformInt*) gpuProgram->uniformLoader->get("shadows");
	u->set(0);
	shadowsEnabled = 0;
}

GLuint Renderer::createShadowMap(Camera& camera)
{
	// See https://github.com/JoeyDeVries/LearnOpenGL/blob/master/src/5.advanced_lighting/3.1.shadow_mapping/shadow_mapping.cpp:120
	// - Create depth texture
	GLuint depthMap;
	glGenTextures(1, &depthMap);
	glBindTexture(GL_TEXTURE_2D, depthMap);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, shadowWidth, shadowHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	GLfloat borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Backup the viewport
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	glViewport(0, 0, shadowWidth, shadowHeight);
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glClear(GL_DEPTH_BUFFER_BIT);

	if(sceneNodes.size() == 0)
	{
		std::cout << "empty scene" << std::endl;
		exit(-1);
	}

#if _DEBUG
	checkForGLError();
#endif

	glBindVertexArray(vao);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);

	shadowProgram->use();
	shadowProgram->uniformLoader->load();


	for(int i=0; i<sceneNodes.size(); i++)
	{
		glDrawRangeElementsBaseVertex(sceneNodes[i].primativeMode, sceneNodes[i].startPosition, sceneNodes[i].endPosition,
				(sceneNodes[i].endPosition - sceneNodes[i].startPosition), GL_UNSIGNED_INT, (void*)(0), sceneNodes[i].startPosition);

	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
	glBindVertexArray(0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Restore viewport
	glViewport(0, 0, viewport[2], viewport[3]);
	glBindTexture(GL_TEXTURE_2D, depthMap);

#if _DEBUG
	checkForGLError();
#endif

	return depthMap;
}

void Renderer::render(Camera* camera)
{
	if(sceneNodes.size() == 0)
	{
		std::cout << "skipping render() on empty scene" << std::endl;
		exit(-1);
		return;
	}

	glm::vec3 lightPos = camera->position + glm::vec3(10.0, 50.0, 0.0);

	GLuint depthProgramId = shadowProgram->getId();
	glm::mat4 lightProjection, lightView;
	glm::mat4 lightSpaceMatrix;
	glm::mat4 model;
	GLfloat near_plane = 0.6f, far_plane = 120.f;
	lightProjection = glm::ortho(-100.0f, 100.0f, -100.0f, 100.0f, near_plane, far_plane);
	//lightProjection = glm::perspective(60.0f, (GLfloat)shadowWidth / (GLfloat)shadowHeight, near_plane, far_plane); // Note that if you use a perspective projection matrix you'll have to change the light position as the current light position isn't enough to reflect the whole scene.
	lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
	lightSpaceMatrix = lightProjection * lightView;


	// Set shadow program uniforms: uniform mat4 lightSpaceMatrix; uniform mat4 model;
	shadowProgram->uniformLoader->addUniform(
			"lightSpaceMatrix",
			new UniformMat4(lightSpaceMatrix)
	);

	gpuProgram->uniformLoader->addUniform(
			"lightSpaceMatrix",
			new UniformMat4(lightSpaceMatrix)
	);

	UniformVec3* lightPosUniform = (UniformVec3*) gpuProgram->uniformLoader->get("lightPos");
	lightPosUniform->set(lightPos);

	if(shadowsEnabled) shadowMap = createShadowMap(*camera);
	glUseProgram(0);


	glBindVertexArray(vao);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);

#if _DEBUG
	checkForGLError();
#endif

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);

	frustum.extractFrustum(camera->modelViewMatrix, camera->projectionMatrix);
	for(int i=0; i<sceneNodes.size(); i++)
	{
		glm::vec4 position(sceneNodes[i].lx, sceneNodes[i].ly, sceneNodes[i].lz, 1.f);

		// Frustum culling test
		if( frustum.spherePartiallyInFrustum(position.x, position.y, position.z, sceneNodes[i].boundingSphere) > 0)
		{

			gpuProgram->use();
#if _DEBUG
			checkForGLError();
#endif


			UniformMat4* viewUniform = (UniformMat4*) gpuProgram->uniformLoader->get("view");
			viewUniform->set(camera->modelViewMatrix);

			glActiveTexture(GL_TEXTURE0);
#if _DEBUG
			checkForGLError();
#endif


			glBindTexture(GL_TEXTURE_2D,  sceneNodes[i].diffuseTextureId );


#if _DEBUG
			checkForGLError();
#endif
			if(shadowsEnabled) {
				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D,  shadowMap );
			}
			gpuProgram->uniformLoader->load();

#if _DEBUG
			checkForGLError();
#endif

			glDrawRangeElementsBaseVertex(sceneNodes[i].primativeMode, sceneNodes[i].startPosition, sceneNodes[i].endPosition,
					(sceneNodes[i].endPosition - sceneNodes[i].startPosition), GL_UNSIGNED_INT, (void*)(0), sceneNodes[i].startPosition);

#if _DEBUG
			checkForGLError();
#endif
		}
	}

	if(shadowsEnabled == 1) glDeleteTextures(1, &shadowMap);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
	glBindVertexArray(0);

#if _DEBUG
	checkForGLError();
#endif
}
