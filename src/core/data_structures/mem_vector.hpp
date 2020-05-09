#include "../types.hpp"

namespace core {

    template <typename T, typename AllocT = std::allocator<T>>
    class mem_vector : public std::vector<T, AllocT> {
    public:
        using std::vector<T, AllocT>::vector;

        void rshift_assign(const T& value) {
            rshift();
            this->front() = value;
        }

        void lshift_assign(const T& value) {
            lshift();
            this->back() = value;
        }

        void rshift() {
            for (size_t i = this->size() - 2; i < this->size(); --i)
                (*this)[i + 1] = std::move((*this)[i]);
            this->front() = T();
        }

        void lshift() {
            for (size_t i = 0; i < this->size() - 1; ++i)
                (*this)[i] = std::move((*this)[i + 1]);
            this->back() = T();
        }

        void rrotate() {
            T back = std::move(this->back());
            for (size_t i = this->size() - 2; i < this->size(); --i)
                (*this)[i + 1] = std::move((*this)[i]);
            this->front() = std::move(back);
        }

        void lrotate() {
            T front = std::move(this->front());
            for (size_t i = 0; i < this->size() - 1; ++i)
                (*this)[i] = std::move((*this)[i + 1]);
            this->back() = std::move(front);
        }

    };

} // namespace core
