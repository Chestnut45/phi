#pragma once

#include <cstdint>
#include <vector>

#include <phi/core/logging.hpp>
#include <phi/core/math/shapes.hpp>

#include <phi/core/structures/free_list.hpp>

namespace Phi
{
    // Represents a quadtree of arbitrary type
    template <typename T>
    class Quadtree
    {
        // Forward declarations
        struct Node;
        struct Element;
        struct ElementNode;

        // Interface
        public:

            // Creates a quadtree with the given root boundaries (extents)
            Quadtree(int left, int top, int right, int bottom);
            ~Quadtree();

            // Delete copy constructor/assignment
            Quadtree(const Quadtree&) = delete;
            Quadtree& operator=(const Quadtree&) = delete;

            // Delete move constructor/assignment
            Quadtree(Quadtree&& other) = delete;
            Quadtree& operator=(Quadtree&& other) = delete;

            // Insertion / Removal

            // Inserts the given element into the quadtree, ensuring
            // it is referenced in all nodes that intersect rect
            // Returns the index for the newly inserted element,
            // or 0 if it is not within the bounds of the quadtree's root
            int Insert(const T& data, const Rectangle& rect);

            // Removes the element at the given index
            void Remove(int index);

            // Lookup / Traversal

            // Gets a const reference to the element with the given index
            const T& Get(int index) const { return elements[index].data; };

            // Returns a list of element indices that intersect the given rectangle
            std::vector<int> FindElements(const Rectangle& rect) const;

            // Returns a list of element indices that may intersect with the given frustum
            // NOTE: May contain false positives
            std::vector<int> FindElements(const Frustum& frustum) const;

            // Cleanup

            // Deferred cleanup function
            // Descends down the tree and collapses any nodes with
            // 4 empty leaves as children to be a single empty leaf
            void Cleanup();

            // Removes all elements from the quadtree, leaving the
            // node structure intact (can be collapsed with cleanup)
            void Clear();

            // Resets the entire quadtree to initial state
            // Removes all nodes, elements, and element nodes
            void Reset();

            // Mutators
            
            // Sets the maximum depth for this quadtree
            // NOTE: Only valid to call on an empty quadtree
            // TODO: Rebuild self on max depth change?
            void SetMaxDepth(int depth) { maxDepth = depth; };

            // Sets the maximum depth for this quadtree
            // NOTE: Only valid to call on an empty quadtree
            // TODO: Rebuild self on max elements per node change?
            void SetMaxElementsPerNode(int max) { maxElementsPerNode = max; };

            // Accessors

            // Returns the maximum depth for this quadtree
            int GetMaxDepth() const { return maxDepth; };

            // Returns the maximum number of elements before a node splits
            int GetMaxElementsPerNode() const { return maxElementsPerNode; };

            // Returns the number of elements in this quadtree
            size_t Size() const { return elements.Count(); };

            // Returns the number of nodes in the tree including branches and leaves
            size_t NumNodes() const;

            // Returns the number of leaf nodes in the tree
            size_t NumLeaves() const;

            // Returns a list of all nodes' bounding rectangles (useful for generating wireframe data)
            std::vector<Rectangle> GetRects() const;

        // Data / implementation
        private:

            // Represents a single quadtree node
            struct Node
            {
                // Index of the first child node if this node is a
                // branch, or the first element if this is a leaf
                // -1 if this node is an empty leaf
                int32_t first = -1;

                // -1 if this node is a branch, or the number
                // of elements we have if this node is a leaf
                int32_t count = 0;

                // Depth of the node
                int depth = 0;

                // Center coordinates and half-sizes
                float cx, cy, hx, hy;
            };

            // Represents a single element in the quadtree
            struct Element
            {
                // User data
                T data;

                // Rectangle that bounds this element
                Rectangle rect{-1, 1, 1, -1};
            };

            // Represents a reference to an element, since a single
            // element may occupy multiple nodes in the quadtree
            struct ElementNode
            {
                // Index of the element this node references
                int element;

                // The next element in the leaf node or -1 for the end of the list
                int next = -1;
            };

            // Internal data structures

            // Storage of all elements in the quadtree
            FreeList<Element> elements;

            // Storage of all element nodes (references) in the quadtree
            FreeList<ElementNode> elementNodes;

            // Storage of all nodes in the quadtree (branches / leaves)
            // Index 0 is always the root node
            std::vector<Node> nodes;

            // Bounding rectangle for the root node / quadtree extents
            Rectangle rootRect;

            // Maximum depth allowed
            int maxDepth = 8;

            // The max number of element nodes a node may have before
            // it attempts to split (may fail due to maxDepth)
            int maxElementsPerNode = 2;

            // The first free node to reclaim or -1 if none exists
            int firstFree = -1;

            // Helper functions

            // Splits the given node into 4 smaller nodes
            // NOTE: Ignores maxDepth, caller must enforce
            void Split(int nodeIndex);

            // Adds an element node to the given leaf node
            void AddElementNode(int nodeIndex, int element);

            // Gets an AABB for the given node
            AABB GetAABB(int nodeIndex) const;
    };

    // Implementation

    template <typename T>
    Quadtree<T>::Quadtree(int left, int top, int right, int bottom)
        : rootRect(left, top, right, bottom)
    {
        // Insert the empty leaf root node
        Node node;
        node.cx = (left + right) * 0.5f;
        node.cy = (top + bottom) * 0.5f;
        node.hx = right - node.cx;
        node.hy = top - node.cy;
        nodes.push_back(node);
    }

    template <typename T>
    Quadtree<T>::~Quadtree()
    {
    }

    template <typename T>
    int Quadtree<T>::Insert(const T& data, const Rectangle& rect)
    {
        // Return early if rect is out of bounds
        if (!rect.Intersects(rootRect)) return 0;

        // Create node list
        std::vector<int> toProcess;

        // Add the element to the internal list
        Element element;
        element.data = data;
        element.rect = rect;
        int elementIndex = elements.Insert(element);

        // Traverse the tree from root down, inserting element nodes where necessary
        toProcess.push_back(0);
        while (toProcess.size() > 0)
        {
            // Grab the next node to process
            int nodeIndex = toProcess.back();
            Node& node = nodes[nodeIndex];
            toProcess.pop_back();

            if (node.count == -1)
            {
                // Node is a branch, process all child nodes that intersect the rectangle

                // First child index
                const int fc = node.first;

                if (rect.top > node.cy)
                {
                    // Check the tl and tr child nodes
                    if (rect.left <= node.cx) toProcess.push_back(fc);
                    if (rect.right > node.cx) toProcess.push_back(fc + 1);
                }

                if (rect.bottom <= node.cy)
                {
                    // Check the bl and br child nodes
                    if (rect.left <= node.cx) toProcess.push_back(fc + 2);
                    if (rect.right > node.cx) toProcess.push_back(fc + 3);
                }
            }
            else
            {
                // Node is a leaf that intersects the rect, must insert element node
                AddElementNode(nodeIndex, elementIndex);

                // Split if we've reached the limit of elements per node,
                // but only if we haven't reached the maximum depth yet
                if (node.count > maxElementsPerNode && node.depth < maxDepth)
                {
                    Split(nodeIndex);
                }
            }
        }

        return elementIndex;
    }

    template <typename T>
    void Quadtree<T>::Remove(int index)
    {
        // Grab the element for bounds access
        const Element& toRemove = elements[index];

        // Remove all element nodes

        // Init list
        std::vector<int> toProcess;

        // Traverse the tree from root down
        toProcess.push_back(0);
        while (toProcess.size() > 0)
        {
            // Grab the next node to process
            int nodeIndex = toProcess.back();
            Node& node = nodes[nodeIndex];
            toProcess.pop_back();

            if (node.count == -1)
            {
                // Node is a branch, process all child nodes that intersect the rectangle

                // First child index
                const int fc = node.first;

                if (toRemove.rect.top > node.cy)
                {
                    // Check the tl and tr child nodes
                    if (toRemove.rect.left <= node.cx) toProcess.push_back(fc);
                    if (toRemove.rect.right > node.cx) toProcess.push_back(fc + 1);
                }

                if (toRemove.rect.bottom <= node.cy)
                {
                    // Check the bl and br child nodes
                    if (toRemove.rect.left <= node.cx) toProcess.push_back(fc + 2);
                    if (toRemove.rect.right > node.cx) toProcess.push_back(fc + 3);
                }
            }
            else
            {
                // Node is a leaf that intersects the rect, remove all element nodes for the given element
                int currentEN = node.first;
                int prevEN = currentEN;
                while (currentEN != -1)
                {
                    // Remove element nodes that point to the to be removed element
                    if (elementNodes[currentEN].element == index)
                    {
                        // Check if this is node.first
                        if (prevEN == currentEN)
                        {
                            // Special case, must update node.first
                            node.first = elementNodes[currentEN].next;
                            elementNodes.Erase(currentEN);
                        }
                        else
                        {
                            // Standard case, point previous element node to our next
                            elementNodes[prevEN].next = elementNodes[currentEN].next;
                            elementNodes.Erase(currentEN);
                        }

                        // Update the count
                        node.count--;
                    }

                    // Store previous element node and grab the next one
                    prevEN = currentEN;
                    currentEN = elementNodes[currentEN].next;
                }
            }
        }

        // Remove the actual element
        elements.Erase(index);
    }

    template <typename T>
    std::vector<int> Quadtree<T>::FindElements(const Rectangle& rect) const
    {
        // Lists
        std::vector<int> foundElements, toProcess;

        // Return early if rect is out of bounds
        if (!rect.Intersects(rootRect)) return foundElements;

        // Traverse the tree from root down, inserting element nodes where necessary
        toProcess.push_back(0);
        while (toProcess.size() > 0)
        {
            // Grab the next node to process
            int nodeIndex = toProcess.back();
            const Node& node = nodes[nodeIndex];
            toProcess.pop_back();

            if (node.count == -1)
            {
                // Node is a branch, process all child nodes that intersect the rectangle

                // First child index
                const int fc = node.first;

                if (rect.top > node.cy)
                {
                    // Check the tl and tr child nodes
                    if (rect.left <= node.cx) toProcess.push_back(fc);
                    if (rect.right > node.cx) toProcess.push_back(fc + 1);
                }

                if (rect.bottom <= node.cy)
                {
                    // Check the bl and br child nodes
                    if (rect.left <= node.cx) toProcess.push_back(fc + 2);
                    if (rect.right > node.cx) toProcess.push_back(fc + 3);
                }
            }
            else
            {
                // Node is a leaf that intersects the rect, add all
                // elements in this leaf that aren't already in the list
                int nextEN = node.first;
                while (nextEN != -1)
                {
                    // Grab the element index
                    int element = elementNodes[nextEN].element;

                    // Make sure we haven't already added it
                    const auto& it = std::find(foundElements.begin(), foundElements.end(), element);
                    if (it == foundElements.end())
                    {
                        // Only add it if the rectangles intersect
                        if (elements[element].rect.Intersects(rect)) foundElements.push_back(element);
                    }

                    // Check next element
                    nextEN = elementNodes[nextEN].next;
                }
            }
        }

        return foundElements;
    }

    template <typename T>
    std::vector<int> Quadtree<T>::FindElements(const Frustum& frustum) const
    {
        // Lists
        std::vector<int> foundElements, toProcess;

        // Return early if rect is out of bounds
        if (!AABB(rootRect).IntersectsFast(frustum)) return foundElements;

        // Traverse the tree from root down
        toProcess.push_back(0);
        while (toProcess.size() > 0)
        {
            // Grab the next node to process
            int nodeIndex = toProcess.back();
            const Node& node = nodes[nodeIndex];
            toProcess.pop_back();

            if (node.count == -1)
            {
                // Node is a branch, process all child nodes that intersect the rectangle
                for (int i = node.first; i < node.first + 4; i++)
                {
                    if (GetAABB(i).IntersectsFast(frustum)) toProcess.push_back(i);
                }
            }
            else
            {
                // Node is a leaf that intersects the frustum, add all
                // elements in this leaf that aren't already in the list
                int nextEN = node.first;
                while (nextEN != -1)
                {
                    // Grab the element index
                    int element = elementNodes[nextEN].element;

                    // Make sure we haven't already added it
                    const auto& it = std::find(foundElements.begin(), foundElements.end(), element);
                    if (it == foundElements.end())
                    {
                        foundElements.push_back(element);

                        // OLD: Only add it if the rectangle intersects the frustum
                        // if (AABB(elements[element].rect).IntersectsFast(frustum)) foundElements.push_back(element);
                    }

                    // Check next element
                    nextEN = elementNodes[nextEN].next;
                }
            }
        }

        return foundElements;
    }

    template <typename T>
    void Quadtree<T>::Cleanup()
    {
        // Init list
        std::vector<int> toProcess;

        // Only process the root node if it is a branch
        if (nodes[0].count == -1)
        {
            toProcess.push_back(0);
        }

        // Process all subsequent branches
        while (toProcess.size() > 0)
        {
            const int nodeIndex = toProcess.back();
            toProcess.pop_back();
            
            // Grab a reference to the branch node to process
            Node& node = nodes[nodeIndex];

            // Count the number of empty leaf children
            int emptyLeaves = 0;
            for (int i = 0; i < 4; i++)
            {
                // Calculate the child node's index
                int ci = node.first + i;

                // Grab a const reference to the child node
                const Node& child = nodes[ci];

                // Check if it is an empty leaf
                if (child.count == 0) emptyLeaves++;

                // If it's a branch, add it to the toProcess list
                else if (child.count == -1) toProcess.push_back(ci);
            }

            // Compact any nodes with 4 empty leaves for children
            if (emptyLeaves == 4)
            {
                // Add child nodes to the free list
                nodes[node.first].first = firstFree;
                firstFree = node.first;

                // Make this node an empty leaf now
                node.first = -1;
                node.count = 0;
            }
        }
    }

    template <typename T>
    void Quadtree<T>::Clear()
    {
        // Clear internal element containers
        elements.Clear();
        elementNodes.Clear();

        for (auto& node : nodes)
        {
            // Make all leaf nodes empty
            if (node.count != -1)
            {
                node.first = -1;
                node.count = 0;
            }
        }
    }

    template <typename T>
    void Quadtree<T>::Reset()
    {
        // Clear all internal containers
        elements.Clear();
        elementNodes.Clear();
        nodes.clear();

        // Reset the free pointer
        firstFree = -1;

        // Re-insert the empty leaf root node
        Node node;
        node.cx = (rootRect.left + rootRect.right) * 0.5f;
        node.cy = (rootRect.top + rootRect.bottom) * 0.5f;
        node.hx = rootRect.right - node.cx;
        node.hy = rootRect.top - node.cy;
        nodes.push_back(node);
    }

    template <typename T>
    size_t Quadtree<T>::NumNodes() const
    {
        // Init list
        std::vector<int> toProcess;
        toProcess.push_back(0);

        // Init counter
        size_t nodeCount = 0;

        // Process all nodes
        while (toProcess.size() > 0)
        {
            nodeCount++;

            // Grab the next node to process
            const int nodeIndex = toProcess.back();
            toProcess.pop_back();
            
            // Grab a reference
            const Node& node = nodes[nodeIndex];

            // Process all child nodes if this is a branch
            if (node.count == -1)
            {
                // Branch, add all child nodes to process
                toProcess.push_back(node.first);
                toProcess.push_back(node.first + 1);
                toProcess.push_back(node.first + 2);
                toProcess.push_back(node.first + 3);
            }
        }

        return nodeCount;
    }

    template <typename T>
    size_t Quadtree<T>::NumLeaves() const
    {
        // Init list
        std::vector<int> toProcess;
        toProcess.push_back(0);

        // Init counter
        size_t nodeCount = 0;

        // Process all nodes
        while (toProcess.size() > 0)
        {
            // Grab the next node to process
            const int nodeIndex = toProcess.back();
            toProcess.pop_back();
            
            // Grab a reference
            const Node& node = nodes[nodeIndex];

            // Process all child nodes if this is a branch
            if (node.count == -1)
            {
                // Branch, add all child nodes to process
                toProcess.push_back(node.first);
                toProcess.push_back(node.first + 1);
                toProcess.push_back(node.first + 2);
                toProcess.push_back(node.first + 3);
            }
            else
            {
                // Only increase counter if this is a leaf
                nodeCount++;
            }
        }

        return nodeCount;
    }

    template <typename T>
    std::vector<Rectangle> Quadtree<T>::GetRects() const
    {
        std::vector<Rectangle> rects;
        std::vector<int> toProcess;

        // Process down from the root
        toProcess.push_back(0);
        while (toProcess.size() > 0)
        {
            // Grab the next node to process
            const int nodeIndex = toProcess.back();
            toProcess.pop_back();

            // Grab a reference to the node
            const Node& node = nodes[nodeIndex];

            if (node.count == -1)
            {
                // Node is a branch, push back all child nodes
                const int fc = node.first;
                toProcess.push_back(fc);
                toProcess.push_back(fc + 1);
                toProcess.push_back(fc + 2);
                toProcess.push_back(fc + 3);
            }
            else
            {
                // Node is a leaf, add our rectangle to the list
                rects.push_back(Rectangle(node.cx - node.hx, node.cy + node.hy, node.cx + node.hx, node.cy - node.hy));
            }
        }

        return rects;
    }

    template <typename T>
    void Quadtree<T>::Split(int nodeIndex)
    {
        // Index of new first child
        int fc;

        // Check if there is a free run of 4 nodes to reclaim
        if (firstFree == -1)
        {
            // There are no free nodes, push back 4 new nodes

            // Index of new first child
            fc = nodes.size();

            // Add child nodes
            Node child;
            nodes.push_back(child);
            nodes.push_back(child);
            nodes.push_back(child);
            nodes.push_back(child);
        }
        else
        {
            // There exists a run of 4 empty leaf nodes to reclaim without adding to the vector
            
            // Set our first child to be the first free, and
            // update first free to be its first free (should be valid or -1)
            fc = firstFree;
            firstFree = nodes[fc].first;
            nodes[fc].first = -1;
        }

        // Update child nodes

        // Grab a reference to the current node;
        const Node& node = nodes[nodeIndex];

        // Calculate new dimensions and depth
        float hhx = node.hx * 0.5f;
        float hhy = node.hy * 0.5f;
        int newDepth = node.depth + 1;

        // TL
        nodes[fc].cx = node.cx - hhx;
        nodes[fc].cy = node.cy + hhy;
        nodes[fc].hx = hhx;
        nodes[fc].hy = hhy;
        nodes[fc].depth = newDepth;

        // TR
        nodes[fc + 1].cx = node.cx + hhx;
        nodes[fc + 1].cy = node.cy + hhy;
        nodes[fc + 1].hx = hhx;
        nodes[fc + 1].hy = hhy;
        nodes[fc + 1].depth = newDepth;
        
        // BL
        nodes[fc + 2].cx = node.cx - hhx;
        nodes[fc + 2].cy = node.cy - hhy;
        nodes[fc + 2].hx = hhx;
        nodes[fc + 2].hy = hhy;
        nodes[fc + 2].depth = newDepth;
        
        // BR
        nodes[fc + 3].cx = node.cx + hhx;
        nodes[fc + 3].cy = node.cy - hhy;
        nodes[fc + 3].hx = hhx;
        nodes[fc + 3].hy = hhy;
        nodes[fc + 3].depth = newDepth;

        // Transfer all element nodes to children
        int nextEN = node.first;
        while (nextEN != -1)
        {
            // Grab the element that the element node points to
            int elementIndex = elementNodes[nextEN].element;
            Element& e = elements[elementIndex];

            // Decide which child nodes to add an element node to
            if (e.rect.top > nodes[nodeIndex].cy)
            {
                // Check the tl and tr child nodes
                if (e.rect.left <= nodes[nodeIndex].cx) AddElementNode(fc, elementIndex);
                if (e.rect.right > nodes[nodeIndex].cx) AddElementNode(fc + 1, elementIndex);
            }

            if (e.rect.bottom <= nodes[nodeIndex].cy)
            {
                // Check the bl and br child nodes
                if (e.rect.left <= nodes[nodeIndex].cx) AddElementNode(fc + 2, elementIndex);
                if (e.rect.right > nodes[nodeIndex].cx) AddElementNode(fc + 3, elementIndex);
            }

            // Check next element node and remove the old one
            int old = nextEN;
            nextEN = elementNodes[nextEN].next;
            elementNodes.Erase(old);
        }

        // Make the current node a branch pointing to the new children
        nodes[nodeIndex].count = -1;
        nodes[nodeIndex].first = fc;

        // Recursively split new children if necessary and possible
        for (int i = fc; i < fc + 4; i++)
        {
            if (nodes[i].count > maxElementsPerNode && nodes[i].depth < maxDepth) Split(i);
        }
    }

    template <typename T>
    void Quadtree<T>::AddElementNode(int nodeIndex, int element)
    {
        // Grab a reference to the given node
        Node& node = nodes[nodeIndex];

        // Create the new element node
        ElementNode en;
        en.element = element;
        en.next = node.first;

        // Insert it and update the node
        node.first = elementNodes.Insert(en);
        node.count++;
    }

    template <typename T>
    AABB Quadtree<T>::GetAABB(int nodeIndex) const
    {
        const Node& node = nodes[nodeIndex];
        return std::move(AABB(Rectangle(node.cx - node.hx, node.cy + node.hy, node.cx + node.hx, node.cy - node.hy)));
    }
}