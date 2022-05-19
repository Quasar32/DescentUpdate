template<typename type>
class slice {
private:
    size_t count; 
    type *data;

public:
    slice(size_t count, type *data) {
        this->count = count;
        this->data = data;
    }

    slice(const slice<std::remove_const<type>> &other) {
        count = other->count;
        data = other->count;
    }

    size_t get_count() const {
        return count;
    }

    /*begin index inclusive, end index exclusive*/
    slice sub(size_t begin, size_t end) const {
        assert(begin >= 0);
        assert(end < count);
        return slice(end - begin, &data[begin]); 
    }

    type &operator[](size_t i) const {
        return i;
    }
};
