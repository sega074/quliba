#pragma once

#include <vector>
#include <array>
#include <memory>
#include <atomic>
#include <thread>
#include <future>
#include <numeric>
#include <iostream>
#include <chrono>
#include <string>
#include <mutex>


template <class T, uint32_t sz_> class  QPoints {   // очередь с сиспользованием указателей на тип и совмещенное 
                                            // использование указателей нача ла и конца в одной переменной 
    private:

    struct point_p {
        uint32_t    p_coutn{0};             // защита от ABA
        uint16_t    p_beg{0};               // указатель на индекс с которого пишем
        uint16_t    p_end{0};               // указатель на индекс с которого читаем
    };

    std::atomic <point_p>   p_;             // указатели на начало и конец очереди
    std::array<std::atomic<T*>,sz_> vec_element_;// вектор для хранения элементов очереди
    const uint32_t atm_count_;                      // кол попыток внести изменения в атомарные переменные

    public:

        QPoints (uint32_t tc = 16 )
        : vec_element_(std::array<std::atomic<T*>,sz_ > ())
        , atm_count_(tc) {
        for(int i = 0 ; i < sz_; ++i) {
            vec_element_[i] = nullptr;
        }
    }

    ~QPoints(){}

    int idForAdd(){ // получить индекс для добавления элемента 
                    // для предотвращения повторной выдачи кол пр 
                    // должнобыть меньше размера очереди
        uint32_t count {0};
        point_p p;  // предыдыщее значение
        point_p pn; // новое значение
        do {
            if(count++ > atm_count_ ){ return -1; }
            pn = p = p_.load(std::memory_order_acquire);
            pn.p_beg++;
            pn.p_coutn++;
            if (pn.p_beg == sz_) pn.p_beg = 0;
            std::atomic_thread_fence(std::memory_order_release);
            if (pn.p_beg == pn.p_end){ return -1;}
        } while (!p_.compare_exchange_weak(p,pn,std::memory_order_release));

        return p.p_beg;
    }

    int idForGet(){ // полчить индекс для чтения элемента 

        uint32_t count {0};
        point_p  p;     // прежнее значение 
        point_p  pn;    // новое значение
        
        do {
            if( count++ > atm_count_ ){ return -1; }
            pn = p = p_.load(std::memory_order_acquire);
            if( pn.p_beg == pn.p_end ){  return -1; }
            std::atomic_thread_fence(std::memory_order_acquire);
            pn.p_end++;
            pn.p_coutn++;
            if (pn.p_end == sz_) pn.p_end = 0;
        } while (!p_.compare_exchange_weak(p,pn, std::memory_order_release));

        return p.p_end;
    }

    T* addEl(int id , T* el){
        if((id == -1) || (id >= sz_) || (el == nullptr)) {return el;}
        uint32_t count {0};
        T* tmp_el;
        do {
            count++;
            if(count >= atm_count_) return el;
            tmp_el = nullptr;
        } while(!vec_element_[id].compare_exchange_weak (tmp_el,el,std::memory_order_release));
        return nullptr;
    }

    T* getEl(int id){
        if ((id == -1) || (id >= sz_)) {return nullptr;}
        uint32_t count {0};
        T* ret;
        T* tmp_el;
        do{
            count++;
            if(count >= atm_count_) return nullptr;
            tmp_el = ret = vec_element_[id].load(std::memory_order_acquire);
        } while(!vec_element_[id].compare_exchange_weak (tmp_el,nullptr,std::memory_order_release));
        return ret;
    }


    T* addElem(T* elT){
        if (elT == nullptr){ return nullptr;  }      
        int id = idForAdd();
        if (id == -1) { return elT;}
        return addEl(id,elT);
    }

    T* getElem(){
      int id = idForGet();
      if(id == -1) {return nullptr;}
      return getEl(id);
    }

    uint32_t lenQueue()const {return sz_;}

    uint32_t elInQueue() const {

        point_p p = p_.load(std::memory_order_acquire);

        if(p.p_beg >= p.p_end){
            return p.p_beg - p.p_end;
        } else {
            return sz_ + p.p_beg - p.p_end;
        }
    }

    uint32_t getBeg() const {
        return (p_.load(std::memory_order_relaxed)).p_beg;
    }

    uint32_t getEnd() const {
        return (p_.load(std::memory_order_relaxed)).p_end;
    }

};
