#include "SimpleLRU.h"

namespace Afina {
namespace Backend {


bool SimpleLRU::DeleteNodeForSpace(const std::string &key, const std::string &value){
    size_t _put_size = value.size() + key.size();
    if (_now_size + _put_size > _max_size){
        lru_node* step = _lru_tail;

        std::size_t _delete_size = step->key.size() + step->value.size();

        while(_now_size + _put_size > _max_size){
            _now_size -= _delete_size;
            _lru_index.erase(step->key);
            step = _lru_tail->prev;
            _lru_tail = step;
            _delete_size = step->key.size() + step->value.size();
        }
    }
}

bool SimpleLRU::MoveNode(lru_node &node){
    if (!node.prev)
        return true;
    else if (!node.next)
        _lru_tail = node.prev;
    else//some element in list
        node.next->prev = node.prev;

    std::unique_ptr<lru_node> tmp;

    std::swap(node.next, tmp);
    std::swap(node.next, _lru_head);
    std::swap(node.prev->next, tmp);
    std::swap(_lru_head, tmp);
    return true;
}

bool SimpleLRU::PutNode(const std::string &key, const std::string &value, size_t _put_size){
    DeleteNodeForSpace(key, value);

    lru_node* new_node = new lru_node{key, value};

    if (_now_size == 0){
        _lru_head = std::unique_ptr<lru_node>(new lru_node);
        _lru_tail = new_node;
    }

    _now_size += _put_size;
    _lru_head->prev = new_node;

    (new_node->next).swap(_lru_head);
    _lru_head.release();
    _lru_head.reset(new_node);
    _lru_index.insert({std::reference_wrapper<const std::string>(_lru_head->key),
            std::reference_wrapper<lru_node>(*_lru_head)});
    return true;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Put(const std::string &key, const std::string &value) {
    std::size_t _put_size = key.size() + value.size();
    if (_put_size > _max_size)
        return false;

    if(_lru_index.count(key))
        Set(key, value);
    else
        return PutNode(key, value, _put_size);
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::PutIfAbsent(const std::string &key, const std::string &value) {
    std::size_t _put_size = key.size() + value.size();
    if (_put_size > _max_size)
        return false;

    if (!_lru_index.count(key))
        return PutNode(key, value, _put_size);
    return false;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Set(const std::string &key, const std::string &value) {
    if (_lru_index.count(key)){
        auto it = _lru_index.find(key);
        lru_node &node = it->second;
        _now_size -= (node.value.size() + key.size());

        DeleteNodeForSpace(key, value);
        node.value = value;
        _now_size += value.size() + key.size();

        MoveNode(node);
        return true;
    }
    else
        return false;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Delete(const std::string &key) {
    if (_lru_index.count(key)){
        auto it = _lru_index.find(key);
        lru_node &a = it->second;
        _lru_index.erase(key);

        size_t _delete_size = a.key.size() + a.value.size();
        _now_size -= _delete_size;

        if ((!a.next) && (!a.prev))
            _lru_head.reset();
        else if (!a.prev)
            _lru_head = std::move(_lru_head->next);
        else if (!a.next)
            _lru_tail = a.prev;
        else{
            a.next->prev = a.prev;
            a.prev->next = std::move(a.next);
        }
    }
    else
        return false;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Get(const std::string &key, std::string &value) {
    if (_lru_index.count(key)){
        auto it = _lru_index.find(key);
        lru_node &node = it->second;
        value = node.value;
        MoveNode(node);
        return true;
    }
    else
        return false;
}

} // namespace Backend
} // namespace Afina

