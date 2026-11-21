# SDL3-Wavefront Architecture Documentation

## Table of Contents
1. [System Overview](#1-system-overview)
2. [Component Architecture](#2-component-architecture)
3. [Class Hierarchy Diagrams](#3-class-hierarchy-diagrams)
4. [Data Flow Diagrams](#4-data-flow-diagrams)
5. [Render Pipeline](#5-render-pipeline)
6. [Memory Management](#6-memory-management)

---

## 1. System Overview

### High-Level Architecture

```mermaid
graph TB
    subgraph "Application Layer"
        A[MyGLApp<br/>main.cpp]
    end
    
    subgraph "Rendering Layer"
        B[Renderer<br/>Core Manager]
        C[IRenderBackend<br/>Abstract Interface]
        D[OpenGLBackend<br/>GL 3.3+ Impl]
    end
    
    subgraph "Scene Management"
        E[SceneGraph<br/>Hierarchy]
        F[SceneNode<br/>Geometry Data]
        G[Material<br/>PBR Properties]
    end
    
    subgraph "Graphics Resources"
        H[GpuProgram<br/>Shader Wrapper]
        I[ShaderCache<br/>Compilation Cache]
        J[Texture<br/>Pixel Data]
    end
    
    subgraph "Utilities"
        K[Camera<br/>View/Proj Matrices]
        L[Frustum<br/>Culling]
        M[ConfigLoader<br/>Settings]
    end
    
    subgraph "External APIs"
        N[OpenGL 3.3+]
        O[SDL3 Window/Input]
        P[tiny_obj_loader<br/>OBJ Parser]
    end
    
    A --> B
    B --> C
    D --> C
    D --> N
    B --> E
    E --> F
    F --> G
    B --> H
    H --> I
    F --> J
    B --> K
    B --> L
    B --> M
    A --> O
    B --> P
```

---

## 2. Component Architecture

### Component Dependencies

```mermaid
graph LR
    subgraph "Entry Point"
        Main[main.cpp]
    end
    
    subgraph "Core Systems"
        App[MyGLApp]
        Renderer[Renderer]
        Backend[IRenderBackend / OpenGLBackend]
    end
    
    subgraph "Scene System"
        SceneGraph[SceneGraph]
        SceneNode[SceneNode]
        Material[Material]
        Texture[Texture]
    end
    
    subgraph "Graphics System"
        GpuProgram[GpuProgram]
        Shader[Shader + Cache]
        Uniform[Uniform Types]
    end
    
    subgraph "Math & Culling"
        Camera[Camera]
        Frustum[Frustum]
    end
    
    subgraph "I/O"
        Config[ConfigLoader]
        ObjLoader[tiny_obj_loader]
    end
    
    Main --> App
    App --> Renderer
    Renderer --> Backend
    Renderer --> SceneGraph
    SceneGraph --> SceneNode
    SceneNode --> Material
    SceneNode --> Texture
    Renderer --> GpuProgram
    GpuProgram --> Shader
    Shader --> Uniform
    Renderer --> Camera
    Renderer --> Frustum
    App --> Config
    Renderer --> ObjLoader
```

---

## 3. Class Hierarchy Diagrams

### Render Backend Hierarchy

```mermaid
classDiagram
    class IRenderBackend {
        <<interface>>
        +initialize(SDL_Window*) bool
        +shutdown() void
        +beginFrame(FrameContext) void
        +submit(vector~RenderCommand~) void
        +endFrame() void
        +bufferToGpu(Camera&, string_view, bool) bool
        +createShadowMap(Camera&) GLuint
        +updateShadowMap(Camera&) void
        +getShadowMapSize(int&, int&) const void
        +takeScreenshot(string_view) void
    }
    
    class OpenGLBackend {
        -Renderer& owner
        -SDL_Window* window
        -bool usePersistentMappedVbo
        -GLuint vao, vbo, ibo
        -GLuint shadowMap, depthMapFBO
        -GpuProgram* gpuProgram
        -GpuProgram* shadowProgram
        -bool useCascadedShadows
        -int cascadeCount
        -vector~GLuint~ cascadeShadowMaps
        +fitDirectionalShadowMatrix() static void
        +computeCascadeFarPlanes() static void
        +enableCascadedShadows(int) void
        +disableCascadedShadows() void
    }
    
    class SdlGpuBackend {
        <<optional>>
        -Renderer& owner
        -SDL_Window* window
        -SDL_GpuDevice* device
        -SDL_GpuBuffer* vbo, ibo
        -SDL_GpuGraphicsPipeline* pipeline
    }
    
    IRenderBackend <|-- OpenGLBackend
    IRenderBackend <|-- SdlGpuBackend
```

### Shader System Hierarchy

```mermaid
classDiagram
    class Shader {
        <<base>>
        -GLuint id
        -string filePath
        -string shaderSrc
        +Shader(const char*)
        +Shader(string_view)
        +Shader()
        +load(const char*) void
        +load(string_view) void
        +getId() GLuint noexcept
        +createFromCache(string_view) shared_ptr~Shader~ static
        ~Shader()
    }
    
    class FragmentShader {
        +FragmentShader(const char*)
        +FragmentShader(string_view)
        +createFromCache(string_view) shared_ptr~FragmentShader~ static
        -createFragmentShader() void
    }
    
    class VertexShader {
        +VertexShader(const char*)
        +VertexShader(string_view)
        +createFromCache(string_view) shared_ptr~VertexShader~ static
        -createVertexShader() void
    }
    
    class ShaderCache {
        <<singleton>>
        -unordered_map~string, CacheEntry~ cache
        -mutex cacheMutex
        -size_t totalCompilations
        +getInstance() ShaderCache& static
        +getShader(string_view) shared_ptr~Shader~
        +invalidateAll() void
        +invalidate(string_view) void
        +getStats() pair~size_t, size_t~ const
        -compileAndCache(string_view) shared_ptr~Shader~
    }
    
    class Uniform {
        <<abstract>>
        -GLuint location
        +load() abstract void
        +getLocation() GLuint
        +setLocation(GLuint) void
    }
    
    class UniformMat4 {
        -glm::mat4 matrix
        +UniformMat4(glm::mat4&)
        +load() override void
        +set(glm::mat4&) void
    }
    
    class UniformVec3 {
        -glm::vec3 vector
        +UniformVec3(glm::vec3&)
        +load() override void
        +set(glm::vec3&) void
    }
    
    class UniformInt {
        -GLint i
        +UniformInt(GLint)
        +load() override void
        +set(GLint) void
    }
    
    class UniformLoader {
        -GLuint programId
        -unordered_map~string, unique_ptr~Uniform~~ uniforms
        +UniformLoader(GLuint)
        +addUniform(string_view, UniformPtr) void
        +get(string_view) Uniform*
        +load() const void
    }
    
    class GpuProgram {
        -GLuint id
        -unique_ptr~UniformLoader~ uniformLoader
        +GpuProgram()
        +getId() GLuint
        +attachShader(Shader&) void
        +use() const
        +getUniformLoader() UniformLoader*
    }
    
    Shader <|-- FragmentShader
    Shader <|-- VertexShader
    ShaderCache ..> Shader : manages
    Uniform <|-- UniformMat4
    Uniform <|-- UniformVec3
    Uniform <|-- UniformInt
    UniformLoader o-- Uniform : owns
    GpuProgram o-- UniformLoader : owns
```

### Scene System Hierarchy

```mermaid
classDiagram
    class SceneNode {
        +char name[256]
        +char material[256]
        +unique_ptr~Vertex[]~ vertexData
        +size_t vertexDataSize
        +glm::mat4 modelViewMatrix
        +GLuint startPosition
        +GLuint endPosition
        +GLenum primitiveMode
        +GLuint textureIds[4]
        +GLfloat boundingSphere
        +GLfloat lx, ly, lz
        +SceneNode()
        +SceneNode(SceneNode&&) noexcept
        +setName(string_view) void
        +setMaterial(string_view) void
    }
    
    class SceneGraphNode {
        +string name
        +glm::vec3 localTranslation
        +glm::vec3 localScale
        +glm::quat localRotation
        +glm::mat4 worldTransform
        +bool worldTransformDirty
        +SceneGraphNode::Ptr parent
        +vector~Ptr~ children
        +SceneNode sceneData
        +addChild(Ptr) Ptr
        +removeChild(Ptr) bool
        +findNodeByName(string_view) Ptr
        +getDescendants(vector~Ptr~&) const
        +markDirty() void
        +refreshWorldTransform() const
        +getWorldTransform() glm::mat4 const
        +forEach(function~void~Ptr~~) const
    }
    
    class SceneGraph {
        +vector~SceneGraphNode::Ptr~ roots
        +unordered_map~string, Ptr~ nodeByName
        +addRoot(Ptr) void
        +findNode(string_view) Ptr
        +getRoots() const vector~Ptr~
        +refreshAllTransforms() const
        +getAllNodes(vector~Ptr~&) const
    }
    
    class Material {
        +char name[256]
        +float ambient[3]
        +float diffuse[3]
        +float specular[3]
        +float transmittance[3]
        +float emission[3]
        +float shininess
        +float ior
        +float dissolve
        +int illum
        +char textureNames[4][256]
        +setName(string_view) void
        +setDiffuseTexName(string_view) void
    }
    
    class Texture {
        +unsigned width
        +unsigned height
        +unsigned bpp
        +int mode
        +shared_ptr~unsigned char[]~ data
        +isValid() bool const
        +getDataSize() size_t const
    }
    
    SceneGraph "1" *-- "*" SceneGraphNode : contains
    SceneGraphNode "1" o-- "1" SceneNode : holds sceneData
    SceneGraphNode "*" o-- "*" SceneGraphNode : parent/children
```

---

## 4. Data Flow Diagrams

### Application Startup Flow

```mermaid
sequenceDiagram
    participant Main as main()
    participant App as MyGLApp
    participant Config as ConfigLoader
    participant SDL as SDL3
    participant GL as OpenGL
    participant Renderer as Renderer
    participant Backend as OpenGLBackend
    
    Main->>App: MyGLApp(modelPath)
    App->>Config: ConfigLoader("app.cfg")
    Config-->>App: config loaded
    
    App->>SDL: SDL_Init(VIDEO | EVENTS)
    SDL-->>App: success
    
    App->>SDL: SDL_GL_SetAttribute(depth=24, doubleBuffer=1)
    App->>SDL: SDL_CreateWindow()
    SDL-->>App: window
    
    App->>SDL: SDL_GL_CreateContext(4.6/4.5/4.3/3.3)
    SDL-->>App: glContext
    
    App->>GL: gladLoadGLLoader()
    GL-->>App: loaded
    
    App->>Renderer: setBackend(OpenGLBackend)
    Backend->>GL: glEnable(DEPTH_TEST)
    
    App->>SDL: SDL_CreateThread(LoadScene)
    Note over Renderer,Backend: Scene loads in background thread
```

### Scene Loading Flow (Threaded)

```mermaid
flowchart TD
    A[LoadScene Thread] --> B{Cache exists?}
    B -->|Yes| C[Read BinCacheFileHeader]
    C --> D{Version match?}
    D -->|No| E[Build from OBJ]
    D -->|Yes| F[Deserialize Materials]
    F --> G[Deserialize SceneNodes]
    G --> H[Deserialize VertexData]
    H --> I[Deserialize Textures]
    I --> J[buildScene returns true]
    
    B -->|No| E
    E --> K[tiny_obj_loader ParseFromFile]
    K --> L[Extract Materials]
    L --> M[Create SceneNodes per material]
    M --> N[buildScene returns true]
    
    J --> O[main thread detects sceneLoaded]
    N --> O
    O --> P[bufferToGpu]
```

### Render Frame Flow

```mermaid
flowchart TD
    A[Frame Start] --> B[update frameContext]
    B --> C{Shadows enabled?}
    
    C -->|Yes| D[compute light frustum]
    D --> E{Cascaded shadows?}
    E -->|Yes| F[create cascade shadow maps]
    E -->|No| G[create single shadow map]
    F --> H[render shadow pass]
    G --> H
    
    C -->|No| I[beginFrame clear color/depth]
    H --> I
    
    I --> J[extract frustum from VP matrix]
    J --> K[collectVisibleNodes via AABB tree]
    K --> L{Occlusion culling?}
    L -->|Yes| M[query GPU occlusion results]
    L -->|No| N[sort by texture ID]
    M --> N
    
    N --> O[submit render commands to backend]
    O --> P[endFrame swap buffers]
    P --> Q[update perf stats HUD]
```

---

## 5. Render Pipeline

### GPU Resource Lifecycle

```mermaid
graph TD
    subgraph "Initialization"
        A[bufferToGpu called] --> B[Load textures from disk/cache]
        B --> C[glGenVertexArrays + glBindVertexArray]
        C --> D[glGenBuffers VBO/IBO]
        D --> E{Persistent mapping supported?}
        E -->|OpenGL 4.4+| F[glBufferStorage with PERSISTENT_BIT]
        E -->|Fallback| G[glBufferData with DYNAMIC_DRAW]
        F --> H[glMapBufferRange + memcpy]
        G --> H
        H --> I[glVertexAttribPointer setup]
    end
    
    subgraph "Shader Programs"
        J[Create GpuProgram] --> K[Load Vertex Shader]
        K --> L[Load Fragment Shader]
        L --> M[glLinkProgram]
        M --> N[Add Uniforms via UniformLoader]
        N --> O[UniformMat4 / UniformVec3 / UniformInt]
    end
    
    subgraph "Shadow Resources"
        P[Create Shadow Program] --> Q[Load depth shaders]
        Q --> R[glGenTextures shadowMap]
        R --> S[glGenFramebuffers depthMapFBO]
        S --> T[glFramebufferTexture2D]
    end
    
    subgraph "Per-Frame"
        U[beginFrame glClear] --> V[submit render commands]
        V --> W[gpuProgram->use + uniformLoader->load]
        W --> X[glDrawElements per visible node]
        X --> Y[endFrame SDL_GL_SwapWindow]
    end
    
    A --> J
    P --> R
    U --> V
```

### Memory Allocation Map

```mermaid
graph LR
    subgraph "Heap Allocations"
        A[Renderer instance]
        B[OpenGLBackend instance]
        C[SceneNodes vector]
        D[Textures map]
        E[GpuProgram instances]
        F[Uniform objects]
        G[ShaderCache entries]
    end
    
    subgraph "GPU Memory"
        H[VBO vertex data]
        I[IBO index data]
        J[Texture images]
        K[Shadow map textures]
        L[Cascade shadow maps]
        M[Framebuffer objects]
    end
    
    A --> B
    A --> C
    A --> D
    A --> E
    E --> F
    G -.->|caches| E
    H -.->|VBO data| I
    J -.->|glTexImage2D| K
    K -.->|FBO attachment| L
```

---

## 6. Memory Management

### Ownership Model

```mermaid
graph TB
    subgraph "Owner: Renderer"
        R[Renderer]
        R -->|owns|unique_ptr~ConfigLoader~ configLoader
        R -->|owns|unique_ptr~IRenderBackend~ backend
        R -->|owns|vector~SceneNode~ sceneNodes
        R -->|owns|vector~Vertex~ vertexData
        R -->|owns|vector~GLuint~ indices
        R -->|owns|unordered_map~string, Material~ materials
        R -->|owns|unordered_map~string, shared_ptr~Texture~~ textures
    end
    
    subgraph "Owner: OpenGLBackend"
        O[OpenGLBackend]
        O -->|owns|GpuProgram gpuProgram
        O -->|owns|GpuProgram shadowProgram
        O -->|owns|GLuint vao/vbo/ibo
        O -->|owns|GLuint shadowMap
        O -->|owns|GLuint depthMapFBO
        O -->|owns|vector~GLuint~ cascadeShadowMaps
    end
    
    subgraph "Owner: ShaderCache"
        SC[ShaderCache]
        SC -->|owns|unordered_map~string, CacheEntry~ cache
    end
    
    subgraph "Owner: Texture"
        T[Texture]
        T -->|owns|shared_ptr~unsigned char[]~ data
    end
    
    R --> O
    R --> SC
    T -.->|stored in| textures
```

### Thread Safety Model

```mermaid
flowchart TD
    subgraph "Main Thread"
        A[MyGLApp::start loop]
        A --> B[update input/camera]
        B --> C[renderer.render]
        C --> D[backend.submit commands]
        D --> E[backend.endFrame swap]
    end
    
    subgraph "Load Thread"
        F[LoadScene thread]
        F --> G[parse OBJ / read cache]
        G --> H[addSceneNode with mutex lock]
        H --> I[buildScene compute bounding spheres]
    end
    
    subgraph "Mutex Protection"
        M[(sceneDataMutex)]
        H -.->|acquire| M
        M -.->|release| H
    end
    
    G -->|sets sceneLoaded flag| A
```

---

## Appendix: Key Design Patterns

| Pattern | Location | Purpose |
|---------|----------|---------|
| **Strategy** | `IRenderBackend` + implementations | Swap rendering backends without changing Renderer |
| **Factory** | `ShaderCache::getInstance()` | Lazy shader compilation with caching |
| **Composite** | `SceneGraph` → `SceneGraphNode` → children | Hierarchical scene representation |
| **Observer** | Dirty flag pattern on `SceneGraphNode` | Deferred world transform computation |
| **Singleton** | `ShaderCache` | Global shader compilation cache |
| **RAII** | `unique_ptr`, `shared_ptr` throughout | Automatic resource cleanup |
| **Bridge** | `Renderer` ↔ `IRenderBackend` | Decouple rendering logic from API |

---

*Generated for sdlgl3-wavefront project*
*Date: 2026-08-10*
