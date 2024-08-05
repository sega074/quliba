#pragma once

#include <array>
#include <memory>
#include <atomic>
#include <numeric>

template <typename T>   using  UT =  std::unique_ptr<T>;    // определение типа храненеия 


template <class T, uint32_t sz_> class QUMPoints {
    public:
    struct point_p {
        uint32_t    count{0};           //  защита от ABA (ДБ всегда четная)
                                        // не чет это промежуточное состояние
        uint32_t    point{0};           // указатель на индекс с которого пишем
    };

    std::atomic <point_p>   p_beg;      // указатели на начало очереди
    std::atomic <point_p>   p_end;      // указатель на конец очереди
    const uint32_t atm_count_;          // количестов попыток

    std::array<UT<T>, sz_>  vec_element_;
    
    public:

   QUMPoints(const uint32_t tc = 16)
    : p_beg({0,0})
    , p_end({0,0})
    , atm_count_(tc)
    , vec_element_(std::array<UT<T>, sz_>())
    {}

    ~QUMPoints(){}


    UT<T> addElem(UT<T> elT){     

        if (elT == nullptr) return std::move(elT);
        int count = 0;
        point_p p;  // предыдущее значение
        point_p     pm;     //промежуточное значение
        point_p pn; // новое значение
        uint32_t p_tmp = 0;
        do {
            if(++count >= atm_count_ ){return std::move(elT);}
            pn = p = p_beg.load(std::memory_order_acquire);
            if (pn.count& 0x1) {continue;}  // должно быить ченое значение
            p_tmp  = pn.point + 1;
            ++pn.count;
            if (p_tmp == sz_) {p_tmp = 0;}
            if (p_tmp == (p_end.load(std::memory_order_release)).point){return std::move(elT);}
        } while (!p_beg.compare_exchange_weak(p,pn
                ,std::memory_order_release,std::memory_order::memory_order_relaxed));
        pm = pn;
        vec_element_[p_tmp] = std::move(elT);
        //
        ++pn.count;
        pn.point = p_tmp;
        count = 0;
        do{
            p = pm;
            if (++count >= atm_count_){return std::move(elT);}
        }while (!p_beg.compare_exchange_weak(p,pn,std::memory_order_release, std::memory_order_relaxed));
        //p_beg.store(pn,std::memory_order_release);
        return nullptr;
    }

    UT<T> getElem(){

        int count = 0;
        point_p     p;     //прежнее значение
        point_p     pm;     //промежуточное значение
        point_p     pn;    // новое значение
        UT<T>       ret = nullptr;
        
        do {
            if( ++count >= atm_count_ ){ return nullptr; }
            pn = p = p_end.load(std::memory_order_acquire);
            if(pn.count& 0x1){continue;}
            if(p.point == (p_beg.load(std::memory_order_acquire)).point){return nullptr;}
            ++pn.count;
        } while (!p_end.compare_exchange_weak(p,pn,
                std::memory_order_release, std::memory_order_relaxed));
        pm = pn;
        ++pn.count;
        if (++pn.point == sz_) pn.point = 0;
        ret = std::move(vec_element_[pn.point]);
        //
        //p_end.store(pn,std::memory_order_release);
        count = 0;
        do{
            p = pm;
            if (++count >= atm_count_){return std::move(ret);}
        }while (!p_end.compare_exchange_weak(p,pn,std::memory_order_release, std::memory_order_relaxed));

        return std::move(ret);
    }

    uint32_t lenQueue()const {return sz_;}

    uint32_t elInQueue() const {
        uint32_t count=0;
        point_p pb;
        point_p pe;
        do{
            pb = p_beg.load(std::memory_order_relaxed);
            pe = p_end.load(std::memory_order_relaxed);
        } while((pb.count&0x1 || pe.count & 0x1 ) && ++count <= atm_count_);

        if (count >= atm_count_) return UINT32_MAX;

        if(pb.point >= pe.point){
            return pb.point - pe.point;
        } else {
            return sz_ + pb.point - pe.point;
        }
    }

/**
 * @brief Прочитать указателина начала и конца данных расположенных в очереди
 * 
 * @return std::tuple <uint16_t, uint16_t>  пара значений индекса начала и индекса конца
 */
    std::tuple <uint32_t, uint32_t> getBegEnd() const noexcept {

        uint32_t count=0;
        point_p pb;
        point_p pe;
        do{
            pb = p_beg.load(std::memory_order_relaxed);
            pe = p_end.load(std::memory_order_relaxed);
        } while((pb.count&0x1 || pe.count & 0x1 ) && ++count <= atm_count_);

        if (count >= atm_count_) return std::make_tuple(UINT32_MAX,UINT32_MAX);;

        return std::make_tuple(pb.point,pe.point);
    }

    constexpr uint32_t getsz() const {return sz_;}
};