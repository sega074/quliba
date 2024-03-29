/* Пример использования класса QPoint

sega074@yandex.ru (Sergey Dmitrievich Gordeychuk)

*/

#include <vector>
#include <memory>
#include <atomic>
#include <thread>
#include <future>
#include <numeric>
#include <iostream>
#include <chrono>
#include <string>
#include "QPoints.h"


#define ELEMENT_1 12345678
#define ELEMENT_2 54321000
#define ELEMENT_3 10000333


int main(int argc, char** argv) {

    constexpr uint32_t try_atm_q = 10;

    QPoints <uint64_t, 32> qsm(try_atm_q);
   
    std::atomic<uint64_t> count1{0};
    std::atomic<uint64_t> count2{0};
    std::atomic<uint64_t> count3{0};

    std::atomic<uint64_t> ucount1{0};
    std::atomic<uint64_t> ucount2{0};
    std::atomic<uint64_t> ucount3{0};


    uint64_t c_count;   // количистов операций записи и чтения
    uint32_t s_count;   // количистов потоков для отправки
    uint32_t r_count;   // количестов потоков для приема

    volatile bool bl_end = false;

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

    std::vector <std::thread> thread_send;  // вектор потоков для отправки 
    std::vector <std::thread> thread_recv;  // вектор потоков для приема

/**/
    uint64_t * element_1 = new uint64_t;
    *element_1 = ELEMENT_1;
    uint64_t * element_2 = new uint64_t;
    *element_2 = ELEMENT_2;
    uint64_t * element_3 = new uint64_t;
    *element_3 = ELEMENT_3;

/**/
    auto thread_send_qsm = [&](){   
        uint64_t ste = 1;
        uint32_t tp_el =0;  // тип элемента 0,1,2
        while (count1 < c_count  || count2 < c_count || count3 < c_count){
            int id = qsm.idForAdd();
            uint64_t* el;
            if (id == -1){
                std::this_thread::sleep_for(std::chrono::microseconds(ste));
                if (++ste >= r_count ){ste =1;}
                continue;
            }
            if (tp_el == 0 && count1 < c_count){

                el =  qsm.addEl(id,element_1);

                if ( el == nullptr ){
                    count1++;
                } else {
                    continue;
                }
            } else if (tp_el == 1 && count2 < c_count){

                el =  qsm.addEl(id,element_2);
            
                if ( el == nullptr ){
                    count2++;
                } else {
                    continue;
                }
            } else if (tp_el == 2 && count3 < c_count){

                el =  qsm.addEl(id,element_3);

                if ( el == nullptr ){
                    count3++;
                } else {
                    continue;
                }
            }
            ste = 1;
            if (++tp_el > 2 ){ tp_el = 0;}
        }
    };
/**/
    for (int i = 0 ; i < s_count; ++i){     
            thread_send.emplace_back(thread_send_qsm);
    }
/* RECV */
    auto thread_recv_qsm  = [&](){
        int st = 0;
        uint64_t ste= 1;
        while ((!bl_end) || (qsm.elInQueue() > 0) || (st < try_atm_q))  {
                int id = qsm.idForGet();
                uint64_t* ret;
                if (id == -1){
                    std::this_thread::sleep_for(std::chrono::microseconds(ste));

                    if (++ste >= r_count ){ste =1;}
                        if(bl_end && qsm.elInQueue() > 0) {
                        std::cout << "bl_end == true & elInQueue() "<< qsm.elInQueue() <<std::endl;
                    }
                    if (bl_end && (qsm.elInQueue() == 0))st++;
                        continue;
                }

                ret =  qsm.getEl(id);

                if (ret != nullptr){
                    if (*ret == ELEMENT_1)      {ucount1++;}
                    else if (*ret == ELEMENT_2) {ucount2++;}
                    else if (*ret == ELEMENT_3) {ucount3++;}
                    else {
                        std::cout << "err get X elem: "<< std::hex << ret << std::dec <<std::endl;
                    }
                } else {
                    std::this_thread::sleep_for(std::chrono::microseconds(ste));
                    ste++;
                }
                if (ste >= r_count *10 ){
                    ste =1;
                }
                if(bl_end && qsm.elInQueue() > 0) {
                    std::cout << "bl_end == true & elInQueue()"<< qsm.elInQueue() <<std::endl;
                }
                if (bl_end && (qsm.elInQueue() == 0))st++;
        }
        
    };
/**/
    for (int i = 0 ; i < r_count; ++i){     
            thread_recv.emplace_back(thread_recv_qsm);
    }
/**/

    for (int i = 0 ; i < s_count ; ++i){    
        thread_send[i].join();
    }

    bl_end= true;   

    for (int i = 0 ; i < r_count; ++i){
        thread_recv[i].join();
    }

    std::cout << std::dec << std::endl << std::endl;
    std::cout << std::endl;    
    std::cout << "send count1 " << count1      << " receive ucont1 " << ucount1 << std::endl;
    std::cout << "send count2 " << count2      << " receive ucont2 " << ucount2 << std::endl;
    std::cout << "send count3 " << count3      << " receive ucont3 " << ucount3 << std::endl;

    if ( count1  != ucount1 || count2 != ucount2 || count3 != ucount3 ){
        std::tuple<uint16_t, uint16_t> point = qsm.getBegEnd();
        std::cout << "beg    " << std::get<0>(point) << " end    " << std::get<1>(point) << std::endl; 

        for (int i = 0 ; i < qsm.lenQueue(); ++i){
            uint64_t *el = qsm.getEl(i);
            if (el != nullptr){
                std::cout << std::dec << i << " " << std::hex << *el << std::endl;
            }

        }
        return 1;
    }
    
   return 0;
}
