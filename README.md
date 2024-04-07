# Phi

Phi is a cross-platform 3D desktop application engine using C++ and OpenGL. It is still quite early in its development, and much of the API may be subject to frequent changes (see `Roadmap` for details).

I am developing Phi as a personal project alongside Harmonic - an upcoming falling-sand inspired frequency based roguelike - in the hopes that much of the game's systems can be made generic and reusable for other 3D application projects. It is currently not recommended to use this engine for anything other than research or quick prototyping. I do my best to add relevant comments to every feature I add, so exploring the source is the current recommended way to learn about the engine (until I have the time to put together a wiki).

## Engine Architecture

### Apps

Creating an application in Phi is pretty simple. Eventually there will be a script to generate the template for you, but for now use `templates/new_app.cpp` and `templates/new_app.hpp` as reference.

NOTE: For now, you also must add your app to the `CMakeLists.txt` file, following the format of the other internal applications / tools. This will likely also be handled by the template generation script once it's made.

- Inherit publicly from `Phi::App`, and then override Update(float) and Render()
- In the main entrypoint, create an instance of your application and call `App::Run()`

The structure of `Phi::App` is inspired by the MonoGame framework, and exists mainly to give you access to 4 functions:

- Constructor (for initialization logic)
- Destructor (for cleanup / resource management)
- Update(float) (called every frame, given elapsed time since last frame in seconds)
- Render() (called every frame after update)

More functionality is provided to user applications in the form of inherited helper class members such as `Phi::Input` and `Phi::RNG`, or can be added via including various parts of the engine.

### Scene Management

TODO: Explain `Phi::Scene`, `Phi::Node`, `Phi::BaseComponent`

### Resource Management / Rendering Abstraction

TODO: Explain RAII wrappers (`Phi::Texture2D`, `Phi::GPUBuffer`, etc.) and plans for `Phi::ResourceManager`

### Tools

Many tools are planned / in development, currently the most mature tool is `particle_effect_editor`.

![effect_editor_screenshot.png](https://github.com/Chestnut45/phi/blob/main/screenshots/effect_editor_screenshot.png)

## Roadmap

Currently, features are mostly being added in the order I need them for testing (or if I think something is super cool). Since many design decisions about the engine's scope are yet to be made, there will be a broad coverage of features with little depth until more structure emerges. This is not a complete list of all planned features, but expect anything that ends up here to eventually make it in.

Current version: 0.3.2

NOTE: Wondering where the commit history went? I had to recreate the repo due to many changed texture files bloating the .git pack files and history. Since it's early in the project's life, losing this history is fine, and well worth reducing the repo size by ~150mb. Internal engine data such as textures may eventually be stored entirely separately from the repo and downloaded as part of an installation process to optimize size further.

### Math Modules
- [ ] RNG
    - [x] Basic float / int distributions
    - [ ] Custom weighted distributions
- [ ] Noise
    - [ ] OpenSimplex2 integration
    - [ ] 1D, 2D, 3D sampling
- [ ] Shapes (Intersection testable)
    - [ ] 2D
        - [x] Rectangle / IRectangle
        - [ ] Circle
    - [ ] 3D
        - [x] Plane
        - [x] Frustum
        - [x] AABB
        - [x] Sphere
### Data Structures
- [x] Free List
- [x] Quadtree
- [ ] Experimental (waiting on better compiler support for c++23 for multiple arguments in overloaded subscript operator)
    - [ ] Hash Map
    - [ ] Array Grid 3D
    - [ ] Hash Grid 3D
### Core Modules
- [x] App
- [ ] File Management
    - [ ] File wrapper class
    - [ ] Document valid special paths (data://, user://, etc.)
- [ ] Input
    - [x] Keyboard input
    - [x] Mouse input
    - [ ] Gamepad input
    - [ ] Buffered inputs (bool IsKeyDownBuffered(key, numFrames))
- [x] Logging
- [ ] Resource Manager
    - [x] Texture2D management
    - [ ] Shader management
### Graphics Modules
- [x] Indirect draw command structures
- [x] Internal vertex formats
- [ ] Materials
    - [x] Basic Material
    - [ ] PBR Material
- [ ] RAII OpenGL Wrappers
    - [x] Cubemap
    - [x] Framebuffer
    - [x] GPU Buffer
    - [x] Shader
    - [x] Texture2D
    - [x] Vertex Attributes (VAO)
    - [ ] Texture Arrays
### Scene Management
- [x] Node hierarchy
- [x] Node composition (AddComponent<T>(...))
- [ ] Components
    - [x] Camera
    - [x] Transform
    - [x] Directional light
    - [ ] Point light
    - [ ] Bounding Sphere
        - [x] Intersection tests
        - [x] Auto transform / scale flags
        - [ ] Encompass child nodes
    - [ ] Renderable
        - [x] Skybox
        - [ ] CPU Particle Effect
            - [x] Batched indirect rendering pipeline
            - [x] Additive / Standard blend modes
            - [x] Loading / Saving from YAML files
            - [x] Effect controls
            - [x] Effect editor application
            - [ ] Expanded emitter API (support procedural generation not just YAML files)
        - [ ] GPU Particle Effect
            - [ ] Compute shader simulation
        - [ ] Basic Mesh
            - [x] Batched indirect rendering pipeline
            - [x] Procedural geometry generation
            - [x] Procedural normal generation
            - [ ] .obj loading
        - [ ] Voxel Mesh (Ray Traced)
            - [x] Batched indirect rendering pipeline
            - [ ] Loading from voxel model
        - [ ] Voxel Mesh (Instanced)
        - [ ] PBR Mesh

## Prerequisites

- CMake
- OpenGL 4.6
- C++20 compliant compiler
- C/C++ Tools VSCode extension

## Building

NOTE: On Windows, you may have to copy glew32d.dll from build/bin to C:\Windows\System32.

- CMake: Configure
- CMake: Set Build/Debug Target
- CMake: Build

## Tested Platforms

- GCC 12+ / CMake 3.27.4 / Ubuntu 23.10 / 6.5.0-10-lowlatency / NVIDIA RTX 3070ti on 535.129.03
- GCC 12+ / CMake 3.27.4 / Ubuntu 23.10 / 6.5.0-10-lowlatency / Intel Iris XE on Mesa 23.2.1-1ubuntu3
- GCC 13+ / CMake 3.27.8 / Windows 11 / NVIDIA RTX 3070ti on 532.09
- GCC 13+ / CMake 3.27.8 / Windows 11 / Intel Iris XE on 31.0.101.4255

## License

See `LICENSE` file at root of project.

This is tentative since I'm not sure about the legal details regarding including other libraries such as GLFW, Dear ImGui, etc., but my intention is to make the engine open source in case any part of it would be useful to anyone doing similar work.