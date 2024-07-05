#include "node.hpp"

namespace Phi
{
    Node::Node(Scene* scene, NodeID id, const std::string& name)
        :
        scene(scene),
        id(id),
        name(name)
    {
    }

    Node::~Node()
    {
    }

    void Node::Delete()
    {
        scene->Delete(id);
    }

    void Node::AddChild(Node* node)
    {
        // Guard against nullptr
        if (node)
        {

            // Get the node's parent
            Node* parent = node->GetParent();
            if (parent)
            {
                // Early out if the node is already one of our children
                if (parent == this) return;

                // Remove existing relationship
                parent->RemoveChild(node);
            }

            // Update references
            children.push_back(node);
            node->parent = this;
        }
    }

    void Node::RemoveChild(Node* node)
    {
        // Guard against nullptr
        if (node)
        {
            const auto& it = std::find(children.begin(), children.end(), node);
            if (it != children.end())
            {
                node->parent = nullptr;
                children.erase(it);
            }
        }
    }

    Node* Node::GetParent() const
    {
        return parent;
    }
}