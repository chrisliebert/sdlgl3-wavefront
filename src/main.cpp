#include "Common.h"
#include "SceneNode.h"
#include "Renderer.h"

class MyGLApp
{
public:
	SDL_Window* window;
	Renderer renderer;
	Camera* camera;
	ConfigLoader* configLoader;
	SDL_GLContext glContext;
	SDL_Event event;
	SDL_Thread* sceneLoaderThread;

	double speed;
	double mouseSpeed;
	double deltaTime;
	int runLevel;
	double lastTime;
	bool windowGrab, showCursor;
	bool sceneLoaded, useBinCache;

	std::string modelFilename;
	void start();
	void keyUp(SDL_Keycode& key);
	void keyDown(SDL_Keycode& key);
	~MyGLApp();
	MyGLApp(const char* filename);
	void init(const char* filename);
	void errorMsg(const char* title);
	void infoMsg(const char* msg);
};


void MyGLApp::infoMsg(const char* msg)
{
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Info", msg, NULL);
}

void MyGLApp::errorMsg(const char* title)
{
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title, SDL_GetError(), NULL);
}

MyGLApp::MyGLApp(const char* filename) {
	init(filename);
}

MyGLApp::~MyGLApp()
{
	delete camera;
	delete configLoader;
	SDL_GL_DeleteContext(glContext);
	SDL_DestroyWindow(window);
	SDL_Quit();
#if _DEBUG
	std::cout << "closed application" << std::endl;
#endif
}


static int LoadScene(void* appPtr) {
	MyGLApp* app = (MyGLApp*) appPtr;

	bool sceneLoaded = false;
	std::string cacheFileName(CACHE_DIRECTORY);
	cacheFileName += std::string(DIRECTORY_SEPARATOR) + std::string(app->modelFilename) + ".bin";
	app->renderer.cacheFileName = cacheFileName;
	if(app->useBinCache) {
		// Load scene saved in binary cache
		sceneLoaded = app->renderer.buildScene(*app->camera, cacheFileName.c_str());
	}

	// If cache loading fails or is disabled, then create the scene from scratch
	if(!sceneLoaded) {
		// Create Scene
		std::cout << "Creating Scene" << std::endl;
		app->renderer.addWavefront(app->modelFilename.c_str(), glm::translate(glm::mat4(1.f), glm::vec3(100.0, 0.0, 100.0)));

		app->useBinCache = false;
		// Build scene one objects have been added
		sceneLoaded = app->renderer.buildScene(*app->camera);
	}

	if(!sceneLoaded) {
		std::cerr << "Unable to load scene" << std::endl;
		exit(7);
	}
	app->sceneLoaded = true;
	return 0;
}

void MyGLApp::init(const char* filename) {

	configLoader = new ConfigLoader("app.cfg");
	speed = configLoader->getFloat("camera.speed");
	mouseSpeed = configLoader->getFloat("mouse.speed");
	windowGrab = configLoader->getBool("window.grab");
	showCursor = configLoader->getBool("mouse.show");
	runLevel = 1;
	lastTime = SDL_GetTicks();
	deltaTime = 0.0;
	window = 0;
	camera = 0;
	modelFilename = std::string(filename);
	sceneLoaderThread = 0;
	sceneLoaded = false;
	useBinCache = configLoader->getBool("useBinObjCache");

	if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
	{
		std::cerr << "Unable to initialize SDL: " << SDL_GetError() << std::endl;
		runLevel = 0;
	}
	else
	{
		SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);

		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

		// Enable multisampling if present in app.cfg
		if(configLoader->hasVar("window.multiSampleBuffers")) {
			SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, configLoader->getInt("window.numMultiSampleBuffers"));
		}

		Uint32 flags = SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;

		if(configLoader->getBool("window.fullscreen")) {
			flags |= SDL_WINDOW_FULLSCREEN;
		}
		window = SDL_CreateWindow("Loading",
				configLoader->getInt("window.position.x"),
				configLoader->getInt("window.position.y"),
				configLoader->getInt("window.width"),
				configLoader->getInt("window.height"),
				flags);
		if (window == NULL)
		{
			fprintf(stderr, "Unable to create window: %s\n", SDL_GetError());
			errorMsg("Unable to create window");
			runLevel = 0;
			return;
		}
		else
		{
			if(configLoader->getBool("window.hide")) {
				SDL_HideWindow(window);
			}

			glContext = SDL_GL_CreateContext(window);
			if (glContext == NULL)
			{
				errorMsg("Unable to create OpenGL context");
				errorMsg(SDL_GetError());
				runLevel = 0;
				return;
			}
			/* This makes our buffer swap synchronized with the monitor's vertical refresh */
			SDL_GL_SetSwapInterval(0);

			//GLEW
			glewExperimental = GL_TRUE;
			GLenum err = glewInit();
			checkForGLError();
			if (GLEW_OK != err)
			{
				// Problem: glewInit failed, something is seriously wrong.
				const char* errorStr = (char*)glewGetErrorString(err);
				errorMsg(errorStr);

			}

			//std::cout << "Using GLEW " << glewGetString(GLEW_VERSION) << std::endl;
			//SDL_SetWindowTitle(window, (const char*)glGetString(GL_VERSION));
		}

		if(configLoader->getBool("window.grab"))
			SDL_SetWindowGrab(window, SDL_TRUE);

		if(!showCursor) {
			if (SDL_ShowCursor(SDL_DISABLE) < 0)
			{
				std::cerr << "Unable to hide the cursor" << std::endl;
			}
		}
		/*
			if(SDL_SetRelativeMouseMode(SDL_TRUE) < 0) {
			errorMsg(SDL_GetError());
			} */

		checkForGLError();

		// Get largest anisotropic filtering level
		GLfloat fLargest;
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, fLargest);

		checkForGLError();

		// Enable depth test
		glEnable(GL_DEPTH_TEST);
		checkForGLError();

		// Accept fragment if it closer to the camera than the former one
		glDepthFunc(GL_LESS);

		// Cull triangles which normal is not towards the camera
		// Enable only if faces all faces are drawn counter-clockwise
		if(configLoader->getBool("cullFace")) glEnable(GL_CULL_FACE);
		else glDisable(GL_CULL_FACE);

		checkForGLError();
		glClearColor(1.f, 0.8f, 0.8f, 1.0f);
		checkForGLError();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		checkForGLError();

		camera = new Camera();
		camera->position.x = configLoader->getFloat("camera.position.x");
		camera->position.y = configLoader->getFloat("camera.position.y");
		camera->position.z = configLoader->getFloat("camera.position.z");

		sceneLoaderThread = SDL_CreateThread(LoadScene, "MainLoadSceneThread", this);

		GLint viewport[4];
		glGetIntegerv(GL_VIEWPORT, viewport);

		double width = (double)viewport[2];
		double height = (double)viewport[3];

		glViewport(0, 0, viewport[2], viewport[3]);
	}
}

void MyGLApp::keyDown(SDL_Keycode& key)
{
	switch (key)
	{
	case SDLK_ESCAPE:
		runLevel = 0;
		break;
	case SDLK_g:
		windowGrab = windowGrab ? false : true;
		break;
	case SDLK_h:
		showCursor = showCursor ? false : true;
		if(showCursor) {
			if (SDL_ShowCursor(SDL_ENABLE) < 0)
			{
				std::cerr << "Unable to hide the cursor" << std::endl;
			}
		} else {
			if (SDL_ShowCursor(SDL_DISABLE) < 0)
			{
				std::cerr << "Unable to show the cursor" << std::endl;
			}
		}
		break;
	}
}

void MyGLApp::keyUp(SDL_Keycode& key)
{

}

void MyGLApp::start()
{
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);

	double width = (double)viewport[2];
	double height = (double)viewport[3];

	// Get mouse position
	double xpos, ypos;
	int x, y;

	float groundLevel = configLoader->getFloat("ground.level");

	bool sceneFinishedLoading = false;
	bool closeOnLoad = configLoader->getBool("closeOnLoad");

	/* Get mouse position */
	while (runLevel > 0)
	{
		SDL_PollEvent(&event);

		if (event.type == SDL_QUIT)
		{
			runLevel = 0;
			break;
		}
		else if (event.type == SDL_KEYDOWN)
		{
			keyDown(event.key.keysym.sym);
			if (runLevel < 1)
			{
				break;
			}
		}
		else if (event.type == SDL_KEYUP)
		{
			keyUp(event.key.keysym.sym);
		}

		const Uint8 *keys = SDL_GetKeyboardState(NULL);


		if (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP])
		{
			camera->moveForward(deltaTime * speed);
		}

		if (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN])
		{
			camera->moveBackward(deltaTime * speed);
		}

		if (keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT])
		{
			camera->moveRight(deltaTime * speed);
		}

		if (keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT])
		{
			camera->moveLeft(deltaTime * speed);
		}

		if(keys[SDL_SCANCODE_1])
		{
			renderer.enableShadows();
		}

		if(keys[SDL_SCANCODE_2])
		{
			renderer.disableShadows();
		}

		SDL_GetMouseState(&x, &y);
		xpos = (double)x;
		ypos = (double)y;

		/* Ignore mouse input less than 2 pixels from origin (smoothing) */
		if (abs(x - (int)floor(viewport[2] / 2.0)) < 2)
			x = (int)floor(viewport[2] / 2.0);
		if (abs(y - (int)floor(viewport[3] / 2.0)) < 2)
			y = (int)floor(viewport[3] / 2.0);

		xpos = (double)x;
		ypos = (double)y;

		// Compute time difference between current and last frame
		double currentTime = SDL_GetTicks();
		deltaTime = currentTime - lastTime;
		lastTime = currentTime;

		if(windowGrab) {
			SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);
			SDL_WarpMouseInWindow(window, (int)(width / 2.0), (int)(height / 2.0));
			SDL_EventState(SDL_MOUSEMOTION, SDL_ENABLE);

			camera->aim(
					mouseSpeed * (floor(width / 2.0) - xpos),
					mouseSpeed * (floor(height / 2.0) - ypos)
			);
		}

		// Keep camera fixed above a plane
		if(camera->position.y < groundLevel) {
			camera->position.y = groundLevel;
		}

		camera->update();

		// Render frame
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		if(!sceneFinishedLoading && sceneLoaded) {
			renderer.bufferToGpu(*camera, useBinCache);
			SDL_SetWindowTitle(window, configLoader->getVar("window.title").c_str());
			sceneFinishedLoading = true;
			if(closeOnLoad) {
				runLevel = 0;
			}
		}

		if(sceneFinishedLoading)
			renderer.render(camera);

		SDL_GL_SwapWindow(window);
	}

	// Close window right away
	SDL_HideWindow(window);

	if(!sceneFinishedLoading) {
		// Wait for loader to finish if exited before loading finished
		int ret = 0;
		SDL_WaitThread(sceneLoaderThread, &ret);
		if(ret != 0) {
			std::cerr << "Scene loader thread exited with status " << ret << std::endl;
		}
	}
}

int main(int argc, char** argv)
{
#define MAX_OBJ_FILENAME 500;
	std::string  modelName;
	if (argc != 2) {
		std::cerr << "Usage: " << argv[0] << " <model.obj>" << std::endl;
#if _DEBUG
		modelName = std::string("test.obj");
#else
		return -1;
#endif

	} else {
		modelName = std::string(argv[1]);
	}

	std::cout << "Loading " << modelName << std::endl;
	std::string modelPath(MODEL_DIRECTORY);
	modelPath += std::string(DIRECTORY_SEPARATOR) + modelName;
	std:: ifstream objFile(modelPath.c_str());

#if _MSC_VER
	std::cerr << "MSVC detected" << std::endl;
#else
	if (false == objFile.is_open())
	{
		std::cerr << "Unable to load " << modelPath << std::endl;
		return -2;
	}
#endif

	MyGLApp objViewer(modelName.c_str());
	objViewer.start();
	SDL_Quit();
	return 0;
}
