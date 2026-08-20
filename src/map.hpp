#ifndef SJTU_MAP_HPP
#define SJTU_MAP_HPP

#include <functional>
#include <cstddef>
#include "utility.hpp"
#include "exceptions.hpp"

namespace sjtu {

template<class Key, class T, class Compare = std::less<Key>>
class map {
public:
    typedef pair<const Key, T> value_type;

private:
    struct Node {
        value_type *data;
        Node *c[2]; // c[0]=left, c[1]=right
        Node *par;
        Node *next_free; // for free list
        int ht;
        Node(value_type *d, Node *p = nullptr) : data(d), par(p), next_free(nullptr), ht(1) { c[0] = c[1] = nullptr; }
    };

    Node *rt;
    Node *free_head; // head of free list for erased nodes
    size_t sz;
    Compare cmp;

    int ht(Node *n) const { return n ? n->ht : 0; }
    void setHt(Node *n) { if (n) n->ht = 1 + std::max(ht(n->c[0]), ht(n->c[1])); }
    int bf(Node *n) const { return n ? ht(n->c[0]) - ht(n->c[1]) : 0; }

    Node* rotate(Node *x, int dir) {
        int opp = 1 - dir;
        Node *y = x->c[dir];
        x->c[dir] = y->c[opp];
        if (y->c[opp]) y->c[opp]->par = x;
        y->c[opp] = x;
        y->par = x->par;
        x->par = y;
        setHt(x);
        setHt(y);
        return y;
    }

    Node* balance(Node *n) {
        if (!n) return n;
        setHt(n);
        int b = bf(n);
        if (b > 1) {
            if (bf(n->c[0]) < 0) n->c[0] = rotate(n->c[0], 1);
            return rotate(n, 0);
        }
        if (b < -1) {
            if (bf(n->c[1]) > 0) n->c[1] = rotate(n->c[1], 0);
            return rotate(n, 1);
        }
        return n;
    }

    void rebalanceUp(Node *n) {
        while (n) {
            Node *p = n->par;
            bool isRoot = (n == rt);
            Node *newN = balance(n);
            if (isRoot) {
                rt = newN;
            } else {
                if (n == p->c[0]) p->c[0] = newN;
                else p->c[1] = newN;
            }
            n = p;
        }
    }

    Node* findMin(Node *n) const {
        while (n && n->c[0]) n = n->c[0];
        return n;
    }

    Node* findMax(Node *n) const {
        while (n && n->c[1]) n = n->c[1];
        return n;
    }

    void addToFreeList(Node *node) {
        node->c[0] = nullptr;
        node->c[1] = nullptr;
        node->par = nullptr;
        node->next_free = free_head;
        free_head = node;
    }

    void clearFreeList() {
        while (free_head) {
            Node *tmp = free_head;
            free_head = free_head->next_free;
            delete tmp->data;
            delete tmp;
        }
    }

public:
    class const_iterator;

    class iterator {
        friend class map;
        friend class const_iterator;
        Node *nd;
        map *mp;
    public:
        iterator() : nd(nullptr), mp(nullptr) {}
        iterator(Node *n, map *m) : nd(n), mp(m) {}
        iterator(const iterator &o) : nd(o.nd), mp(o.mp) {}
        iterator operator++(int) { iterator t = *this; ++(*this); return t; }
        iterator &operator++() {
            if (!nd) throw invalid_iterator();
            if (nd->c[1]) { nd = mp->findMin(nd->c[1]); }
            else { Node *p = nd->par; while (p && nd == p->c[1]) { nd = p; p = p->par; } nd = p; }
            return *this;
        }
        iterator operator--(int) { iterator t = *this; --(*this); return t; }
        iterator &operator--() {
            if (!nd) { if (!mp || !mp->rt) throw invalid_iterator(); nd = mp->findMax(mp->rt); return *this; }
            if (nd->c[0]) { nd = mp->findMax(nd->c[0]); return *this; }
            Node *p = nd->par; while (p && nd == p->c[0]) { nd = p; p = p->par; } nd = p;
            return *this;
        }
        value_type &operator*() const { return *(nd->data); }
        value_type *operator->() const noexcept { return nd->data; }
        bool operator==(const iterator &rhs) const { return nd == rhs.nd && mp == rhs.mp; }
        bool operator!=(const iterator &rhs) const { return !(*this == rhs); }
        bool operator==(const const_iterator &rhs) const;
        bool operator!=(const const_iterator &rhs) const;
    };

    class const_iterator {
        friend class map;
        friend class iterator;
        Node *nd;
        map *mp;
    public:
        const_iterator() : nd(nullptr), mp(nullptr) {}
        const_iterator(Node *n, map *m) : nd(n), mp(m) {}
        const_iterator(const const_iterator &o) : nd(o.nd), mp(o.mp) {}
        const_iterator(const iterator &o) : nd(o.nd), mp(o.mp) {}
        const_iterator operator++(int) { const_iterator t = *this; ++(*this); return t; }
        const_iterator &operator++() {
            if (!nd) throw invalid_iterator();
            if (nd->c[1]) nd = mp->findMin(nd->c[1]);
            else { Node *p = nd->par; while (p && nd == p->c[1]) { nd = p; p = p->par; } nd = p; }
            return *this;
        }
        const_iterator operator--(int) { const_iterator t = *this; --(*this); return t; }
        const_iterator &operator--() {
            if (!nd) { if (!mp || !mp->rt) throw invalid_iterator(); nd = mp->findMax(mp->rt); return *this; }
            if (nd->c[0]) { nd = mp->findMax(nd->c[0]); return *this; }
            Node *p = nd->par; while (p && nd == p->c[0]) { nd = p; p = p->par; } nd = p;
            return *this;
        }
        const value_type &operator*() const { return *(nd->data); }
        const value_type *operator->() const noexcept { return nd->data; }
        bool operator==(const const_iterator &rhs) const { return nd == rhs.nd && mp == rhs.mp; }
        bool operator!=(const const_iterator &rhs) const { return !(*this == rhs); }
        bool operator==(const iterator &rhs) const;
        bool operator!=(const iterator &rhs) const;
    };

private:
    void insertNode(const value_type &val) {
        if (!rt) {
            rt = new Node(new value_type(val.first, val.second), nullptr);
            sz++;
            return;
        }
        Node *cur = rt;
        Node *parent = nullptr;
        int dir;
        while (cur) {
            parent = cur;
            if (cmp(val.first, cur->data->first)) { dir = 0; cur = cur->c[0]; }
            else if (cmp(cur->data->first, val.first)) { dir = 1; cur = cur->c[1]; }
            else return;
        }
        Node *newNode = new Node(new value_type(val.first, val.second), parent);
        parent->c[dir] = newNode;
        sz++;
        rebalanceUp(parent);
    }

    void copyRec(Node *&node, Node *parent, Node *other) {
        if (!other) { node = nullptr; return; }
        node = new Node(new value_type(other->data->first, other->data->second), parent);
        sz++;
        copyRec(node->c[0], node, other->c[0]);
        copyRec(node->c[1], node, other->c[1]);
    }

    void clearRec(Node *node) {
        if (!node) return;
        clearRec(node->c[0]);
        clearRec(node->c[1]);
        delete node->data;
        delete node;
    }

    Node* findNode(Node *node, const Key &key) const {
        if (!node) return nullptr;
        if (cmp(key, node->data->first)) return findNode(node->c[0], key);
        if (cmp(node->data->first, key)) return findNode(node->c[1], key);
        return node;
    }

    void eraseNode(Node *nd) {
        if (!nd->c[0] || !nd->c[1]) {
            int hasRight = nd->c[1] ? 1 : 0;
            Node *child = nd->c[hasRight];
            Node *parent = nd->par;
            if (child) child->par = parent;
            if (parent) {
                if (nd == parent->c[0]) parent->c[0] = child;
                else parent->c[1] = child;
            } else {
                rt = child;
            }
            addToFreeList(nd);
            rebalanceUp(parent);
        } else {
            Node *succ = findMin(nd->c[1]);
            Node *succPar = succ->par;
            Node *succRight = succ->c[1];
            
            // Remove succ from its position
            if (succRight) succRight->par = succPar;
            if (succPar->c[0] == succ) succPar->c[0] = succRight;
            else succPar->c[1] = succRight;
            
            // Put succ in nd's position
            succ->c[0] = nd->c[0];
            succ->c[1] = nd->c[1];
            succ->par = nd->par;
            if (succ->c[0]) succ->c[0]->par = succ;
            if (succ->c[1]) succ->c[1]->par = succ;
            if (nd->par) {
                if (nd == nd->par->c[0]) nd->par->c[0] = succ;
                else nd->par->c[1] = succ;
            } else {
                rt = succ;
            }
            
            addToFreeList(nd);
            
            // Rebalance from succPar up to root
            if (succPar == nd) {
                rebalanceUp(succ->par);
            } else {
                rebalanceUp(succPar);
            }
        }
    }

public:
    map() : rt(nullptr), free_head(nullptr), sz(0) {}
    map(const map &other) : rt(nullptr), free_head(nullptr), sz(0) { *this = other; }
    map &operator=(const map &other) {
        if (this == &other) return *this;
        clear();
        copyRec(rt, nullptr, other.rt);
        return *this;
    }
    ~map() { 
        clearRec(rt); 
        clearFreeList();
    }

    T &at(const Key &key) {
        Node *n = findNode(rt, key);
        if (!n) throw index_out_of_bound();
        return n->data->second;
    }
    const T &at(const Key &key) const {
        Node *n = findNode(rt, key);
        if (!n) throw index_out_of_bound();
        return n->data->second;
    }
    T &operator[](const Key &key) {
        Node *n = findNode(rt, key);
        if (n) return n->data->second;
        T val{};
        insertNode(value_type(key, val));
        n = findNode(rt, key);
        return n->data->second;
    }
    const T &operator[](const Key &key) const {
        Node *n = findNode(rt, key);
        if (!n) throw index_out_of_bound();
        return n->data->second;
    }

    iterator begin() { return iterator(rt ? findMin(rt) : nullptr, this); }
    const_iterator cbegin() const { return const_iterator(rt ? findMin(rt) : nullptr, const_cast<map*>(this)); }
    iterator end() { return iterator(nullptr, this); }
    const_iterator cend() const { return const_iterator(nullptr, const_cast<map*>(this)); }
    bool empty() const { return sz == 0; }
    size_t size() const { return sz; }
    void clear() { 
        clearRec(rt); 
        rt = nullptr; 
        sz = 0; 
    }

    pair<iterator, bool> insert(const value_type &value) {
        Node *n = findNode(rt, value.first);
        if (n) return {iterator(n, this), false};
        insertNode(value);
        Node *nn = findNode(rt, value.first);
        return {iterator(nn, this), true};
    }

    void erase(iterator pos) {
        if (pos.mp != this) throw invalid_iterator();
        if (pos == end()) throw invalid_iterator();
        eraseNode(pos.nd);
        sz--;
    }

    size_t count(const Key &key) const { return findNode(rt, key) ? 1 : 0; }
    iterator find(const Key &key) {
        Node *n = findNode(rt, key);
        return n ? iterator(n, this) : end();
    }
    const_iterator find(const Key &key) const {
        Node *n = findNode(rt, key);
        return n ? const_iterator(n, const_cast<map*>(this)) : cend();
    }
};

template<class K, class T, class C> bool map<K,T,C>::iterator::operator==(const const_iterator &rhs) const { return nd == rhs.nd && mp == rhs.mp; }
template<class K, class T, class C> bool map<K,T,C>::iterator::operator!=(const const_iterator &rhs) const { return !(*this == rhs); }
template<class K, class T, class C> bool map<K,T,C>::const_iterator::operator==(const iterator &rhs) const { return nd == rhs.nd && mp == rhs.mp; }
template<class K, class T, class C> bool map<K,T,C>::const_iterator::operator!=(const iterator &rhs) const { return !(*this == rhs); }

}
#endif
