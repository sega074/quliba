#include <vector>
#include <atomic>
#include <thread>
#include <iostream>
#include <chrono>
#include <algorithm>    
#include "QUPoints.h"
#include "QUPoints_Functs_test.h"


//#define SIZE_QUEUE_A    64  // Размер очереди а
const uint_fast32_t SIZE_QUEUE_A {64};
// #define SIZE_QUEUE_B    48  // размер очереди b
const uint_fast32_t SIZE_QUEUE_B {48};      

int main(int argc, char** argv) {

 
    std::atomic<uint64_t> count_a{0};   // количtство выполенных операций из a
    std::atomic<uint64_t> count_b{0};   // количество выполенных операций из b
    std::atomic<int>      run_fase{0};


    uint64_t c_count;   // количестов операций чтения и запис 
    uint32_t s_count;   // количистов потоков для отправки
    uint32_t r_count;   // количестов потоков для приема
    uint32_t n_elem;    // количиство элементов которые записываются в очереди
                        // диапзон от 1 до общая размерность очередей -1


    if (argc <= 1){
        std::cout << std::string(argv[0]) << " entries_in_one_of_the_3_inetrations streams_of_writers streams_of_readers";
        std::cout << std::endl;
        return 1;
    }

    if (argc >=2){  // количестов операций чтение и записи очереди
        c_count = std::stoul(std::string(argv[1]));
    } else if (argc == 1 || c_count == 0) {
        c_count = 100000l;
    }

    if (argc >=3){  // количестов потоков для читающих из очереди a
        s_count = std::stoul(std::string(argv[2]));
    } else if ( s_count == 0) {
        s_count = 3;
    }

    if (argc >=4){  // количичтво потоков читающих из очереди b
        r_count = std::stoul(std::string(argv[3]));
    } else if ( r_count == 0) {
        r_count = 3;
    }

    if(argc >= 5){      // количество элементов первоначально записанных в очередь
        n_elem = std::stoul(std::string (argv[4]));
    } else if (n_elem == 0){
        n_elem = SIZE_QUEUE_A -1;
    }

    QUPoints <uint64_t, SIZE_QUEUE_A> qp_a;
    QUPoints <uint64_t, SIZE_QUEUE_B> qp_b;

    std::vector <std::thread> thread_a;  // вектор потоков читает из a
    std::vector <std::thread> thread_b;  // вектор потоков читает из b




    // первоначальное заполнение очередей
    std::cout << std::endl;
    for (int i = 0; i < n_elem; ++i){
        UT<uint64_t>  el;

        if(qp_a.elInQueue() < i){
            if (qp_a.addElem(std::make_unique<uint64_t>(i)) == nullptr){
                std::cout <<  i << " ";
            }
        } else if (qp_b.elInQueue() < i - qp_a.elInQueue()){
            if (qp_b.addElem(std::make_unique<uint64_t>(i)) == nullptr) {
                std::cout <<  i << " ";
            }
        }
    }
    std::cout << std::endl;

    std::cout << std::endl << "Pervision elem qp_a:" << qp_a.elInQueue() << std::endl;
    std::cout << "Pervision elem qp_b:" << qp_b.elInQueue() << std::endl << std::endl;

    
    for (int i = 0 ; i < r_count; ++i){     // запуск потоков приема
        thread_a.emplace_back(thread_qp_x < uint64_t,
                                            SIZE_QUEUE_A,
                                            SIZE_QUEUE_B >
                                            (c_count, count_a, run_fase, qp_a, qp_b));
    }

    for (int i = 0 ; i < s_count; ++i){     // запуск потоков отправки
        thread_b.emplace_back(thread_qp_x < uint64_t,
                                            SIZE_QUEUE_B,
                                            SIZE_QUEUE_A >
                                            (c_count, count_b, run_fase, qp_b, qp_a));
    }
    
    run_fase++; // Элемент одновременного запкска

    // дождаться завершения
    while (run_fase > 0 && count_a < c_count && count_b < c_count){
        std::this_thread::sleep_for(std::chrono::microseconds(1000000));
    }

    run_fase = 0;

    for (int i = 0 ; i < r_count ; ++i){    // ожидение звершения потоков отправки
        thread_a[i].join();
    }

    for (int i = 0 ; i < s_count ; ++i){
        thread_b[i].join();
    }

    std::cout <<  std::endl <<"elments in to qp_a:" << qp_a.elInQueue() << std::endl;
    std::cout << "elments in to qp_b:" << qp_b.elInQueue() << std::endl << std::endl;
    
    const uint_fast32_t el =qp_a.elInQueue()+ qp_b.elInQueue();
    
    std::vector <uint64_t> rez (el);
    for (int i = 0 ; i < el; ++i){
        UT <uint64_t> u;
        if (qp_a.elInQueue() > 0 ){
            u = qp_a.getElem();
            if (u != nullptr) {
                rez[i] = *u; 
            }
            continue;
        }
        if(qp_b.elInQueue() > 0){
            u = qp_b.getElem();
            if (u != nullptr) {
                rez[i] = *u;
            }  
            continue;
        }
    }
    
    std::sort(std::begin(rez), std::end(rez));

    std::cout << "count_a:" << count_a << std::endl;
    std::cout << "count_b:" << count_b << std::endl;
    std::cout << std::endl;

  
    for (int i = 0 ; i < el ; ++i){
       
        std::cout << rez[i] << " ";
       
    }

    std::cout << std::endl;
    
    return 0;

}