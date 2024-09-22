#include <vector>
#include <memory>
#include <atomic>
#include <thread>
#include <future>
#include <numeric>
#include <iostream>
#include <chrono>
#include <string>
#include <mutex>
#include <stdlib.h>    
#include "QUPoints.h"

#define ELEMENT_1 111111111
#define ELEMENT_2 222222222
#define ELEMENT_3 333333333

std::mutex lock_m;  //  Для операций В/В
        

int main(int argc, char** argv) {

    errno = 0;
    int l = strtol(nullptr,nullptr,0);

    int lr = errno;

    const uint32_t try_atm_q = 10000;
    
   
   // volatile bool bl_end = false;

    std::atomic <int> bl_end {2};

    std::atomic<uint64_t> count1{0};
    std::atomic<uint64_t> count2{0};
    std::atomic<uint64_t> count3{0};

    std::atomic<uint64_t> ucount1{0};
    std::atomic<uint64_t> ucount2{0};
    std::atomic<uint64_t> ucount3{0};


    uint64_t c_count;
    uint32_t s_count;   // количистов потоков для отправки
    uint32_t r_count;   // количестов потоков для приема

    if (argc <= 1){
        std::cout << std::string(argv[0]) << " entries_in_one_of_the_3_inetrations streams_of_writers streams_of_readers";
        std::cout << std::endl;
        return 1;
    }

    if (argc >=2){
        c_count = std::stoul(std::string(argv[1]));
    } else if (argc == 1 || c_count == 0) {
        c_count = 100000l;
    }

    if (argc >=3){
        s_count = std::stoul(std::string(argv[2]));
    } else if ( s_count == 0) {
        s_count = 3;
    }

    if (argc >=4){
        r_count = std::stoul(std::string(argv[3]));
    } else if ( r_count == 0) {
        r_count = 3;
    }

    QUPoints <uint64_t,64> qp(try_atm_q); 

    QUPoints <uint64_t,32> qp_i1(try_atm_q);

    QUPoints <uint64_t,32> qp_i2(try_atm_q);

    QUPoints <uint64_t,32> qp_i3(try_atm_q);


    for (int i = 0; i < 25; ++i){
        UT<uint64_t>  el;


        el = qp_i1.addElem(std::make_unique<uint64_t>(ELEMENT_1));
        if(el != nullptr){
            std::cout << "err1" << std::endl;
        }

        el = qp_i2.addElem(std::make_unique<uint64_t>(ELEMENT_2));
        if(el != nullptr){
            std::cout << "err2" << std::endl;
        }

        el = qp_i3.addElem(std::make_unique<uint64_t>(ELEMENT_3));
        if(el != nullptr){
            std::cout << "err3" << std::endl;
        }
    }
    //
    UT< uint64_t> ret; 
    int coutn_id = 0; 
    int id;   
    do {
        id = qp_i1.idForGet();
        coutn_id++;
    } while (id == -1 && coutn_id < try_atm_q);
    int try_recv = 0;
    do {
        ret  = qp_i1.getEl(id);
        try_recv++;
        if (ret == nullptr)std::this_thread::sleep_for(std::chrono::microseconds(1) );
    } while ((ret == nullptr) && (try_recv < try_atm_q));

    std::vector <std::thread> thread_send;  // вектор потоков для отправки 
    std::vector <std::thread> thread_recv;  // вектор потоков для приема

    //----------------------------------------------------------------- send

    std::function <void(const int, const uint64_t,const uint64_t,const int , std::atomic<uint64_t>&, QUPoints <uint64_t,32>&, QUPoints <uint64_t,64>&)> 
            thread_send_x = [&](const int nid, const uint64_t elem,const uint64_t c_count, int try_atm_q, std::atomic<uint64_t>& count, QUPoints<uint64_t,32>& qp_i, QUPoints <uint64_t,64>& qp){
       
    while (bl_end == 2);

    while (count < c_count ){
          
            UT< uint64_t> ret;
            while ((ret = qp_i.getElem()) == nullptr ) {
                if (ret == nullptr && qp_i.elInQueue() <= 2){
                    qp_i.addElem(std::make_unique<uint64_t>(elem));
                }
                std::this_thread::sleep_for(std::chrono::microseconds(1));
            }

            if (*ret == elem) {
                int id_add = qp.idForAdd();
                if (id_add == -1){  // вернуть образец обратно в очередь так как индекс для запис не получен
                    qp_i.addElem(std::move(ret));
                } else {    // записать элемент
                    UT< uint64_t> ret_add = std::move( qp.addEl(id_add,std::move(ret)));
                    if (ret_add == nullptr){
                        count++;
                    } 
                }
            } else {    
                std::lock_guard<std::mutex> lk{lock_m};
                std::cout << "(send) err jther elem " << elem << " from " << nid  << " ret "<< (ret != nullptr ? *ret: 0) << std::endl;
            }

    }
        std::cout << "end send " << elem  << " " << std::endl;
        return;
    };


    // ----------------------------------------------------- Q rcv


    auto thread_recv_q = [&](){
       

        while(bl_end == 2);

        int atm = 100000;
        UT< uint64_t> ret ;

        while ( /* bl_end > 0 || */ (qp.elInQueue() > 0 || atm > 0) ){    
            int id = qp.idForGet();
            if (id == -1){
                //if (qp.elInQueue() == 0) {--atm;}
                --atm;
                std::this_thread::sleep_for(std::chrono::microseconds(1));
                continue;
            }
           
           
            
            while ((ret = std::move (qp.getEl(id))) == nullptr  );

            
                atm = 1000000;
                
                if (*ret == ELEMENT_1) {
                    ucount1++;
                    qp_i1.addElem(std::move(ret));
                } else if (*ret == ELEMENT_2) {
                    ucount2++;
                    qp_i2.addElem(std::move(ret));
                } else if (*ret == ELEMENT_3) {
                    ucount3++;
                    qp_i3.addElem(std::move(ret));
                } else {
                    std::lock_guard<std::mutex> lk{lock_m};
                    std::cout << "err(recv) get elem: " << *ret << std::endl; // !!!
                }
        }

    };

    
    for (int i = 0 ; i < r_count; ++i){     // запуск потоков приема
            thread_recv.emplace_back(thread_recv_q);
    }

    for (int i = 0 ; i < s_count; ++i){     // запуск потоков отправки
        int j = i%3;
        if (j == 0){
            thread_send.emplace_back (thread_send_x, i,ELEMENT_1, c_count, try_atm_q, std::ref(count1), std::ref(qp_i1), std::ref(qp) );
        } else if (j == 1){
            thread_send.emplace_back (thread_send_x, i,ELEMENT_2, c_count, try_atm_q, std::ref(count2), std::ref(qp_i2), std::ref(qp) );
        } else {
            thread_send.emplace_back (thread_send_x, i,ELEMENT_3, c_count, try_atm_q, std::ref(count3), std::ref(qp_i3), std::ref(qp) );
        }
    }
    
    bl_end--; // Элемент одновременного запкска

    for (int i = 0 ; i < s_count ; ++i){    // ожидение звершения потоков отправки
        thread_send[i].join();
    }

    
    //bl_end= 0;   // установить признак завершения

   

    for (int i = 0 ; i < r_count ; ++i){
        thread_recv[i].join();
    }

   

    std::cout << std::dec << std::endl << std::endl;
    std::cout << "send count1 " << count1      << " receive cont1 " << ucount1 << std::endl;
    std::cout << "send count2 " << count2      << " receive cont2 " << ucount2 << std::endl;
    std::cout << "send count3 " << count3      << " receive cont3 " << ucount3 << std::endl;

    if ( (count1  != ucount1) || (count2 != ucount2) || (count3 != ucount3) ){
        std::cout  << "first " << qp.p_.load().p_beg;
        std::cout << " end " << qp.p_.load().p_end;
        std::cout << " elem " << qp.elInQueue() << std::endl;

        for (int i = 0; i < qp.vec_element_.size(); ++i){
            std::cout  << i << " " ;
            std::cout << (((qp.vec_element_[i]!= nullptr) && qp.vec_fint_[i]) ? *qp.vec_element_[i]:0);
            std::cout << " " << qp.vec_fint_[i] << std::endl;
        }
        return 1;
    }
    
   return 0;

}