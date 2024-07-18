#pragma once

#include <cstdint>
#include <vector>
#include <type_traits>

#include <phi/scene/scene.hpp>

namespace Phi
{
    // Represents a single node within a scene
    // 
    // You may attach an arbitrary number of components to each node
    // 
    // Components may be any type, but a node may only have 1 component per type
    // 
    // Nodes may also have children, and you can traverse this hierarchy within
    // any component by inheriting from BaseComponent to get access to the node
    // the component is attached to, and then GetParent() / GetChildren() to traverse
    class Node
    {
        // Interface
        public:
            
            // NOTE: Do not instantiate this class directly! Instead, use the Scene::CreateNode*() methods.
            // Consider it a bug that this constructor is public
            // TODO: Don't use scene registry for node storage? (Pointer stability is a must...)
            Node(Scene& scene, NodeID id, const std::string& name);

            ~Node();

            // Delete copy constructor/assignment
            Node(const Node&) = delete;
            Node& operator=(const Node&) = delete;

            // Delete move constructor/assignment
            Node(Node&& other) = delete;
            Node& operator=(Node&& other) = delete;

            // Component management

            // Constructs a component in-place and assigns it to the node
            // NOTE: Pass your component's constructor arguments directly to this function:
            template <typename T, typename... Args>
            T& AddComponent(Args&&... args)
            {
                // Add the component
                T& component = scene.registry.emplace<T>(id, args...);

                // Set the node pointer if T is a BaseComponent
                if constexpr (std::is_base_of_v<BaseComponent, T>)
                {
                    component.SetNode(this);
                }

                return component;
            }

            // Returns a pointer to the given component, if it exists
            template <typename T>
            T* Get()
            {
                return scene.registry.try_get<T>(id);
            }

            // Returns true if the node has all of the given components, or false otherwise
            template <typename... T>
            bool Has()
            {
                return scene.registry.all_of<T...>(id);
            }

            template <typename... T>
            bool HasAny()
            {
                return scene.registry.any_of<T...>(id);
            }

            // Deletes the given component type from the node, if it exists
            template <typename T>
            void RemoveComponent()
            {
                scene.registry.remove<T>(id);
            }

            // Delete this node and all of its components from the scene entirely
            inline void Delete() const { scene.Delete(id); }

            // Hierarchy management
        
            // Adds a node to our list of children
            // If the node already has a parent, it is removed from that parent first
            void AddChild(Node* node);

            // Removes the given child node from our list of children, and
            // updates the child node's parent reference to be empty
            void RemoveChild(Node* node);

            // Returns a pointer to the parent node, or nullptr if we have none
            Node* GetParent() const;

            // Accessors / Mutators

            // Gets the id of this node
            NodeID GetID() const { return id; };

            // Gets the name of this node
            const std::string& GetName() const { return name; }

            // Sets the name of this node
            void SetName(const std::string& name) { this->name = name; }

            // Gets a reference to the scene that this node belongs to
            Scene& GetScene() const { return scene; }

            // Gets the list of children nodes by id
            const std::vector<Node*>& GetChildren() const { return children; }
        
        // Data / implementation
        private:

            // Pointer stability guarantee for nodes
            static constexpr auto in_place_delete = true;

            // Reference to the scene this node belongs to
            Scene& scene;

            // Identifiers
            const NodeID id;
            std::string name;
            
            // Hierarchy data
            Node* parent = nullptr;
            std::vector<Node*> children;

            // Necessary for scenes to manage / edit nodes
            friend class Scene;
    };
}