#pragma once

/// Unique pointer template class for array pointers
template <class T> class UniqueArray
{
public:
    /// Construct empty.
    UniqueArray() : ptr_(nullptr) { }

    /// Construct from pointer.
    explicit UniqueArray(T* ptr) : ptr_(ptr) { }

    /// Prevent copy construction.
    UniqueArray(const UniqueArray&) = delete;
    /// Prevent assignment.
    UniqueArray& operator=(const UniqueArray&) = delete;

    /// Assign from pointer.
    UniqueArray& operator = (T* ptr)
    {
        Reset(ptr);
        return *this;
    }

    /// Construct empty.
    UniqueArray(std::nullptr_t) { }   // NOLINT(google-explicit-constructor)

    /// Move-construct from UniqueArray.
    UniqueArray(UniqueArray&& up) noexcept :
        ptr_(up.Detach()) {}

    /// Move-assign from UniqueArray.
    UniqueArray& operator =(UniqueArray&& up) noexcept
    {
        Reset(up.Detach());
        return *this;
    }

    /// Point to the object.
    T* operator ->() const
    {
        assert(ptr_);
        return ptr_;
    }

    /// Dereference the object.
    T& operator *() const
    {
        assert(ptr_);
        return *ptr_;
    }

    /// Test for less than with another unique pointer.
    template <class U>
    bool operator <(const UniqueArray<U>& rhs) const { return ptr_ < rhs.ptr_; }

    /// Test for equality with another unique pointer.
    template <class U>
    bool operator ==(const UniqueArray<U>& rhs) const { return ptr_ == rhs.ptr_; }

    /// Test for inequality with another unique pointer.
    template <class U>
    bool operator !=(const UniqueArray<U>& rhs) const { return ptr_ != rhs.ptr_; }

    /// Cast pointer to bool.
    operator bool() const { return !!ptr_; }    // NOLINT(google-explicit-constructor)

    /// Detach pointer from UniqueArray without destroying.
    T* Detach()
    {
        T* ptr = ptr_;
        ptr_ = nullptr;
        return ptr;
    }

    /// Check if the pointer is null.
    bool Null() const { return ptr_ == 0; }

    /// Check if the pointer is not null.
    bool NotNull() const { return ptr_ != 0; }

    /// Return the raw pointer.
    T* Get() const { return ptr_; }

    /// Reset.
    void Reset(T* ptr = nullptr)
    {
        delete[] ptr_;
        ptr_ = ptr;
    }

    /// Return hash value for HashSet & HashMap.
    unsigned ToHash() const { return (unsigned)((size_t)ptr_ / sizeof(T)); }

    /// Destruct.
    ~UniqueArray()
    {
        Reset();
    }

private:
    T* ptr_;

};