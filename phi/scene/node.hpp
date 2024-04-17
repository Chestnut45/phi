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
    // 
    // NOTE: Do not instantiate this class directly! For now it is a bug that the
    // constructor is public (it has to be for the internal registry to be able to
    // construct nodes in-place, which is noticeably faster, especially in heavily
    // dynamic scenes with lots of nodes being created / deleted constantly)
    class Node
    {
        // Interface
        public:
            
            // TODO: Should be private...
            Node(Scene* scene, NodeID id);

            ~Node();

            // Default copy constructor/assignment
            Node(const Node&) = default;
            Node& operator=(const Node&) = default;

            // Default move constructor/assignment
            Node(Node&& other) = default;
            Node& operator=(Node&& other) = default;

            // Component management

            // Constructs a component in-place and assigns it to the node
            template <typename T, typename... Args>
            T& AddComponent(Args&&... args)
            {
                // Add the component
                T& component = scene->registry.emplace<T>(id, args...);

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
                return scene->registry.try_get<T>(id);
            }

            // Returns true if the node has all of the given components, or false otherwise
            template <typename... T>
            bool Has()
            {
                return scene->registry.all_of<T...>(id);
            }

            template <typename... T>
            bool HasAny()
            {
                return scene->registry.any_of<T...>(id);
            }

            // Deletes the given component type from the node, if it exists
            template <typename T>
            void RemoveComponent()
            {
                scene->registry.remove<T>(id);
            }

            // Delete this node from the scene entirely
            void Delete();

            // Hierarchy management
        
            // Adds a node to our list of children
            // If the node already has a parent, it is removed from that parent first
            void AddChild(Node* node);

            // Removes the given child node from our list of children, and
            // updates the child node's parent reference to be empty
            void RemoveChild(Node* node);

            // Returns a pointer to the parent node, or nullptr if we have none
            Node* GetParent() const;

            // Accessors

            // Gets the id of this node
            NodeID GetID() const { return id; };

            // Gets the scene that this node belongs to
            Scene* GetScene() const { return scene; };

            // Gets the list of children nodes by id
            const std::vector<Node*>& GetChildren() const { return children; };
        
        // Data / implementation
        private:

            // Pointer stability guarantee for components
            static constexpr auto in_place_delete = true;

            // Reference to the scene this node belongs to
            Scene* const scene = nullptr;

            // Unique identifier
            const NodeID id{entt::null};
            
            // Hierarchy data
            Node* parent = nullptr;
            std::vector<Node*> children;
        
        // Friends
        private:

            // Necessary for scenes to be able to create nodes
            friend class Scene;
    };
}