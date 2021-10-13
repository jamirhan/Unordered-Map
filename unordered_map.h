#include <iostream>
#include <vector>
#include <cmath>
#include <limits>
#include <list>

// TODO:
// 1) change allocator when copying
//

namespace tools {
    template<bool B, typename T, typename F>
    struct cond_t {
        typedef F type;
    };

    template<typename T, typename F>
    struct cond_t<true, T, F> {
        typedef T type;
    };
}


template <typename T, typename Alloc=std::allocator<T>>
class List {

    struct Node{
        T* content = nullptr;
        Node* next = nullptr;
        Node* prev = nullptr;
    };


    using AllocTraitT = std::allocator_traits<Alloc>;
    using NodeAlloc = typename AllocTraitT::template rebind_alloc<Node>;
    using AllocTraitNode = std::allocator_traits<NodeAlloc>;

    Node* first_el = nullptr;
    Node* last_el = nullptr;
    size_t els = 0;

    NodeAlloc node_allocator;
    Alloc allocator_T;

    void copy(const List& another, bool copy_alloc_node, bool copy_T_alloc) {
        if (copy_alloc_node) {
            node_allocator = another.node_allocator;
        }
        if (copy_T_alloc) {
            allocator_T = another.allocator_T;
        }
        Node* cur_el = another.first_el;
        while (cur_el->next) {
            push_back(*(cur_el->content));
            cur_el = cur_el->next;
        }

    }

    void initialize() {
        Node* fake_node = AllocTraitNode::allocate(node_allocator, 1);
        AllocTraitNode::construct(node_allocator, fake_node);
        first_el = fake_node;
        last_el = fake_node;
    }

    template <typename... Args>
    Node* create_node(Args&&... args) {
        Node* new_node = AllocTraitNode::allocate(node_allocator, 1);
        AllocTraitNode ::construct(node_allocator, new_node);
        new_node->content = AllocTraitT::allocate(allocator_T, 1);
        AllocTraitT::construct(allocator_T, new_node->content, std::forward<Args>(args)...);
        return new_node;
    }

    void delete_node(Node* ptr) {
        AllocTraitT::destroy(allocator_T, ptr->content);
        AllocTraitT::deallocate(allocator_T, ptr->content, 1);
        AllocTraitNode::destroy(node_allocator, ptr);
        AllocTraitNode::deallocate(node_allocator, ptr, 1);
    }

    void delete_red() {
        AllocTraitNode::deallocate(node_allocator, first_el, 1);
    }

public:

    explicit List(size_t count, const Alloc& allocator = Alloc()):
            node_allocator(allocator), allocator_T(allocator) {
        initialize();
        for (;count;--count)
            emplace_back();
    }

    List(List&& another)  noexcept {
        first_el = another.first_el;
        last_el = another.last_el;
        els = another.els;
        another.first_el = nullptr;
        another.last_el = nullptr;
        another.els = 0;
    }

    List& operator=(List&& another)   noexcept {
        if (first_el != another.first_el)
            clear();
        delete_red();
        first_el = another.first_el;
        last_el = another.last_el;
        els = another.els;
        another.first_el = nullptr;
        another.last_el = nullptr;
        another.els = 0;
        return *this;
    }

    explicit List(size_t count, const T& value, const Alloc& allocator = Alloc()):
            node_allocator(allocator), allocator_T(allocator) {
        initialize();
        for (size_t i = 0; i < count; ++i) {
            push_back(value);
        }
    }

    explicit List(const Alloc& alloc = Alloc()): node_allocator(alloc), allocator_T(alloc) {
        initialize();
    }

    List(const List<T, Alloc>& another):
            node_allocator(AllocTraitNode::select_on_container_copy_construction(another.node_allocator)),
            allocator_T(AllocTraitT::select_on_container_copy_construction(another.allocator_T)) {
        initialize();
        copy(another, false, false);
    }

    List& operator=(const List<T, Alloc>& another) {
        if (this == &another)
            return *this;
        clear();
        copy(another, AllocTraitNode ::propagate_on_container_copy_assignment::value,
             AllocTraitT::propagate_on_container_copy_assignment::value);
        return *this;
    }

    void clear() {
        while (els > 0)
            pop_back();
    }

    size_t size() const noexcept{
        return els;
    }

    void push_back(const T& el) {
        insert(end(), el);
    }

    void push_back(T&& el) {
        insert(end(), std::move(el));
    }

    void push_front(const T& el) {
        insert(begin(), el);
    }

    void pop_back() {
        erase(end() - 1);
    }

    void pop_front() {
        erase(begin());
    }

    template <bool IsConst>
    class common_iterator {
    public:

        using difference_type = int;
        using value_type = T;
        using pointer = typename tools::cond_t<IsConst, const T*, T*>::type;
        using reference = typename tools::cond_t<IsConst, const T&, T&>::type;
        using iterator_category = std::bidirectional_iterator_tag;

    private:
        friend List;

        Node* cur_el = nullptr;

        explicit common_iterator(Node* ptr): cur_el(ptr) {};

    public:

        common_iterator() = default;

        common_iterator(const common_iterator& a): cur_el(a.cur_el) { };

        common_iterator& operator++() {
            cur_el = cur_el->next;
            return *this;
        }

        common_iterator operator++(int) {
            common_iterator copy = *this;
            ++(*this);
            return copy;
        }

        common_iterator& operator +=(int val) {
            for (int i = 0; i < val; ++i)
                ++(*this);
            return *this;
        }

        common_iterator& operator-=(int val) {
            for (int i = 0; i < val; ++i) {
                --(*this);
            }
            return *this;
        }

        common_iterator operator +(int val) {
            common_iterator copy = *this;
            copy += val;
            return copy;
        }

        common_iterator operator -(int val) {
            common_iterator copy = *this;
            copy -= val;
            return copy;
        }

        common_iterator& operator--() {
            cur_el = cur_el->prev;
            return *this;
        }

        common_iterator operator--(int) {
            common_iterator copy = *this;
            --(*this);
            return copy;
        }

        operator common_iterator<true>() const {
            return common_iterator<true>(cur_el);
        }

        bool operator<=(const common_iterator& op) const {
            return cur_el <= op.cur_el;
        }

        bool operator>=(const common_iterator& op) const {
            return cur_el >= op.cur_el;
        }

        bool operator<(const common_iterator& op) const {
            return cur_el < op.cur_el;
        }

        bool operator>(const common_iterator& op) const {
            return cur_el > op.cur_el;
        }

        bool operator==(const common_iterator& op) const {
            return (cur_el == op.cur_el);
        }

        bool operator!=(const common_iterator& op) const {
            return (cur_el != op.cur_el);
        }

        reference operator*() {
            return *(cur_el->content);
        }

        pointer operator->() {
            return cur_el->content;
        }

        const T& operator*() const {
            return *(cur_el->content);
        }

        const T* operator->() const {
            return cur_el->content;
        }
    };

    using iterator = common_iterator<false>;
    using const_iterator = common_iterator<true>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    iterator begin() {
        return iterator(first_el);
    }

    iterator end() {
        return iterator(last_el);
    }

    const_iterator cbegin() const {
        return const_iterator(first_el);
    }

    const_iterator cend() const {
        return const_iterator(last_el);
    }

    const_iterator begin() const {
        return cbegin();
    }

    const_iterator end() const {
        return cend();
    }

    const_reverse_iterator crbegin() const {
        return const_reverse_iterator(cend());
    }

    const_reverse_iterator rbegin() const {
        return const_reverse_iterator(crbegin());
    }

    const_reverse_iterator crend() const {
        return const_reverse_iterator(cbegin());
    }

    const_reverse_iterator rend() const {
        return const_reverse_iterator(crend());
    }

    reverse_iterator rbegin() {
        return reverse_iterator(end());
    }

    reverse_iterator rend() {
        return reverse_iterator(begin());
    }

    Alloc get_allocator() {
        return allocator_T;
    }

    ~List() {
        clear();
        if (first_el)
            delete_red();
    }

    template<typename... Args>
    void emplace_back(Args&&... args) {
        insert(end(), std::forward<Args>(args)...);
    }

    template<typename... Args>
    void insert(const_iterator it, Args&&... args) {

//        T* new_el = AllocTraitT::allocate(T_allocator, 1);
//        AllocTraitT::construct(T_allocator, new_el, std::forward<Args>(args)...);
        // don't forget deallocate
//        EmptyNode* prev_node = it.cur_el->prev;
//        EmptyNode* new_node = create_node(std::move(*new_el));
        Node* prev_node = it.cur_el->prev;
        Node* new_node = create_node(std::forward<Args>(args)...);
        new_node->prev = prev_node;
        new_node->next = it.cur_el;

        it.cur_el->prev = new_node;
        if (prev_node)
            prev_node->next = new_node;
        else
            first_el = new_node;
        ++els;
    }

    void erase(const_iterator it) {
        Node* node_ptr = it.cur_el;
        Node* prev = node_ptr->prev;
        Node* next = node_ptr->next;
        if (prev)
            prev->next = next;
        else
            first_el = next;
        next->prev = prev;
        delete_node(node_ptr);
        --els;
    }

};


template <class Key, class Value, class Hash = std::hash<Key>, class Equal = std::equal_to<Key>,
                                    class Alloc = std::allocator<std::pair<const Key, Value>>>
class UnorderedMap {
public:
    using size_type = int;
    using NodeType = std::pair<const Key, Value>;
private:
    using list = List<NodeType, Alloc>;
    using IterAlloc = typename std::allocator_traits<Alloc>::template rebind_alloc<typename List<NodeType, Alloc>::iterator>;
    using vector_type = std::vector<List<typename list::iterator, IterAlloc>,
            typename std::allocator_traits<Alloc>::template rebind_alloc<List<typename list::iterator, IterAlloc>>>;
    double max_load_factor_v = 1.5;
    int expand_coefficient = 2;
    list elements;
    vector_type hash_table;
    using NodeAlloc = typename std::allocator_traits<Alloc>::template rebind_alloc<NodeType>;
    NodeAlloc alloc;
public:
    using iterator = typename list::iterator;
    using const_iterator = typename list::const_iterator;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

private:

    size_type get_index(const Key& key) const{
        Hash hash;
        return hash(key) % buckets();
    }


    void set_to_hash(typename list::iterator iter) {
        size_type index = get_index((*iter).first);
        hash_table[index].push_back(iter);
    }

    auto find_iter(const Key& key) {
        size_type index = get_index(key);
        Equal eq;
        for (auto it = hash_table[index].begin(); it != hash_table[index].end(); ++it) {
            if (eq((*(*it)).first, key)) {
                return it;
            }
        }
        return hash_table[index].end();
    }

    const_iterator find_eq(size_type index, const Key& key) const {
        auto it = hash_table[index].begin();
        Equal eq;
        for (;it != hash_table[index].end(); ++it) {
            if (eq((*(*it)).first, key)) {
                return *it;
            }
        }
        return end();
    }

    iterator find_eq(size_type index, const Key& key)  {
        auto it = hash_table[index].begin();
        Equal eq;
        for (;it != hash_table[index].end(); ++it) {
            if (eq((*(*it)).first, key)) {
                return *it;
            }
        }
        return end();
    }

public:

    UnorderedMap() {
        hash_table.resize(1);
    }
    UnorderedMap(const UnorderedMap& another) {
        elements = another.elements;
        max_load_factor_v = another.max_load_factor_v;
        hash_table.resize(another.hash_table.size());
        for (auto it = elements.begin(); it != elements.end(); ++it) {
            set_to_hash(it);
        }
    }
    UnorderedMap(UnorderedMap&&)  noexcept = default;

    UnorderedMap& operator=(const UnorderedMap& another) {
        max_load_factor_v = another.max_load_factor_v;
        elements = another.elements;
        hash_table = another.hash_table;
        return *this;
    }

    UnorderedMap& operator=(UnorderedMap&& another)  noexcept {
        max_load_factor_v = std::move(another.max_load_factor_v);
        hash_table.clear();
        hash_table = std::move(another.hash_table);
//        hash_table[0] = std::move(another.hash_table[0]);
//        hash_table[1] = std::move(another.hash_table[1]);
//        hash_table[2] = std::move(another.hash_table[2]);
        elements = std::move(another.elements);
        return *this;
    }

    void rehash(size_type n) {
        vector_type old = std::move(hash_table);
        hash_table = vector_type(n);
        for (auto it = elements.begin(); it != elements.end(); ++it) {
            set_to_hash(it);
        }
    }

    iterator find(const Key& key) {
        size_type index = get_index(key);
        return find_eq(index, key);
    }

    const_iterator find(const Key& key) const {
        size_type index = get_index(key);
        return find_eq(index, key);
    }

    std::pair<iterator, bool> insert(const NodeType& element) {
        iterator it = find(element.first);
        if (it != end())
            return {it, false};
        if (load_factor() > max_load_factor_v) {
            rehash(size() * expand_coefficient);
        }
        elements.push_back(element);
        set_to_hash(elements.end() - 1);
        return {elements.end() - 1, true};
    }

    std::pair<iterator, bool> insert(NodeType&& element) {
        iterator it = find(element.first);
        if (it != end())
            return {it, false};
        if (load_factor() > max_load_factor_v) {
            rehash(size() * expand_coefficient);
        }
        elements.push_back(std::move(element));
        set_to_hash(elements.end() - 1);
        return {elements.end() - 1, true};
    }

    template <typename P>
    std::pair<iterator, bool> insert(P&& element) {

        iterator it = find(element.first);
        if (it != end())
            return {it, false};
        if (load_factor() > max_load_factor_v) {
            rehash(size() * expand_coefficient);
        }
        elements.template emplace_back(std::forward<P>(element));

        set_to_hash(elements.end() - 1);
        return {elements.end() - 1, true};
    }

    template <class InputIterator>
    void insert(InputIterator it1, InputIterator it2) {
        for (;it1 != it2; ++it1) {
            insert(*it1);
        }
    }

    void reserve(size_type n) {
        rehash(std::ceil(n / max_load_factor_v));
    }

    size_type max_size() const noexcept {
        return std::numeric_limits<size_type>::max();
    }

    double load_factor() const {
        double s = size();
        double b = buckets();
        return s / b;
    }

    template<class... Args>
    std::pair<iterator, bool> emplace(Args&&... args) {
        if (load_factor() > max_load_factor()) {
            rehash(size() * expand_coefficient);
        }
        elements.emplace_back(std::forward<Args>(args)...);
        const Key& key = (elements.end() - 1)->first;
        iterator it = find(key);
        if (it != end()) {
            elements.erase(elements.end() - 1);
            return {it, false};
        }
        set_to_hash(elements.end() - 1);
        return {elements.end() - 1, true};
//        NodeType* new_node = std::allocator_traits<NodeAlloc>::allocate(alloc, 1);
//        std::allocator_traits<NodeAlloc>::construct(alloc, new_node, std::forward<Args>(args)...);
//        return insert(std::move(*new_node));
    }

    iterator erase(const_iterator pos) {
        iterator next = find(pos->first) + 1;
        auto el = find_iter(pos->first);
        hash_table[get_index(pos->first)].erase(el);
        elements.erase(pos);
        return next;
    }

    iterator erase(const_iterator it1, const_iterator it2) {
        for (;it1 != it2; it1 = erase(it1)) {
        }
        if (it2 == cend())
            return end();
        else
            return find(it2->first);
    }

    Value& operator[](const Key& key) {
        return insert(std::move(std::make_pair(key, Value()))).first->second;
    }

    Value& operator[](Key&& key) {
        return insert(std::move(std::make_pair(std::move(key), Value()))).first->second;
    }

    Value& at(const Key& key) const {
        auto it = find(key);
        if (it == end()) {
            throw std::out_of_range("Retard");
        }
        return it->second;
    }

    Value& at(const Key& key) {
        auto it = find(key);
        if (it == end()) {
            throw std::out_of_range("Retard");
        }
        else {
            return it->second;
        }
    }

    double max_load_factor() const {
        return max_load_factor_v;
    }

    size_type size() const noexcept {
        return elements.size();
    }

    size_type buckets() const noexcept {
        return hash_table.size();
    }

    iterator begin() noexcept {
        return elements.begin();
    }

    iterator end() noexcept{
        return elements.end();
    }

    const_iterator cbegin() const noexcept{
        return elements.cbegin();
    }

    const_iterator cend() const noexcept{
        return elements.cend();
    }

    const_iterator begin() const noexcept{
        return cbegin();
    }

    const_iterator end() const noexcept {
        return cend();
    }

    const_reverse_iterator crbegin() const noexcept{
        return elements.crbegin();
    }

    const_reverse_iterator rbegin() const noexcept {
        return elements.rbegin();
    }

    const_reverse_iterator crend() const noexcept {
        return elements.crend();
    }

    const_reverse_iterator rend() const noexcept {
        return elements.rend();
    }

    reverse_iterator rbegin() noexcept {
        return elements.rbegin();
    }

    reverse_iterator rend() noexcept {
        return elements.rend();
    }

    ~UnorderedMap() = default;

};
