#pragma once

#include <webgpu-raytracer/gltf_asset.hpp>

namespace glTF
{

    template <typename T>
    struct AccessorIterator
    {
        AccessorIterator(Asset const & asset, Accessor const & accessor, std::uint32_t index)
        {
            auto const & bufferView = asset.bufferViews[accessor.bufferView];
            auto const & buffer = asset.buffers[bufferView.buffer];

            ptr_ = buffer.data.data() + bufferView.byteOffset + accessor.byteOffset;

            stride_ = bufferView.byteStride.value_or(sizeof(T));

            ptr_ += stride_ * index;
        }

        T const & operator * () const
        {
            return *reinterpret_cast<T const *>(ptr_);
        }

        AccessorIterator & operator ++ ()
        {
            ptr_ += stride_;
            return *this;
        }

        AccessorIterator operator ++ (int)
        {
            auto copy = *this;
            operator++();
            return copy;
        }

        friend bool operator == (AccessorIterator const & it1, AccessorIterator const & it2)
        {
            return it1.ptr_ == it2.ptr_;
        }

    private:
        char const * ptr_;
        std::uint32_t stride_;
    };

    template <typename T>
    struct AccessorRange
    {
        AccessorRange(Asset const & asset, Accessor const & accessor)
            : begin_(asset, accessor, 0)
            , end_(asset, accessor, accessor.count)
        {}

        auto begin() const { return begin_; }
        auto end() const { return end_; }

    private:
        AccessorIterator<T> begin_;
        AccessorIterator<T> end_;
    };

}
