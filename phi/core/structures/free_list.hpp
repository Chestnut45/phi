#pragma once

#include <vector>

namespace Phi
{
    // Represents an indexed free list with constant-time
    // random removals without invalidating indices
    // T must be trivially constructible and destructible
    template <typename T>
    class FreeList
    {
        public:

            FreeList();
            ~FreeList();

            // Delete copy constructor/assignment
            FreeList(const FreeList&) = delete;
            FreeList& operator=(const FreeList&) = delete;

            // Delete move constructor/assignment
            FreeList(FreeList&& other) = delete;
            FreeList& operator=(FreeList&& other) = delete;

            // Inserts an element and returns an index to it
            int Insert(const T& element);

            // Removes the nth element
            void Erase(int n);

            // Removes all elements
            void Clear();

            // Returns the size of the internal container
            size_t Size() const;

            // Returns the number of elements in the list
            size_t Count() const;

            // Returns the nth element
            T& operator[](int n);

            // Returns the nth element
            const T& operator[](int n) const;

        private:

            struct FreeElement
            {   
                T element;
                int next = -1;
            };
            
            // Internal container
            std::vector<FreeElement> data;

            // Counter for elements (since data.size() is unrelated)
            size_t count = 0;

            // The lowest free index or -1 if all are occupied
            int firstFree = -1;
    };

    template <typename T>
    FreeList<T>::FreeList()
    {
    }

    template <typename T>
    FreeList<T>::~FreeList()
    {
    }

    template <typename T>
    int FreeList<T>::Insert(const T& element)
    {
        count++;
        if (firstFree != -1)
        {
            // Reclaim earlier index
            const int index = firstFree;
            firstFree = data[firstFree].next;
            data[index].element = element;
            return index;
        }
        else
        {
            // Insert a new element
            FreeElement fe;
            fe.element = element;
            data.push_back(fe);
            return static_cast<int>(data.size() - 1);
        }
    }

    template <typename T>
    void FreeList<T>::Erase(int n)
    {
        data[n].next = firstFree;
        firstFree = n;
        count--;
    }

    template <typename T>
    void FreeList<T>::Clear()
    {
        data.clear();
        firstFree = -1;
        count = 0;
    }

    template <typename T>
    size_t FreeList<T>::Size() const
    {
        return data.size();
    }

    template <typename T>
    size_t FreeList<T>::Count() const
    {
        return count;
    }

    template <typename T>
    T& FreeList<T>::operator[](int n)
    {
        return data[n].element;
    }

    template <typename T>
    const T& FreeList<T>::operator[](int n) const
    {
        return data[n].element;
    }
}