#pragma once

#include <algorithm>
#include <cstdint>
#include <vector>

namespace Phi
{
    // TODO: Extract hash map implementation to Phi::HashMap<K, V>
    //
    // Represents a sparse regular 3D grid of arbitrary data
    // Provides amortized O(1) time complexity for insert, search, and erase operations
    // Restrictions: T must be default-constructible to use the operator[] overload for inserts
    //
    // Implementation:
    // The internal data structure is implemented as a custom hash table
    // All elements are stored contiguously in a std::vector for efficient iteration
    // Robin hood hashing with backward shift deletion is used to keep average probe
    // sequence length low when collisions do occur
    template <typename T>
    class HashGrid3D
    {
        // Interface
        public:

            HashGrid3D();
            ~HashGrid3D();

            // Default copy constructor/assignment
            HashGrid3D(const HashGrid3D&) = default;
            HashGrid3D& operator=(const HashGrid3D&) = default;

            // Default move constructor/assignment
            HashGrid3D(HashGrid3D&& other) = default;
            HashGrid3D& operator=(HashGrid3D&& other) = default;

            // Stores element with position (key) for better iteration
            struct GridElement
            {
                // Position on the grid
                int32_t x, y, z;

                // Your element
                T data;

                // Empty constructor
                GridElement(int32_t x, int32_t y, int32_t z)
                    : x(x), y(y), z(z)
                {
                }

                // Constructs element in place by forwarding arguments to T's constructor
                template <typename... Args>
                GridElement(int32_t x, int32_t y, int32_t z, Args&&... args)
                    : x(x), y(y), z(z), data(std::forward<Args>(args)...)
                {
                }

                // Default copy constructor/assignment
                GridElement(const GridElement&) = default;
                GridElement& operator=(const GridElement&) = default;

                // Default move constructor/assignment
                GridElement(GridElement&& other) = default;
                GridElement& operator=(GridElement&& other) = default;
            };

            // Data access / modification

            // Fast read-write access to the element at the given location
            // Creates an element if no element exists at that location
            T& operator()(int32_t x, int32_t y, int32_t z)
            {
                // Calculate hash and initial values
                uint64_t hash = Hash(x, y, z);
                uint32_t bucketIndex = hash & (buckets.size() - 1);
                uint16_t fingerprint = hash;
                uint16_t distance = 0;

                // Search through the buckets until the element is found or known to be absent
                while (true)
                {
                    // Grab a reference to the current bucket
                    Bucket& currentBucket = buckets[bucketIndex];

                    // Early out if the element does not exist
                    if (currentBucket.empty || distance > currentBucket.distance) break;

                    // Assuming the bucket exists, check the fingerprint
                    if (currentBucket.fingerprint == fingerprint)
                    {
                        // If the key also matches, we've found the element
                        GridElement& e = elements[currentBucket.index];
                        if (x == e.x && y == e.y && z == e.z) return e.data;
                    }

                    // If the current bucket does not trigger a return or break, check the next bucket
                    bucketIndex = ++bucketIndex & (buckets.size() - 1);
                    distance++;
                }
                
                // If we reach this point, the element is known to be absent and must be inserted
                uint32_t elementIndex = elements.size();
                Bucket newBucket = Bucket(distance, fingerprint, elementIndex);
                elements.emplace_back(x, y, z);

                // Update load factor and rehash if necessary
                UpdateLoadFactor();
                Rehash();

                // Place the bucket
                while (true)
                {
                    // Grab a reference to the current bucket
                    Bucket& currentBucket = buckets[bucketIndex];

                    if (currentBucket.empty)
                    {
                        // Place the final bucket and return the data we inserted
                        currentBucket = newBucket;
                        return elements[elementIndex].data;
                    }

                    // Current bucket is not empty, swap with new bucket if necessary
                    if (newBucket.distance > currentBucket.distance) std::swap(newBucket, currentBucket);

                    // Check next bucket and increase distance
                    bucketIndex = ++bucketIndex & (buckets.size() - 1);
                    newBucket.distance++;
                }
            };

            // Returns a pointer to the element at the given location, or
            // nullptr if no element exists at that location (does not create)
            T* At(int32_t x, int32_t y, int32_t z);

            // Constructs an element in-place at the given location
            // Saves one copy over using operator[] for inserts
            template <typename... Args>
            void Emplace(int32_t x, int32_t y, int32_t z, Args&&... args);

            // Erases the element at the given location, if it exists
            void Erase(int32_t x, int32_t y, int32_t z);

            // Erases all elements / buckets in the grid
            void Clear();

            // Accessors / properties

            // Returns a const reference to the internal vector of elements
            const std::vector<GridElement>& Elements() const { return elements; };

            // Returns the number of elements in the grid
            size_t Size() const { return elements.size(); };

            // Returns the ratio of elements to buckets in the internal hash map
            float LoadFactor() const { return loadFactor; };

            // TODO: Iterators
        
        // Data / implementation
        private:

            // Constants and defaults
            static constexpr uint8_t MIN_BUCKET_EXPONENT = 4;
            static constexpr uint8_t MAX_BUCKET_EXPONENT = 32;
            static constexpr uint8_t INITIAL_BUCKET_EXPONENT = 16;

            // Internal types
            struct Bucket
            {
                bool empty = true;
                uint16_t distance = 0; // Distance to home bucket
                uint16_t fingerprint = 0; // Last 2 bytes of hash
                uint32_t index = 0; // Index into the vector of elements
                
                // Constructors
                Bucket() = default;
                Bucket(uint16_t distance, uint16_t fingerprint, uint32_t index)
                    : empty(false), distance(distance), fingerprint(fingerprint), index(index)
                {
                }

                // Default copy constructor/assignment
                Bucket(const Bucket&) = default;
                Bucket& operator=(const Bucket&) = default;

                // Default move constructor/assignment
                Bucket(Bucket&& other) = default;
                Bucket& operator=(Bucket&& other) = default;
            };

            // Contiguous storage of all elements in the grid
            std::vector<GridElement> elements;

            // Indexing structure
            // A dynamic array of buckets
            std::vector<Bucket> buckets;

            // Internal statistics
            float loadFactor = 0.0f;
            float minLoad = 0.1f;
            float maxLoad = 0.9f;

            // TODO: Tracking for smart search at high load factors
            uint16_t minDistance = 0;
            uint16_t maxDistance = 0;

            // Represents the size of the bucket array, expressed as 2^n
            uint8_t bucketSizeExponent = INITIAL_BUCKET_EXPONENT;
            

            // Calculates a hash for the given input coordinate
            inline uint64_t Hash(int32_t x, int32_t y, int32_t z)
            {
                // Only take the least significant 20 bits + the sign of each coordinate
                uint64_t xComponent = (x | ((x < 0) << 20)) & 0x1fffff;
                uint64_t yComponent = (y | ((y < 0) << 20)) & 0x1fffff;
                uint64_t zComponent = (z | ((z < 0) << 20)) & 0x1fffff;
                uint64_t hash = (xComponent << 42) | (yComponent << 21) | zComponent;

                // Finalize and return the mixed value
                hash = (hash ^ (hash >> 30)) * 0xbf58476d1ce4e5b9;
                hash = (hash ^ (hash >> 27)) * 0x94d049bb133111eb;
                hash ^= (hash >> 31);
                return hash;
            }

            // Calculates the most efficient size given the current
            // load factor, then rehashes every element on the grid
            void Rehash();

            // Updates the load factor cached value
            void UpdateLoadFactor() { loadFactor = (float)elements.size() / buckets.size(); };
    };

    // Template implementation
    
    template <typename T>
    HashGrid3D<T>::HashGrid3D()
    {
        buckets.resize(1 << bucketSizeExponent);
    }

    template <typename T>
    HashGrid3D<T>::~HashGrid3D()
    {

    }

    template <typename T>
    T* HashGrid3D<T>::At(int32_t x, int32_t y, int32_t z)
    {
        // Calculate hash and initial values
        uint64_t hash = Hash(x, y, z);
        uint32_t bucketIndex = hash & (buckets.size() - 1);
        uint16_t fingerprint = hash;
        uint16_t distance = 0;

        // Search through the buckets until the element is found or known to be absent
        while (true)
        {
            // Grab a reference to the current bucket
            Bucket& currentBucket = buckets[bucketIndex];

            // Early out if the element does not exist
            if (currentBucket.empty || distance > currentBucket.distance) return nullptr;

            // Assuming the bucket exists, check the fingerprint
            if (currentBucket.fingerprint == fingerprint)
            {
                // If the key also matches, we've found the element
                GridElement& e = elements[currentBucket.index];
                if (x == e.x && y == e.y && z == e.z) return &e.data;
            }

            // If the current bucket does not trigger a return, check the next bucket
            bucketIndex = ++bucketIndex & (buckets.size() - 1);
            distance++;
        }
    }

    template <typename T>
    template <typename... Args>
    void HashGrid3D<T>::Emplace(int32_t x, int32_t y, int32_t z, Args&&... args)
    {
        // Calculate hash and initial values
        uint64_t hash = Hash(x, y, z);
        uint32_t bucketIndex = hash & (buckets.size() - 1);
        uint16_t fingerprint = hash;
        uint16_t distance = 0;

        // Search through the buckets until the element is found or known to be absent
        while (true)
        {
            // Grab a reference to the current bucket
            Bucket& currentBucket = buckets[bucketIndex];

            // Early out if the element does not exist
            if (currentBucket.empty || distance > currentBucket.distance) break;

            // Assuming the bucket exists, check the fingerprint
            if (currentBucket.fingerprint == fingerprint)
            {
                // If the key also matches, we can just update the existing element and return
                GridElement& e = elements[currentBucket.index];
                if (x == e.x && y == e.y && z == e.z)
                {
                    e.data = T(std::forward<Args>(args)...);
                    return;
                }
            }

            // If the current bucket does not trigger a return or break, check the next bucket
            bucketIndex = ++bucketIndex & (buckets.size() - 1);
            distance++;
        }

        // Element does not exist and must be inserted
        Bucket newBucket = Bucket(distance, fingerprint, elements.size());
        elements.emplace_back(x, y, z, std::forward<Args>(args)...);

        // Update load factor and rehash if necessary
        UpdateLoadFactor();
        Rehash();

        // Place the bucket
        while (true)
        {
            // Grab a reference to the current bucket
            Bucket& currentBucket = buckets[bucketIndex];

            if (currentBucket.empty)
            {
                // We are finished with the insert operation
                currentBucket = newBucket;
                return;
            }

            // Swap and continue probing for displaced bucket if we are further from home
            if (newBucket.distance > currentBucket.distance) std::swap(newBucket, currentBucket);

            // Check next bucket and increase distance
            bucketIndex = ++bucketIndex & (buckets.size() - 1);
            newBucket.distance++;
        }
    }

    template <typename T>
    void HashGrid3D<T>::Erase(int32_t x, int32_t y, int32_t z)
    {
        // Calculate hash and initial values
        uint64_t hash = Hash(x, y, z);
        uint32_t bucketIndex = hash & (buckets.size() - 1);
        uint16_t fingerprint = hash;
        uint16_t distance = 0;

        // Search through the buckets until the element is found or known to be absent
        while (true)
        {
            // Grab a reference to the current bucket
            Bucket& currentBucket = buckets[bucketIndex];

            // Early out if the element does not exist
            if (currentBucket.empty || distance > currentBucket.distance) return;

            // Assuming the bucket exists, check the fingerprint
            if (currentBucket.fingerprint == fingerprint)
            {
                // If the key also matches, we've found the element
                GridElement& e = elements[currentBucket.index];
                if (x == e.x && y == e.y && z == e.z)
                {
                    // Replace the element to be deleted by the last element in
                    // the vector, then delete the last element in the vector
                    e = elements[elements.size() - 1];
                    elements.pop_back();

                    // Lookup the displaced item and update their bucket's index
                    uint64_t displacedHash = Hash(e.x, e.y, e.z);
                    uint32_t checkIndex = displacedHash & (buckets.size() - 1);
                    uint16_t displacedFingerprint = displacedHash;

                    // Probe for the displaced bucket (it is guaranteed to exist)
                    while (true)
                    {
                        Bucket& checkBucket = buckets[checkIndex];
                        if (checkBucket.index == elements.size())
                        {
                            // Update the displaced element's bucket's index
                            checkBucket.index = currentBucket.index;
                            currentBucket.empty = true;
                            break;
                        }
                        checkIndex = ++checkIndex & (buckets.size() - 1);
                    }

                    // Backward-shift the adjacent buckets to the old location to ensure optimal locations
                    Bucket* previousBucket = &currentBucket;
                    bucketIndex = ++bucketIndex & (buckets.size() - 1);
                    while (true)
                    {
                        // Once we reach an empty bucket or a bucket at it's preferred slot, stop
                        Bucket& shiftBucket = buckets[bucketIndex];
                        if (shiftBucket.empty || shiftBucket.distance == 0) return;

                        // Shift current bucket to previous bucket
                        shiftBucket.distance--;
                        *previousBucket = shiftBucket;
                        shiftBucket.empty = true;

                        // Update pointer and index for next iteration
                        previousBucket = &shiftBucket;
                        bucketIndex = ++bucketIndex & (buckets.size() - 1);
                    }
                }
            }

            // If the current bucket does not trigger an early out or erase, check the next bucket
            bucketIndex = ++bucketIndex & (buckets.size() - 1);
            distance++;
        }
    }

    template <typename T>
    void HashGrid3D<T>::Clear()
    {
        elements.clear();
        buckets.clear();
    }

    template <typename T>
    void HashGrid3D<T>::Rehash()
    {
        // Check if we should resize or not
        uint8_t offset = loadFactor > maxLoad ? 1 : loadFactor < minLoad ? -1 : 0;
        if (offset == 0) return;
        
        // Update bucket size exponent and rehash all elements if it has changed
        uint8_t newExponent = std::clamp(static_cast<uint8_t>(bucketSizeExponent + offset), MIN_BUCKET_EXPONENT, MAX_BUCKET_EXPONENT);

        // TODO: Rehash all elements
    }
}