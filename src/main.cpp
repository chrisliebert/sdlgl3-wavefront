#include "Common.h"
#include <stdio.h>
#include "SceneNode.h"
#include "ConfigLoader.h"
#include "Renderer.h"
#include "OpenGLBackend.h"
#include <array>
#include <filesystem>
#include <memory>
#include <string_view>
#include <atomic>
#include <limits>

#if __has_include("stb_image_write.h")
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#endif

#ifndef SDL_WINDOW_SHOWN
#define SDL_WINDOW_SHOWN 0
#endif

static std::string sanitizeCacheKey(std::string_view modelPath)
{
    std::filesystem::path path(modelPath);
    std::string key = path.filename().string();
    if (key.empty()) key = "scene";

    for (char& ch : key)
    {
        if (ch == '\\' || ch == '/' || ch == ':' || ch == ' ')
            ch = '_';
    }
    return key;
}

static std::string resolveModelPath(std::string_view modelInput)
{
    std::filesystem::path inputPath(modelInput);
    if (inputPath.is_absolute())
        return {};

    inputPath = inputPath.lexically_normal();

    std::filesystem::path relativeInModelDir = inputPath;
    auto it = inputPath.begin();
    if (it != inputPath.end() && *it == std::filesystem::path(MODEL_DIRECTORY))
    {
        ++it;
        relativeInModelDir.clear();
        for (; it != inputPath.end(); ++it)
        {
            relativeInModelDir /= *it;
        }
    }

    if (relativeInModelDir.empty())
        return {};

    const std::filesystem::path sourceModelPath = std::filesystem::path(MODEL_DIRECTORY) / relativeInModelDir;
    if (std::filesystem::exists(sourceModelPath))
        return relativeInModelDir.generic_string();

    const std::filesystem::path buildModelPath = std::filesystem::path("build") / std::filesystem::path(MODEL_DIRECTORY) / relativeInModelDir;
    if (std::filesystem::exists(buildModelPath))
        return relativeInModelDir.generic_string();

    return {};
}

class MyGLApp
{
public:
    explicit MyGLApp(std::string_view filename);
    ~MyGLApp();
    
    // Non-copyable
    MyGLApp(const MyGLApp&) = delete;
    MyGLApp& operator=(const MyGLApp&) = delete;

    // Getters for private members accessed by LoadScene
    [[nodiscard]] Camera& getCamera() { return *camera; }
    [[nodiscard]] std::atomic<bool>& getSceneLoaded() { return sceneLoaded; }
    [[nodiscard]] Renderer* getRenderer() { return renderer.get(); }
    [[nodiscard]] const std::string& getModelFilename() const { return modelFilename; }
    [[nodiscard]] bool& getUseBinCache() { return useBinCache; }

public:
    void start();

private:
    SDL_Window* window = nullptr;
    std::unique_ptr<Renderer> renderer;
    std::unique_ptr<Camera> camera;
    std::unique_ptr<ConfigLoader> configLoader;
    SDL_GLContext glContext = nullptr;
    SDL_Event event{};
    void* sceneLoaderThread = nullptr;

    double speed = 1.0;
    double mouseSpeed = 0.002;
    double deltaTime = 0.0;
    int runLevel = 0;
    Uint64 lastTimeNs = 0;
    bool windowGrab = false;
    bool showCursor = true;
    std::atomic<bool> sceneLoaded{false};
    bool useBinCache = false;
    bool mcp_capture_frame = false;
    bool moveForwardPressed = false;
    bool moveBackwardPressed = false;
    bool moveRightPressed = false;
    bool moveLeftPressed = false;
    bool sprintPressed = false;
    bool shadowEnablePressed = false;
    bool shadowDisablePressed = false;
    std::string modelFilename;
    bool sdlInitialized = false;

    void update(const FrameContext& frameContext);
    void keyDown(SDL_Keycode key);
    void keyUp(SDL_Keycode key);
    bool startup(std::string_view filename);
    void shutdown();
    void errorMsg(std::string_view title);
    void infoMsg(std::string_view msg);
};

void MyGLApp::infoMsg(std::string_view msg)
{
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Info", msg.data(), window);
}

void MyGLApp::errorMsg(std::string_view title)
{
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title.data(), SDL_GetError(), window);
}

MyGLApp::MyGLApp(std::string_view filename)
{
    renderer = std::make_unique<Renderer>();
    startup(filename);
    sceneLoaded.store(false, std::memory_order_release);
}

MyGLApp::~MyGLApp()
{
    shutdown();
#if defined(_DEBUG) || defined(DEBUG)
    std::cout << "closed application" << std::endl;
#endif
}

void MyGLApp::shutdown()
{
    if (sceneLoaderThread != nullptr)
    {
        int ret = 0;
        SDL_WaitThread(static_cast<SDL_Thread*>(sceneLoaderThread), &ret);
        sceneLoaderThread = nullptr;
    }

    renderer.reset();

    if (glContext != nullptr)
    {
        SDL_GL_DeleteContext(glContext);
        glContext = nullptr;
    }

    if (window != nullptr)
    {
        SDL_DestroyWindow(window);
        window = nullptr;
    }

    if (sdlInitialized)
    {
        SDL_Quit();
        sdlInitialized = false;
    }
}

static int LoadScene(void* appPtr)
{
    try
    {
        auto* app = static_cast<MyGLApp*>(appPtr);
        const std::filesystem::path cachePath = std::filesystem::path(CACHE_DIRECTORY) /
            (sanitizeCacheKey(app->getModelFilename()) + ".bin");
        std::string cacheFileName = cachePath.string();
        app->getRenderer()->cacheFileName = cacheFileName;

        bool sceneLoaded = false;
        if (app->getUseBinCache())
        {
            sceneLoaded = app->getRenderer()->buildScene(app->getCamera(), cacheFileName);
            if (!sceneLoaded)
            {
                std::error_code removeError;
                if (std::filesystem::exists(cacheFileName, removeError))
                {
                    std::filesystem::remove(cacheFileName, removeError);
                    if (!removeError)
                        std::cerr << "Removed invalid cache file: " << cacheFileName << std::endl;
                }
            }
            if (sceneLoaded)
            {
                const auto& nodes = app->getRenderer()->sceneNodes;
                const auto& verts = app->getRenderer()->vertexData;
                const auto& inds = app->getRenderer()->indices;
                bool hasDrawableNode = false;
                for (const auto& node : nodes)
                {
                    if (node.endPosition > node.startPosition)
                    {
                        hasDrawableNode = true;
                        break;
                    }
                }
                if (nodes.empty() || verts.empty() || inds.empty() || !hasDrawableNode)
                {
                    std::cerr << "Cached scene is empty/invalid, rebuilding from OBJ" << std::endl;
                    app->getRenderer()->sceneNodes.clear();
                    app->getRenderer()->vertexData.clear();
                    app->getRenderer()->indices.clear();
                    sceneLoaded = false;
                }
            }
        }

        if (!sceneLoaded)
        {
            std::cout << "Creating Scene" << std::endl;
            app->getRenderer()->addWavefront(app->getModelFilename(), glm::mat4(1.0f));
            app->getUseBinCache() = false;
            sceneLoaded = app->getRenderer()->buildScene(app->getCamera());
        }

        if (!sceneLoaded)
        {
            std::cerr << "Unable to load scene" << std::endl;
            return -7;
        }
        app->getSceneLoaded().store(true, std::memory_order_release);
        return 0;
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Scene loader exception: " << ex.what() << std::endl;
        return -8;
    }
    catch (...)
    {
        std::cerr << "Scene loader exception: unknown error" << std::endl;
        return -9;
    }
}

bool MyGLApp::startup(std::string_view filename)
{
    configLoader = std::make_unique<ConfigLoader>("app.cfg");
    speed = configLoader->getFloat("camera.speed");
    mouseSpeed = configLoader->getFloat("mouse.speed");
    windowGrab = configLoader->getBool("window.grab");
    showCursor = configLoader->getBool("mouse.show");
    runLevel = 1;
    lastTimeNs = SDL_GetTicksNS();
    modelFilename = std::string(filename);
    sceneLoaderThread = nullptr;
    sceneLoaded.store(false, std::memory_order_release);
    useBinCache = configLoader->getBool("useBinObjCache");

    Uint32 sdlInitFlags = SDL_INIT_VIDEO | SDL_INIT_EVENTS;

    if (!SDL_Init(sdlInitFlags))
    {
        std::cerr << "Unable to initialize SDL: " << SDL_GetError() << std::endl;
        runLevel = 0;
        return false;
    }
    sdlInitialized = true;

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    if (configLoader->hasVar("window.multiSampleBuffers"))
    {
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, configLoader->getInt("window.numMultiSampleBuffers"));
    }

    SDL_WindowFlags flags = static_cast<SDL_WindowFlags>(SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_OPENGL);
    flags = static_cast<SDL_WindowFlags>(flags | SDL_WINDOW_SHOWN);

    const int windowWidth = configLoader->hasVar("window.width") ? configLoader->getInt("window.width") : 1280;
    const int windowHeight = configLoader->hasVar("window.height") ? configLoader->getInt("window.height") : 720;
    const bool hasWindowPosition = configLoader->hasVar("window.position.x") && configLoader->hasVar("window.position.y");

    if (configLoader->getBool("window.fullscreen"))
        flags |= SDL_WINDOW_FULLSCREEN;

    window = SDL_CreateWindow("Loading",
        windowWidth,
        windowHeight,
        flags);
    if (window != nullptr && hasWindowPosition)
    {
        SDL_SetWindowPosition(window,
            configLoader->getInt("window.position.x"),
            configLoader->getInt("window.position.y"));
    }

    if (window == nullptr)
    {
        fprintf(stderr, "Unable to create window: %s\n", SDL_GetError());
        errorMsg("Unable to create window");
        runLevel = 0;
        return false;
    }

    if (configLoader->getBool("window.hide"))
        SDL_HideWindow(window);

    struct GlContextVersion { int major; int minor; };
    constexpr std::array<GlContextVersion, 4> candidates = {{
        {4, 6}, {4, 5}, {4, 3}, {3, 3}
    }};

    glContext = nullptr;
    for (const auto& candidate : candidates)
    {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, candidate.major);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, candidate.minor);
        glContext = SDL_GL_CreateContext(window);
        if (glContext != nullptr)
        {
            std::cout << "Requested OpenGL " << candidate.major << "." << candidate.minor << " core context" << std::endl;
            break;
        }
    }

    if (glContext == nullptr)
    {
        errorMsg("Unable to create OpenGL context");
        errorMsg(SDL_GetError());
        runLevel = 0;
        return false;
    }

    SDL_GL_SetSwapInterval(0);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(SDL_GL_GetProcAddress)))
    {
        std::cerr << "Something went wrong initializing GLAD!" << std::endl;
        runLevel = 0;
        return false;
    }

#ifdef GLAD_DEBUG
    glad_set_pre_callback(pre_gl_call);
    glad_debug_glClear = glad_glClear;
#endif

    std::cout << "OpenGL " << GLVersion.major << "." << GLVersion.minor << std::endl;
    if (GLVersion.major < 3 || (GLVersion.major == 3 && GLVersion.minor < 3))
    {
        std::cerr << "Your system doesn't support OpenGL >= 3.3 core features!" << std::endl;
        runLevel = 0;
        return false;
    }

    std::cout << "OpenGL " << glGetString(GL_VERSION) << " GLSL " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;

    checkForGLError();

    if (configLoader->getBool("window.grab"))
    {
        SDL_SetWindowMouseGrab(window, SDL_TRUE);
        SDL_SetWindowRelativeMouseMode(window, SDL_TRUE);
    }

    if (!showCursor)
    {
        SDL_HideCursor();
    }

    checkForGLError();
    glEnable(GL_DEPTH_TEST);
    checkForGLError();
    glDepthFunc(GL_LESS);

    if (configLoader->getBool("cullFace"))
        glEnable(GL_CULL_FACE);
    else
        glDisable(GL_CULL_FACE);
    checkForGLError();

    camera = std::make_unique<Camera>();
    if (configLoader->hasVar("camera.position.x")) camera->position.x = configLoader->getFloat("camera.position.x");
    if (configLoader->hasVar("camera.position.y")) camera->position.y = configLoader->getFloat("camera.position.y");
    if (configLoader->hasVar("camera.position.z")) camera->position.z = configLoader->getFloat("camera.position.z");

    auto backend = std::make_unique<OpenGLBackend>(*renderer);
    backend->initialize(window);
    renderer->setBackend(std::move(backend));

    sceneLoaderThread = SDL_CreateThread(LoadScene, "MainLoadSceneThread", this);

    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    glViewport(0, 0, viewport[2], viewport[3]);

    return runLevel > 0;
}

void MyGLApp::keyDown(SDL_Keycode key)
{
    switch (key)
    {
        case SDLK_ESCAPE:
            runLevel = 0;
            break;
        case SDLK_g:
            windowGrab = !windowGrab;
            if (window)
            {
                SDL_SetWindowMouseGrab(window, windowGrab ? SDL_TRUE : SDL_FALSE);
                SDL_SetWindowRelativeMouseMode(window, windowGrab ? SDL_TRUE : SDL_FALSE);
            }
            break;
        case SDLK_h:
            showCursor = !showCursor;
            if (showCursor) SDL_ShowCursor(); else SDL_HideCursor();
            break;
        case SDLK_w:
        case SDLK_UP:
            moveForwardPressed = true;
            break;
        case SDLK_s:
        case SDLK_DOWN:
            moveBackwardPressed = true;
            break;
        case SDLK_d:
        case SDLK_RIGHT:
            moveRightPressed = true;
            break;
        case SDLK_a:
        case SDLK_LEFT:
            moveLeftPressed = true;
            break;
        case SDLK_LSHIFT:
            sprintPressed = true;
            break;
        case SDLK_1:
            shadowEnablePressed = true;
            break;
        case SDLK_2:
            shadowDisablePressed = true;
            break;
        case SDLK_p:
            if (camera) renderer->bufferToGpu(*camera, useBinCache);
            break;
        case SDLK_m:
            mcp_capture_frame = true;
            break;
    }
}

void MyGLApp::keyUp(SDL_Keycode key)
{
    switch (key)
    {
        case SDLK_w:
        case SDLK_UP:
            moveForwardPressed = false;
            break;
        case SDLK_s:
        case SDLK_DOWN:
            moveBackwardPressed = false;
            break;
        case SDLK_d:
        case SDLK_RIGHT:
            moveRightPressed = false;
            break;
        case SDLK_a:
        case SDLK_LEFT:
            moveLeftPressed = false;
            break;
        case SDLK_LSHIFT:
            sprintPressed = false;
            break;
        case SDLK_1:
            shadowEnablePressed = false;
            break;
        case SDLK_2:
            shadowDisablePressed = false;
            break;
    }
}

void MyGLApp::update(const FrameContext& frameContext)
{
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT)
        {
            runLevel = 0;
            return;
        }
        if (event.type == SDL_EVENT_KEY_DOWN)
        {
            keyDown(event.key.key);
            switch (event.key.scancode)
            {
                case SDL_SCANCODE_W:
                case SDL_SCANCODE_UP:
                    moveForwardPressed = true;
                    break;
                case SDL_SCANCODE_S:
                case SDL_SCANCODE_DOWN:
                    moveBackwardPressed = true;
                    break;
                case SDL_SCANCODE_D:
                case SDL_SCANCODE_RIGHT:
                    moveRightPressed = true;
                    break;
                case SDL_SCANCODE_A:
                case SDL_SCANCODE_LEFT:
                    moveLeftPressed = true;
                    break;
                case SDL_SCANCODE_LSHIFT:
                    sprintPressed = true;
                    break;
                default:
                    break;
            }
        }
        else if (event.type == SDL_EVENT_KEY_UP)
        {
            keyUp(event.key.key);
            switch (event.key.scancode)
            {
                case SDL_SCANCODE_W:
                case SDL_SCANCODE_UP:
                    moveForwardPressed = false;
                    break;
                case SDL_SCANCODE_S:
                case SDL_SCANCODE_DOWN:
                    moveBackwardPressed = false;
                    break;
                case SDL_SCANCODE_D:
                case SDL_SCANCODE_RIGHT:
                    moveRightPressed = false;
                    break;
                case SDL_SCANCODE_A:
                case SDL_SCANCODE_LEFT:
                    moveLeftPressed = false;
                    break;
                case SDL_SCANCODE_LSHIFT:
                    sprintPressed = false;
                    break;
                default:
                    break;
            }
        }
        else if (event.type == SDL_EVENT_MOUSE_MOTION && windowGrab)
            camera->aim(mouseSpeed * static_cast<double>(event.motion.xrel), -mouseSpeed * static_cast<double>(event.motion.yrel));
        else if (event.type == SDL_EVENT_WINDOW_FOCUS_GAINED && windowGrab && window)
        {
            SDL_SetWindowMouseGrab(window, SDL_TRUE);
            SDL_SetWindowRelativeMouseMode(window, SDL_TRUE);
        }
        else if (event.type == SDL_EVENT_WINDOW_FOCUS_LOST)
        {
            moveForwardPressed = false;
            moveBackwardPressed = false;
            moveRightPressed = false;
            moveLeftPressed = false;
            sprintPressed = false;
        }
    }

    if (runLevel < 1 || !camera) return;

    int keyCount = 0;
    const bool* keys = SDL_GetKeyboardState(&keyCount);
    const bool pollForward = keys && ((keyCount > SDL_SCANCODE_W && keys[SDL_SCANCODE_W]) || (keyCount > SDL_SCANCODE_UP && keys[SDL_SCANCODE_UP]));
    const bool pollBackward = keys && ((keyCount > SDL_SCANCODE_S && keys[SDL_SCANCODE_S]) || (keyCount > SDL_SCANCODE_DOWN && keys[SDL_SCANCODE_DOWN]));
    const bool pollRight = keys && ((keyCount > SDL_SCANCODE_D && keys[SDL_SCANCODE_D]) || (keyCount > SDL_SCANCODE_RIGHT && keys[SDL_SCANCODE_RIGHT]));
    const bool pollLeft = keys && ((keyCount > SDL_SCANCODE_A && keys[SDL_SCANCODE_A]) || (keyCount > SDL_SCANCODE_LEFT && keys[SDL_SCANCODE_LEFT]));
    const bool pollSprint = keys && (keyCount > SDL_SCANCODE_LSHIFT && keys[SDL_SCANCODE_LSHIFT]);

    const bool moveForwardActive = moveForwardPressed || pollForward;
    const bool moveBackwardActive = moveBackwardPressed || pollBackward;
    const bool moveRightActive = moveRightPressed || pollRight;
    const bool moveLeftActive = moveLeftPressed || pollLeft;
    const bool sprintActive = sprintPressed || pollSprint;

    if (windowGrab)
    {
        float relX = 0.0f;
        float relY = 0.0f;
        SDL_GetRelativeMouseState(&relX, &relY);
        if (relX != 0.0f || relY != 0.0f)
            camera->aim(mouseSpeed * static_cast<double>(relX), -mouseSpeed * static_cast<double>(relY));
    }

    // Convert to seconds so movement speed is stable across different frame rates.
    const double deltaSeconds = frameContext.deltaTimeMs * 0.001;
    const double speedMultiplier = sprintActive ? 2.0 : 1.0;
    const double movementSpeed = speed * speedMultiplier;
    if (moveForwardActive) camera->moveForward(deltaSeconds * movementSpeed);
    if (moveBackwardActive) camera->moveBackward(deltaSeconds * movementSpeed);
    if (moveRightActive) camera->moveRight(deltaSeconds * movementSpeed);
    if (moveLeftActive) camera->moveLeft(deltaSeconds * movementSpeed);
    if (shadowEnablePressed) renderer->enableShadows();
    if (shadowDisablePressed) renderer->disableShadows();

    camera->update();
}

void MyGLApp::start()
{
    if (runLevel <= 0 || !camera || !configLoader) return;

    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    const float groundLevel = configLoader->getFloat("ground.level");
    bool sceneFinishedLoading = false;
    const bool closeOnLoad = configLoader->getBool("closeOnLoad");
    Uint64 lastHudUpdateCounter = 0;
    constexpr Uint64 hudUpdateIntervalNs = 250000000ULL;
    std::string baseTitle(std::string(configLoader->getVar("window.title")));

    while (runLevel > 0)
    {
        FrameContext frameContext{};
        frameContext.frameStartTicksNs = SDL_GetTicksNS();
        frameContext.absoluteTimeMs = static_cast<double>(frameContext.frameStartTicksNs) / 1000000.0;
        deltaTime = static_cast<double>(frameContext.frameStartTicksNs - lastTimeNs) / 1000000.0;
        lastTimeNs = frameContext.frameStartTicksNs;
        frameContext.deltaTimeMs = deltaTime;
        frameContext.viewportWidth = viewport[2];
        frameContext.viewportHeight = viewport[3];

        update(frameContext);

        if (camera->position.y < groundLevel)
            camera->position.y = groundLevel;

        camera->update();

        if (!sceneFinishedLoading && sceneLoaded.load(std::memory_order_acquire))
        {
            if (!renderer->sceneNodes.empty())
            {
                glm::vec3 minP(std::numeric_limits<float>::max());
                glm::vec3 maxP(std::numeric_limits<float>::lowest());
                for (const auto& node : renderer->sceneNodes)
                {
                    const glm::vec3 c(node.lx, node.ly, node.lz);
                    minP = glm::min(minP, c);
                    maxP = glm::max(maxP, c);
                }

                glm::vec3 center = (minP + maxP) * 0.5f;
                float radius = glm::length(maxP - center);

                for (const auto& node : renderer->sceneNodes)
                {
                    const glm::vec3 c(node.lx, node.ly, node.lz);
                    const float nodeExtent = node.boundingSphere;
                    const float distToCenter = glm::length(c - center);
                    const float outside = distToCenter + nodeExtent;
                    if (outside > radius)
                    {
                        const float newRadius = 0.5f * (radius + outside);
                        const float shift = (outside - newRadius);
                        if (distToCenter > 1e-6f)
                        {
                            center += ((c - center) / distToCenter) * shift;
                        }
                        radius = newRadius;
                    }
                }
                if (radius < 1.0f) radius = 1.0f;

                camera->position = center + glm::vec3(0.0f, radius * 0.35f, radius * 2.5f);
                camera->direction = glm::normalize(center - camera->position);
                const glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
                camera->right = glm::normalize(glm::cross(camera->direction, worldUp));
                camera->up = glm::normalize(glm::cross(camera->right, camera->direction));
                camera->update();

            }

            renderer->bufferToGpu(*camera, useBinCache);
            mcp_capture_frame = true;
            if (window) SDL_SetWindowTitle(window, baseTitle.c_str());
            lastHudUpdateCounter = frameContext.frameStartTicksNs;
            sceneFinishedLoading = true;
            if (closeOnLoad) runLevel = 0;
        }

        if (sceneFinishedLoading && window)
        {
            renderer->render(*camera, frameContext);

            if (mcp_capture_frame) {
                std::cout << "MCP: Capturing window pixels..." << std::endl;
                int captureWidth, captureHeight;
                SDL_GetWindowSize(window, &captureWidth, &captureHeight);

                // Allocate a buffer to hold the pixel data
                std::vector<unsigned char> pixels(captureWidth * captureHeight * 3);

                // Read the rendered frame from the back buffer before swap.
                glReadBuffer(GL_BACK);
                glReadPixels(0, 0, captureWidth, captureHeight, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

                // Write as binary PPM (portable, no external image library required).
                std::ofstream ppmFile("mcp_capture.ppm", std::ios::binary | std::ios::trunc);
                if (ppmFile.is_open())
                {
                    ppmFile << "P6\n" << captureWidth << " " << captureHeight << "\n255\n";
                    for (int row = captureHeight - 1; row >= 0; --row)
                    {
                        const char* rowData = reinterpret_cast<const char*>(pixels.data() + static_cast<size_t>(row) * static_cast<size_t>(captureWidth) * 3u);
                        ppmFile.write(rowData, static_cast<std::streamsize>(captureWidth * 3));
                    }
                    ppmFile.close();
                    std::cout << "MCP: Saved mcp_capture.ppm" << std::endl;
                }
                else
                {
                    std::cerr << "MCP: Failed to write mcp_capture.ppm" << std::endl;
                }
                mcp_capture_frame = false;
            }
            
            if (renderer->isProfilerEnabled())
            {
                Uint64 now = SDL_GetTicksNS();
                if ((now - lastHudUpdateCounter) >= hudUpdateIntervalNs)
                {
                    const Renderer::PerfStats& stats = renderer->getPerfStats();
                    char titleBuffer[256];
                    std::snprintf(titleBuffer, sizeof(titleBuffer),
                        "%s | FPS %.1f | CPU %.2fms | Shadow %.2fms | Draw %d | Visible %d/%d | FrustumCull %d | OccCull %d",
                        baseTitle.c_str(), stats.fps, stats.cpuFrameMs, stats.shadowPassMs,
                        stats.drawCalls, stats.visibleNodes, stats.totalNodes,
                        stats.frustumCulledNodes, stats.occlusionCulledNodes);
                    SDL_SetWindowTitle(window, titleBuffer);
                    lastHudUpdateCounter = now;
                }
            }
        }
    }

    if (window) SDL_HideWindow(window);

    if (!sceneFinishedLoading && sceneLoaderThread != nullptr)
    {
        int ret = 0;
        SDL_WaitThread(static_cast<SDL_Thread*>(sceneLoaderThread), &ret);
        sceneLoaderThread = nullptr;
        if (ret != 0)
            std::cerr << "Scene loader thread exited with status " << ret << std::endl;
    }
}

int main(int argc, char** argv)
{
    std::string modelName;
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <model.obj>" << std::endl;
#if defined(_DEBUG) || defined(DEBUG)
    modelName = "cube.obj";
#else
        return -1;
#endif
    }
    else
    {
        modelName = std::string(argv[1]);
    }

    std::string resolvedModelPath = resolveModelPath(modelName);
    if (resolvedModelPath.empty())
    {
        std::cerr << "Cannot find model file: " << modelName << std::endl;
        std::cerr << "Checked: " << MODEL_DIRECTORY << DIRECTORY_SEPARATOR << modelName
                  << " and build" << DIRECTORY_SEPARATOR << MODEL_DIRECTORY << DIRECTORY_SEPARATOR << modelName << std::endl;
        return -2;
    }

    std::cout << "Loading " << resolvedModelPath << std::endl;
    
#if defined(_MSC_VER)
    std::cout << "MSVC compiler detected" << std::endl;
#endif

    MyGLApp objViewer(resolvedModelPath);
    objViewer.start();
    return 0;
}
