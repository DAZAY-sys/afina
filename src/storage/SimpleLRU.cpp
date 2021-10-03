#include "SimpleLRU.h"

namespace Afina {
namespace Backend {

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Put(const std::string &key, const std::string &value) {
    if(_lru_index.count(key)){
        auto it = _lru_index.find(key);
        it->second.get().value = value;
        return true;
    }
    else
        return SimpleLRU::PutIfAbsent(key, value);
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::PutIfAbsent(const std::string &key, const std::string &value) {
    if (!_lru_index.count(key)){
        std::size_t _put_size = key.size() + value.size();
        if (_put_size > _max_size){
            return false;
        }
        
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
    else
        return false;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Set(const std::string &key, const std::string &value) {
    if (_lru_index.count(key)){

        auto it = _lru_index.find(key);
        it->second.get().value = value;
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
            _lru_head = std::move(_lru_head);
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
        value = it->second.get().value;
        return true;
    }
    else
        return false;
}

} // namespace Backend
} // namespace Afina

