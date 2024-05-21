#pragma once

#include <vector>
#include <array>
#include <memory>
#include <atomic>
#include <numeric>

template <typename T>   using  UT =  std::unique_ptr<T>;    // определение типа храненеия 


template <class T, uint_fast32_t sz_> class QUPoints {
    public:
    struct point_p {
        uint32_t    p_count{0};                //  защита от ABA
        uint16_t    p_beg{0};                  // указатель на индекс с которого пишем
        uint16_t    p_end{0};                  // указатель на индекс с которого читаем
    };

    std::atomic <point_p>   p_;             // указатели на начало и конец очереди
    const uint32_t atm_count_;                    // количестов попыток

    std::array<UT<T>, sz_>  vec_element_;
   
    std::vector< std::atomic<int>> vec_fint_;   // 0 -- пусто
                                                // 1 -- выполняется операция 
                                                // 2 -- присутствуют данные
    
    public:

   QUPoints(const uint32_t tc = 16)
    : atm_count_(tc)
    , vec_element_(std::array<UT<T>, sz_>())
    , vec_fint_(std::vector<std::atomic<int>>(sz_))
    {
        for(int i = 0 ; i < sz_; ++i) {
            vec_element_[i]=nullptr;
            vec_fint_[i]=0;
        }
    }

    ~QUPoints(){}


    int idForAdd(){ // получить индекс для добавления элемента 
                    // для предотвращения повторной выдачи кол пр 
                    // должнобыть меньше размера очереди
        int count = 0;
        point_p p;  // предыдыщее значение
        point_p pn; // новое значение
        do {
            if(count++ > atm_count_ ){ return -1; }
            pn = p = p_.load(std::memory_order_acquire);
            pn.p_beg++;
            pn.p_count++;
            if (pn.p_beg == sz_) {pn.p_beg = 0;}
            if (pn.p_beg == pn.p_end){ return -1;}
        } while (!p_.compare_exchange_weak(p,pn,std::memory_order_release));

        return p.p_beg;
    }

    int idForGet(){ // полчить индекс для чтения элемента 

        int count = 0;
        point_p  p;     //прежнее значение 
        point_p  pn;    // новое значение
        
        do {
            if( count++ > atm_count_ ){ return -1; }
            pn = p = p_.load(std::memory_order_acquire);
            if( p.p_beg == p.p_end ){  return -1; }
            pn.p_end++;
            pn.p_count++;
            if (pn.p_end == sz_) pn.p_end = 0;
        } while (!p_.compare_exchange_weak(p,pn, std::memory_order_release));

        return p.p_end;
    }


    UT<T> addEl(int id ,UT<T> elT ) {
        if(elT == nullptr || id < 0 || id >= sz_) {
            return std::move(elT);
        }
        int fl;
        uint32_t count {0};

        do{
            fl = 0;
            if ( ++count >= atm_count_){
                 return std::move(elT);
            }
        } while (!vec_fint_[id].compare_exchange_weak(fl,1,std::memory_order_release));

        vec_element_[id]= std::move(elT);
        count = 0;

        do {
            fl = 1;
        } while(!vec_fint_[id].compare_exchange_weak(fl, 2, std::memory_order_release));
        return nullptr;
    }


    UT<T> getEl(int id){
        if (id < 0 || id >= sz_) {
             return nullptr;
        }
        int fl;
        uint32_t count {0};
        UT<T> ret {nullptr};

        do{
            fl = 2;
            if ( ++count >= atm_count_){
                 return nullptr;
            }
        } while (!vec_fint_[id].compare_exchange_weak(fl,1,std::memory_order_release));

        ret = std::move(vec_element_[id]);
        count = 0;

        do {
            fl = 1;
        } while(!vec_fint_[id].compare_exchange_weak(fl, 0, std::memory_order_release));

        return std::move (ret);
    }


    UT<T> addElem(UT<T> elT){     

        if (elT == nullptr){
            return nullptr; 
        }  
        int id = idForAdd();
        if (id == -1) {
            return std::move(elT); 
        }
        return  addEl(id,std::move(elT)); 
    }

    UT<T> getElem(){

        int id = idForGet();
        if (id == -1) {
            return nullptr;
        }
        return getEl(id);  
    }

    uint32_t lenQueue()const {return sz_;}

    uint32_t elInQueue() const {

        point_p p = p_.load(std::memory_order_relaxed);

        if(p.p_beg >= p.p_end){
            return p.p_beg - p.p_end;
        } else {
            return sz_ + p.p_beg - p.p_end;
        }
    }

/**
 * @brief Прочитать указателина начала и конца данных расположенных в очереди
 * 
 * @return std::tuple <uint16_t, uint16_t>  пара значений индекса начала и индекса конца
 */
    std::tuple <uint16_t, uint16_t> getBegEnd() const noexcept {

        point_p p = p_.load(std::memory_order_relaxed);

        return std::make_tuple(p.p_beg,p.p_end);
    }
};