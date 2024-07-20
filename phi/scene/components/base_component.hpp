#pragma once

namespace Phi
{
    // Forward declares
    class Node;
    class Scene;
    
    // Interface to inherit in order to get access to the node hierarchy from a component
    class BaseComponent
    {
        // Interface
        public:

            // Access to the node this component belongs to
            // Valid immediately after (but not during) the component's constructor
            Node* GetNode() const { return node; };

            // TODO: Design common component interface...
            // virtual void Ready() = 0;
            // virtual void Update(float delta) = 0;
            // virtual void Destroy() = 0;
        
        protected:

            // Pointer to the node that this component belongs to
            // NOTE: Set automatically during Node::AddComponent<T>(...)
            friend class Node;
            Node* node = nullptr;
    };
}