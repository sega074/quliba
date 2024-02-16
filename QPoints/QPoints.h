#pragma once

// sega074@yandex.ru (Sergey Dmitrievich Gordeychuk)

#include <array>
#include <memory>
#include <atomic>
#include <tuple>


/**
 * @brief Очередь указателей на реализации типа T фиксированного размера (закольцованная очередь)
 * 
 * @tparam T тип данных для указателей 
 * @tparam sz_ размер очереди ( в пределах от 1 до 65536)
 */
template <class T, uint32_t sz_> class  QPoints { 
                                 
    private:

    struct point_p {
        uint32_t    p_coutn{0};                     // защита от ABA
        uint16_t    p_beg{0};                       // указатель на индекс с которого пишем
        uint16_t    p_end{0};                       // указатель на индекс с которого читаем
    };

    std::atomic <point_p>   p_;                     // указатели на начало и конец очереди
    std::array<std::atomic<T*>,sz_> vec_element_;   // вектор для хранения элементов очереди
    const uint32_t atm_count_;                      // кол попыток внести изменения в атомарные переменные

    public:
/**
 * @brief Конструктор с параметрами
 * 
 * @param tc количиство совершаемых попыток определенного действия при атомарных операциях
 */
    QPoints (uint32_t tc = 16 )
        : vec_element_(std::array<std::atomic<T*>,sz_ > ())
        , atm_count_(tc) {
            for(int i = 0 ; i < sz_; ++i) {
                vec_element_[i] = nullptr;
            }
    }

/**
 * @brief Деструктор
 * 
 */
    ~QPoints() noexcept {}

/**
 * @brief Получить индекс в массиве очереди для добавления элемента
 * 
 * @return int индекс по которому доступна операция добавления, или -1
 *  при неудачи или отсутствия места в очереди 
 */
    int idForAdd() noexcept {
        uint32_t count {0};
        point_p p;  // предыдыщее значение
        point_p pn; // новое значение
        do {
            if(count++ > atm_count_ ){ return -1; }
            pn = p = p_.load(std::memory_order_acquire);
            pn.p_beg++;
            pn.p_coutn++;
            if (pn.p_beg == sz_) {pn.p_beg = 0;}
            if (pn.p_beg == pn.p_end){ return -1;}
        } while (!p_.compare_exchange_weak(p,pn,std::memory_order_release));

        return p.p_beg;
    }

/**
 * @brief Получить индекс для чтения элемента
 * 
 * @return int индекс по которому доступна операция чтения элемента или -1
 * при неудачи или отсутствия элементов в очереди
 */
    int idForGet() noexcept { 

        uint32_t count {0};
        point_p  p;     // прежнее значение 
        point_p  pn;    // новое значение      
        do {
            if( count++ > atm_count_ ){ return -1; }
            pn = p = p_.load(std::memory_order_acquire);
            if( pn.p_beg == pn.p_end ){  return -1; }
            pn.p_end++;
            pn.p_coutn++;
            if (pn.p_end == sz_) {pn.p_end = 0;}
        } while (!p_.compare_exchange_weak(p,pn, std::memory_order_release));

        return p.p_end;
    }

/**
 * @brief Добавить элемент указателя на реализацию типа T в очередь по индексу
 * 
 * @param id индес полученный из метода idForAdd()
 * @param el добавляемый элемент T*
 * @return T* возврашаемое значение nullptr при удачи или занчение переданное через el
 */
    T* addEl(int id , T* el) noexcept {
        if((id == -1) || (id >= sz_) || (el == nullptr)) {return el;}
        uint32_t count {0};
        T* tmp_el{nullptr};
        do {
            if( (tmp_el != nullptr) ||  (++count >= atm_count_) ) return el;
            tmp_el = nullptr;
        } while(!vec_element_[id].compare_exchange_weak (tmp_el,el,std::memory_order_release));
        return nullptr;
    }

/**
 * @brief Прочитаь значение элемента из очереди по индексу
 * 
 * @param id индекс полученный из метода idForGet()
 * @return T* возвращаемое значение типа T* при удачи или nullptr 
 */
    T* getEl(int id) noexcept {
        if ((id == -1) || (id >= sz_)) {return nullptr;}
        uint32_t count {0};
        T* ret;
        T* tmp_el;
        do{
            tmp_el = ret = vec_element_[id].load(std::memory_order_acquire);
            if((tmp_el == nullptr) ||  (count++ >= atm_count_)) return nullptr;
        } while(!vec_element_[id].compare_exchange_weak (tmp_el,nullptr,std::memory_order_release));
        return ret;
    }

/**
 * @brief Добавление элемента в очередь
 * 
 * @param elT элемент для добавления 
 * @return T* возвращаемое значение nullptr при удачном добавлении иначе переданное значение elT
 */
    T* addElem(T* elT) noexcept {
        if (elT == nullptr){ return nullptr;  }      
        int id = idForAdd();
        if (id == -1) { return elT;}
        return addEl(id,elT);
    }

/**
 * @brief Получние элемента из очереи 
 * 
 * @return T* возвращемый элемент или nullptr при неудачи
 */
    T* getElem() noexcept {
      int id = idForGet();
      if(id == -1) {return nullptr;}
      return getEl(id);
    }

/**
 * @brief Прочитаь размер очереди
 * 
 * @return uint32_t размер очереди
 */
    uint32_t lenQueue()const noexcept {return sz_;}

/**
 * @brief Прочитаь количистов элементов доступных для чтения из очереди
 * 
 * @return uint32_t количиство элементов доступных для чтения
 */
    uint32_t elInQueue() const noexcept {

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
