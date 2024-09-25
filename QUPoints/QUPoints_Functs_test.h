#pragma onece

#include <functional>
#include <thread>
#include <chrono>
#include "QUPoints.h"
    /**
     * @brief Тестовый класс-функция выполняющая чтение элементов типа  ЕЙ из одной очереди
     *        и занисывающая в другую очередь
     * 
     * @tparam TQ   тип элемента очереди
     * @tparam SZ_I размер очереди чтения 
     * @tparam SZ_O размер очереди записи
     * 
     */

    template  < class TQ,uint_fast32_t SZ_I,uint_fast32_t SZ_O > class thread_qp_x {

        const uint64_t          _c_count;        /// количестов операций
        std::atomic<uint64_t>&  _count;          /// счетчик операций
        std::atomic<int>&       _run_fase;       /// признак работы и завершени 
        QUPoints <TQ,SZ_I>&     _qp_i;           /// очередь для чтения
        QUPoints <TQ,SZ_O>&     _qp_o;



        public:

            thread_qp_x(  
                            const uint64_t c_count,         /// количестов операций
                            std::atomic<uint64_t>& count,   /// счетчик операций
                            std::atomic<int>& run_fase,     /// признак работы и завершени 
                            QUPoints <TQ,SZ_I>& qp_i,       /// очередь для чтения
                            QUPoints <TQ,SZ_O>& qp_o        /// очередь для записи
                        ):
                        _c_count(c_count),
                        _count(count),
                        _run_fase(run_fase),
                        _qp_i(qp_i),
                        _qp_o(qp_o)
                        {}


        void operator() () {
        UT <TQ> ret;                                  
       
        while (_run_fase == 0);    // Для ожидания общего пуска и завершения

        while (_run_fase > 0 && _count < _c_count ){
          
            // прочитаь элемент из одной очереди 
           
            while (_run_fase > 0 && ret == nullptr ) {
                ret = _qp_i.getElem();
                if (ret == nullptr){
                    std::this_thread::sleep_for(std::chrono::microseconds(1));
                }
            }
        
            //  отправить элемент в другую очередь

            while (ret != nullptr){
                ret = _qp_o.addElem(std::move(ret));
            }
            _count++;
        }
    }
    };
