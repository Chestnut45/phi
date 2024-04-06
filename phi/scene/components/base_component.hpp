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

            Node* GetNode() const { return node; };

        private:

            friend class Node;

            void SetNode(Node* node) { this->node = node; };

            // The node that this component belongs to
            // Set by node on component creation
            Node* node = nullptr;
    };
}