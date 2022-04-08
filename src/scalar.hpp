#ifndef SCALAR_HPP
#define SCALAR_HPP

template<typename TypeA, typename TypeB>
auto Min(TypeA A, TypeB B) {
    return A < B ? A : B;
}

template<typename TypeA, typename TypeB>
auto Max(TypeA A, TypeB B) {
    return A > B ? A : B;
}

template<typename Type>
auto Abs(Type A) {
    return A < 0 ? -A : A; 
}

#endif
