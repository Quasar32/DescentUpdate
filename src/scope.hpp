#ifndef SCOPE_HPP
#define SCOPE_HPP

template<typename type> 
class scope {
private:
    type ToCall;

public:
    scope(type ToCall) : ToCall(ToCall) {};

    ~scope() {
        ToCall();
    }
};

template<typename type>
inline auto ScopeCreate(type ToCall) {
    return scope(ToCall);
}
#endif
