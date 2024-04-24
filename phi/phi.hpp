#pragma once

// Single include to access entire Phi engine

// Core systems / data structures
#include "core/app.hpp"
#include "core/file.hpp"
#include "core/input.hpp"
#include "core/logging.hpp"
#include "core/resource_manager.hpp"
#include "core/math/noise.hpp"
#include "core/math/rng.hpp"
#include "core/math/shapes.hpp"
#include "core/structures/free_list.hpp"
#include "core/structures/quadtree.hpp"
#include "core/structures/experimental/array_grid_3d.hpp"
#include "core/structures/experimental/hash_grid_3d.hpp"
#include "core/structures/experimental/hash_map.hpp"

// OpenGL resources
#include "graphics/cubemap.hpp"
#include "graphics/framebuffer.hpp"
#include "graphics/geometry.hpp"
#include "graphics/gpu_buffer.hpp"
#include "graphics/indirect.hpp"
#include "graphics/materials.hpp"
#include "graphics/shader.hpp"
#include "graphics/texture_2d.hpp"
#include "graphics/vertex.hpp"
#include "graphics/vertex_attributes.hpp"

// Scene management / components
#include "scene/node.hpp"
#include "scene/scene.hpp"
#include "scene/components/base_component.hpp"
#include "scene/components/camera.hpp"
#include "scene/components/transform.hpp"
#include "scene/components/collision/bounding_sphere.hpp"
#include "scene/components/lighting/directional_light.hpp"
#include "scene/components/lighting/point_light.hpp"
#include "scene/components/renderable/basic_mesh.hpp"
#include "scene/components/renderable/cpu_particle_effect.hpp"
#include "scene/components/renderable/cpu_particle_emitter.hpp"
#include "scene/components/renderable/skybox.hpp"
#include "scene/components/renderable/voxel_mesh_implicit.hpp"
#include "scene/components/renderable/voxel_mesh_splat.hpp"
#include "scene/components/renderable/voxel_object.hpp"