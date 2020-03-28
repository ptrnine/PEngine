#pragma once

#define GEN_GET_2(A, B) \
    inline auto A##B() const { \
        return DerivedT<T, 2>{this->A(), this->B()}; \
    }

#define GEN_SET_2(A, B) \
    template <typename TT> \
    inline const auto& A##B(const DerivedT<TT, 2>& vec) { \
        this->A(vec.v[0]); this->B(vec.v[1]); \
        return static_cast<DerivedT<T, S>&>(*this);\
    } \
    template <typename T1, typename T2> \
    inline const auto& A##B(T1 a, T2 b) { \
        this->A(a); this->B(b); \
        return static_cast<DerivedT<T, S>&>(*this); \
    }

#define GEN_GET_3(A, B, C) \
    inline auto A##B##C() const { \
        return DerivedT<T, 3>{this->A(), this->B(), this->C()}; \
    }

#define GEN_SET_3(A, B, C) \
    template <typename TT> \
    inline const auto& A##B##C(const DerivedT<TT, 3>& vec) { \
        this->A(vec.v[0]); this->B(vec.v[1]); this->C(vec.v[2]); \
        return static_cast<DerivedT<T, S>&>(*this);\
    } \
    template <typename T1, typename T2, typename T3> \
    inline const auto& A##B##C(T1 a, T2 b, T3 c) { \
        this->A(a); this->B(b); this->C(c); \
        return static_cast<DerivedT<T, S>&>(*this); \
    }

#define GEN_GET_4(A, B, C, D) \
    inline auto A##B##C##D() const { \
        return DerivedT<T, 4>{this->A(), this->B(), this->C(), this->D()}; \
    }

#define GEN_SET_4(A, B, C, D) \
    template <typename TT> \
    inline const auto& A##B##C##D(const DerivedT<TT, 4>& vec) { \
        this->A(vec.v[0]); this->B(vec.v[1]); this->C(vec.v[2]); this->D(vec.v[3]); \
        return static_cast<DerivedT<T, S>&>(*this);\
    } \
    template <typename T1, typename T2, typename T3, typename T4> \
    inline const auto& A##B##C##D(T1 a, T2 b, T3 c, T4 d) { \
        this->A(a); this->B(b); this->C(c); this->D(d); \
        return static_cast<DerivedT<T, S>&>(*this); \
    }

#define GEN_GET_2_PERM(A, B) \
    GEN_GET_2(A, B) \
    GEN_GET_2(B, A)

#define GEN_GET_2_FROM_VEC2(A, B) \
    GEN_GET_2_PERM(A, B) \
    GEN_GET_2(A, A) \
    GEN_GET_2(B, B)

#define GEN_GET_2_FROM_VEC3(A, B, C) \
    GEN_GET_2_PERM(A, C) \
    GEN_GET_2_PERM(B, C) \
    GEN_GET_2(C, C)

#define GEN_GET_2_FROM_VEC4(A, B, C, D) \
    GEN_GET_2_PERM(A, D) \
    GEN_GET_2_PERM(B, D) \
    GEN_GET_2_PERM(C, D) \
    GEN_GET_2(D, D)

#define GEN_GET_3_PERM(A, B, C) \
    GEN_GET_3(A, B, C) \
    GEN_GET_3(B, C, A) \
    GEN_GET_3(C, A, B)

#define GEN_GET_3_FROM_VEC3(A, B, C) \
    GEN_GET_3_PERM(A, B, C) \
    GEN_GET_3_PERM(A, C, B) \
    GEN_GET_3_PERM(A, A, B) \
    GEN_GET_3_PERM(A, A, C) \
    GEN_GET_3_PERM(B, B, A) \
    GEN_GET_3_PERM(B, B, C) \
    GEN_GET_3_PERM(C, C, A) \
    GEN_GET_3_PERM(C, C, B) \
    GEN_GET_3(A, A, A) \
    GEN_GET_3(B, B, B) \
    GEN_GET_3(C, C, C)

#define GEN_GET_3_FROM_VEC4(A, B, C, D) \
    GEN_GET_3_PERM(A, B, D) \
    GEN_GET_3_PERM(A, D, B) \
    GEN_GET_3_PERM(A, C, D) \
    GEN_GET_3_PERM(A, D, C) \
    GEN_GET_3_PERM(B, C, D) \
    GEN_GET_3_PERM(B, D, C) \
    GEN_GET_3_PERM(A, A, D) \
    GEN_GET_3_PERM(B, B, D) \
    GEN_GET_3_PERM(C, C, D) \
    GEN_GET_3_PERM(D, D, A) \
    GEN_GET_3_PERM(D, D, B) \
    GEN_GET_3_PERM(D, D, C) \
    GEN_GET_3(D, D, D)

#define GEN_GET_4_FROM_VEC4(A, B, C, D) \
    GEN_GET_4(A, A, A, A) \
    GEN_GET_4(A, A, A, B) \
    GEN_GET_4(A, A, A, C) \
    GEN_GET_4(A, A, A, D) \
    GEN_GET_4(A, A, B, A) \
    GEN_GET_4(A, A, B, B) \
    GEN_GET_4(A, A, B, C) \
    GEN_GET_4(A, A, B, D) \
    GEN_GET_4(A, A, C, A) \
    GEN_GET_4(A, A, C, B) \
    GEN_GET_4(A, A, C, C) \
    GEN_GET_4(A, A, C, D) \
    GEN_GET_4(A, A, D, A) \
    GEN_GET_4(A, A, D, B) \
    GEN_GET_4(A, A, D, C) \
    GEN_GET_4(A, A, D, D) \
    GEN_GET_4(A, B, A, A) \
    GEN_GET_4(A, B, A, B) \
    GEN_GET_4(A, B, A, C) \
    GEN_GET_4(A, B, A, D) \
    GEN_GET_4(A, B, B, A) \
    GEN_GET_4(A, B, B, B) \
    GEN_GET_4(A, B, B, C) \
    GEN_GET_4(A, B, B, D) \
    GEN_GET_4(A, B, C, A) \
    GEN_GET_4(A, B, C, B) \
    GEN_GET_4(A, B, C, C) \
    GEN_GET_4(A, B, C, D) \
    GEN_GET_4(A, B, D, A) \
    GEN_GET_4(A, B, D, B) \
    GEN_GET_4(A, B, D, C) \
    GEN_GET_4(A, B, D, D) \
    GEN_GET_4(A, C, A, A) \
    GEN_GET_4(A, C, A, B) \
    GEN_GET_4(A, C, A, C) \
    GEN_GET_4(A, C, A, D) \
    GEN_GET_4(A, C, B, A) \
    GEN_GET_4(A, C, B, B) \
    GEN_GET_4(A, C, B, C) \
    GEN_GET_4(A, C, B, D) \
    GEN_GET_4(A, C, C, A) \
    GEN_GET_4(A, C, C, B) \
    GEN_GET_4(A, C, C, C) \
    GEN_GET_4(A, C, C, D) \
    GEN_GET_4(A, C, D, A) \
    GEN_GET_4(A, C, D, B) \
    GEN_GET_4(A, C, D, C) \
    GEN_GET_4(A, C, D, D) \
    GEN_GET_4(A, D, A, A) \
    GEN_GET_4(A, D, A, B) \
    GEN_GET_4(A, D, A, C) \
    GEN_GET_4(A, D, A, D) \
    GEN_GET_4(A, D, B, A) \
    GEN_GET_4(A, D, B, B) \
    GEN_GET_4(A, D, B, C) \
    GEN_GET_4(A, D, B, D) \
    GEN_GET_4(A, D, C, A) \
    GEN_GET_4(A, D, C, B) \
    GEN_GET_4(A, D, C, C) \
    GEN_GET_4(A, D, C, D) \
    GEN_GET_4(A, D, D, A) \
    GEN_GET_4(A, D, D, B) \
    GEN_GET_4(A, D, D, C) \
    GEN_GET_4(A, D, D, D) \
    GEN_GET_4(B, A, A, A) \
    GEN_GET_4(B, A, A, B) \
    GEN_GET_4(B, A, A, C) \
    GEN_GET_4(B, A, A, D) \
    GEN_GET_4(B, A, B, A) \
    GEN_GET_4(B, A, B, B) \
    GEN_GET_4(B, A, B, C) \
    GEN_GET_4(B, A, B, D) \
    GEN_GET_4(B, A, C, A) \
    GEN_GET_4(B, A, C, B) \
    GEN_GET_4(B, A, C, C) \
    GEN_GET_4(B, A, C, D) \
    GEN_GET_4(B, A, D, A) \
    GEN_GET_4(B, A, D, B) \
    GEN_GET_4(B, A, D, C) \
    GEN_GET_4(B, A, D, D) \
    GEN_GET_4(B, B, A, A) \
    GEN_GET_4(B, B, A, B) \
    GEN_GET_4(B, B, A, C) \
    GEN_GET_4(B, B, A, D) \
    GEN_GET_4(B, B, B, A) \
    GEN_GET_4(B, B, B, B) \
    GEN_GET_4(B, B, B, C) \
    GEN_GET_4(B, B, B, D) \
    GEN_GET_4(B, B, C, A) \
    GEN_GET_4(B, B, C, B) \
    GEN_GET_4(B, B, C, C) \
    GEN_GET_4(B, B, C, D) \
    GEN_GET_4(B, B, D, A) \
    GEN_GET_4(B, B, D, B) \
    GEN_GET_4(B, B, D, C) \
    GEN_GET_4(B, B, D, D) \
    GEN_GET_4(B, C, A, A) \
    GEN_GET_4(B, C, A, B) \
    GEN_GET_4(B, C, A, C) \
    GEN_GET_4(B, C, A, D) \
    GEN_GET_4(B, C, B, A) \
    GEN_GET_4(B, C, B, B) \
    GEN_GET_4(B, C, B, C) \
    GEN_GET_4(B, C, B, D) \
    GEN_GET_4(B, C, C, A) \
    GEN_GET_4(B, C, C, B) \
    GEN_GET_4(B, C, C, C) \
    GEN_GET_4(B, C, C, D) \
    GEN_GET_4(B, C, D, A) \
    GEN_GET_4(B, C, D, B) \
    GEN_GET_4(B, C, D, C) \
    GEN_GET_4(B, C, D, D) \
    GEN_GET_4(B, D, A, A) \
    GEN_GET_4(B, D, A, B) \
    GEN_GET_4(B, D, A, C) \
    GEN_GET_4(B, D, A, D) \
    GEN_GET_4(B, D, B, A) \
    GEN_GET_4(B, D, B, B) \
    GEN_GET_4(B, D, B, C) \
    GEN_GET_4(B, D, B, D) \
    GEN_GET_4(B, D, C, A) \
    GEN_GET_4(B, D, C, B) \
    GEN_GET_4(B, D, C, C) \
    GEN_GET_4(B, D, C, D) \
    GEN_GET_4(B, D, D, A) \
    GEN_GET_4(B, D, D, B) \
    GEN_GET_4(B, D, D, C) \
    GEN_GET_4(B, D, D, D) \
    GEN_GET_4(C, A, A, A) \
    GEN_GET_4(C, A, A, B) \
    GEN_GET_4(C, A, A, C) \
    GEN_GET_4(C, A, A, D) \
    GEN_GET_4(C, A, B, A) \
    GEN_GET_4(C, A, B, B) \
    GEN_GET_4(C, A, B, C) \
    GEN_GET_4(C, A, B, D) \
    GEN_GET_4(C, A, C, A) \
    GEN_GET_4(C, A, C, B) \
    GEN_GET_4(C, A, C, C) \
    GEN_GET_4(C, A, C, D) \
    GEN_GET_4(C, A, D, A) \
    GEN_GET_4(C, A, D, B) \
    GEN_GET_4(C, A, D, C) \
    GEN_GET_4(C, A, D, D) \
    GEN_GET_4(C, B, A, A) \
    GEN_GET_4(C, B, A, B) \
    GEN_GET_4(C, B, A, C) \
    GEN_GET_4(C, B, A, D) \
    GEN_GET_4(C, B, B, A) \
    GEN_GET_4(C, B, B, B) \
    GEN_GET_4(C, B, B, C) \
    GEN_GET_4(C, B, B, D) \
    GEN_GET_4(C, B, C, A) \
    GEN_GET_4(C, B, C, B) \
    GEN_GET_4(C, B, C, C) \
    GEN_GET_4(C, B, C, D) \
    GEN_GET_4(C, B, D, A) \
    GEN_GET_4(C, B, D, B) \
    GEN_GET_4(C, B, D, C) \
    GEN_GET_4(C, B, D, D) \
    GEN_GET_4(C, C, A, A) \
    GEN_GET_4(C, C, A, B) \
    GEN_GET_4(C, C, A, C) \
    GEN_GET_4(C, C, A, D) \
    GEN_GET_4(C, C, B, A) \
    GEN_GET_4(C, C, B, B) \
    GEN_GET_4(C, C, B, C) \
    GEN_GET_4(C, C, B, D) \
    GEN_GET_4(C, C, C, A) \
    GEN_GET_4(C, C, C, B) \
    GEN_GET_4(C, C, C, C) \
    GEN_GET_4(C, C, C, D) \
    GEN_GET_4(C, C, D, A) \
    GEN_GET_4(C, C, D, B) \
    GEN_GET_4(C, C, D, C) \
    GEN_GET_4(C, C, D, D) \
    GEN_GET_4(C, D, A, A) \
    GEN_GET_4(C, D, A, B) \
    GEN_GET_4(C, D, A, C) \
    GEN_GET_4(C, D, A, D) \
    GEN_GET_4(C, D, B, A) \
    GEN_GET_4(C, D, B, B) \
    GEN_GET_4(C, D, B, C) \
    GEN_GET_4(C, D, B, D) \
    GEN_GET_4(C, D, C, A) \
    GEN_GET_4(C, D, C, B) \
    GEN_GET_4(C, D, C, C) \
    GEN_GET_4(C, D, C, D) \
    GEN_GET_4(C, D, D, A) \
    GEN_GET_4(C, D, D, B) \
    GEN_GET_4(C, D, D, C) \
    GEN_GET_4(C, D, D, D) \
    GEN_GET_4(D, A, A, A) \
    GEN_GET_4(D, A, A, B) \
    GEN_GET_4(D, A, A, C) \
    GEN_GET_4(D, A, A, D) \
    GEN_GET_4(D, A, B, A) \
    GEN_GET_4(D, A, B, B) \
    GEN_GET_4(D, A, B, C) \
    GEN_GET_4(D, A, B, D) \
    GEN_GET_4(D, A, C, A) \
    GEN_GET_4(D, A, C, B) \
    GEN_GET_4(D, A, C, C) \
    GEN_GET_4(D, A, C, D) \
    GEN_GET_4(D, A, D, A) \
    GEN_GET_4(D, A, D, B) \
    GEN_GET_4(D, A, D, C) \
    GEN_GET_4(D, A, D, D) \
    GEN_GET_4(D, B, A, A) \
    GEN_GET_4(D, B, A, B) \
    GEN_GET_4(D, B, A, C) \
    GEN_GET_4(D, B, A, D) \
    GEN_GET_4(D, B, B, A) \
    GEN_GET_4(D, B, B, B) \
    GEN_GET_4(D, B, B, C) \
    GEN_GET_4(D, B, B, D) \
    GEN_GET_4(D, B, C, A) \
    GEN_GET_4(D, B, C, B) \
    GEN_GET_4(D, B, C, C) \
    GEN_GET_4(D, B, C, D) \
    GEN_GET_4(D, B, D, A) \
    GEN_GET_4(D, B, D, B) \
    GEN_GET_4(D, B, D, C) \
    GEN_GET_4(D, B, D, D) \
    GEN_GET_4(D, C, A, A) \
    GEN_GET_4(D, C, A, B) \
    GEN_GET_4(D, C, A, C) \
    GEN_GET_4(D, C, A, D) \
    GEN_GET_4(D, C, B, A) \
    GEN_GET_4(D, C, B, B) \
    GEN_GET_4(D, C, B, C) \
    GEN_GET_4(D, C, B, D) \
    GEN_GET_4(D, C, C, A) \
    GEN_GET_4(D, C, C, B) \
    GEN_GET_4(D, C, C, C) \
    GEN_GET_4(D, C, C, D) \
    GEN_GET_4(D, C, D, A) \
    GEN_GET_4(D, C, D, B) \
    GEN_GET_4(D, C, D, C) \
    GEN_GET_4(D, C, D, D) \
    GEN_GET_4(D, D, A, A) \
    GEN_GET_4(D, D, A, B) \
    GEN_GET_4(D, D, A, C) \
    GEN_GET_4(D, D, A, D) \
    GEN_GET_4(D, D, B, A) \
    GEN_GET_4(D, D, B, B) \
    GEN_GET_4(D, D, B, C) \
    GEN_GET_4(D, D, B, D) \
    GEN_GET_4(D, D, C, A) \
    GEN_GET_4(D, D, C, B) \
    GEN_GET_4(D, D, C, C) \
    GEN_GET_4(D, D, C, D) \
    GEN_GET_4(D, D, D, A) \
    GEN_GET_4(D, D, D, B) \
    GEN_GET_4(D, D, D, C) \
    GEN_GET_4(D, D, D, D)


#define GEN_SET_2_PERM(A, B) \
    GEN_SET_2(A, B) \
    GEN_SET_2(B, A)

#define GEN_SET_2_FROM_VEC2(A, B) \
    GEN_SET_2_PERM(A, B)

#define GEN_SET_2_FROM_VEC3(A, B, C) \
    GEN_SET_2_PERM(A, C) \
    GEN_SET_2_PERM(B, C)

#define GEN_SET_2_FROM_VEC4(A, B, C, D) \
    GEN_SET_2_PERM(A, D) \
    GEN_SET_2_PERM(B, D) \
    GEN_SET_2_PERM(C, D)

#define GEN_SET_3_PERM(A, B, C) \
    GEN_SET_3(A, B, C) \
    GEN_SET_3(B, C, A) \
    GEN_SET_3(C, A, B)

#define GEN_SET_3_FROM_VEC3(A, B, C) \
    GEN_SET_3_PERM(A, B, C) \
    GEN_SET_3_PERM(A, C, B) \

#define GEN_SET_3_FROM_VEC4(A, B, C, D) \
    GEN_SET_3_PERM(A, B, D) \
    GEN_SET_3_PERM(A, D, B) \
    GEN_SET_3_PERM(A, C, D) \
    GEN_SET_3_PERM(A, D, C) \
    GEN_SET_3_PERM(B, C, D) \
    GEN_SET_3_PERM(B, D, C) \

#define GEN_SET_4_FROM_VEC4(A, B, C, D) \
    GEN_SET_4(A, B, C, D) \
    GEN_SET_4(A, B, D, C) \
    GEN_SET_4(A, C, B, D) \
    GEN_SET_4(A, C, D, B) \
    GEN_SET_4(A, D, B, C) \
    GEN_SET_4(A, D, C, B) \
    GEN_SET_4(B, A, C, D) \
    GEN_SET_4(B, A, D, C) \
    GEN_SET_4(B, C, A, D) \
    GEN_SET_4(B, C, D, A) \
    GEN_SET_4(B, D, A, C) \
    GEN_SET_4(B, D, C, A) \
    GEN_SET_4(C, A, B, D) \
    GEN_SET_4(C, A, D, B) \
    GEN_SET_4(C, B, A, D) \
    GEN_SET_4(C, B, D, A) \
    GEN_SET_4(C, D, A, B) \
    GEN_SET_4(C, D, B, A) \
    GEN_SET_4(D, A, B, C) \
    GEN_SET_4(D, A, C, B) \
    GEN_SET_4(D, B, A, C) \
    GEN_SET_4(D, B, C, A) \
    GEN_SET_4(D, C, A, B) \
    GEN_SET_4(D, C, B, A)
