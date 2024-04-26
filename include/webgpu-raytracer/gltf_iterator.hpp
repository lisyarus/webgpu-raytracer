#pragma once

#include <cstdint>
#include <optional>

namespace glTF
{

    template <typename T>
    struct AccessorIterator
    {
        AccessorIterator(char const * ptr, std::optional<std::uint32_t> stride)
            : ptr_(ptr)
            , stride_(stride.value_or(sizeof(T)))
        {}

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

    private:
        char const * ptr_;
        std::uint32_t stride_;
    };

}
