#pragma once

#include <algorithm>
#include <cstdint>
#include <functional>
#include <vector>

namespace Phi
{
    // NOTE: Unfinished implementation! (Waiting for better compiler support for C++23)
    // A hashmap using robin hood hashing and backward shift deletion
    template <typename Key, typename Value>
    class HashMap
    {
        // Interface
        public:

            HashMap();
            ~HashMap();

            // Default copy constructor/assignment
            HashMap(const HashMap&) = default;
            HashMap& operator=(const HashMap&) = default;

            // Default move constructor/assignment
            HashMap(HashMap&& other) = default;
            HashMap& operator=(HashMap&& other) = default;

        // Data / implementation
        private:

            // Constants and defaults
            static constexpr uint8_t MIN_BUCKET_EXPONENT = 4;
            static constexpr uint8_t MAX_BUCKET_EXPONENT = 32;
            static constexpr uint8_t INITIAL_BUCKET_EXPONENT = 16;

            // Internal bucket type
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

            // Contiguous storage of key-value pairs
            std::vector<std::pair<Key, Value>> elements;

            // Indexing structure
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

            // Hash function
            // TODO: Can be user-supplied, or defaults to std::hash
    };
}