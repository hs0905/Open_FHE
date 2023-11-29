#ifndef CUSTOM_TASK_H
#define CUSTOM_TASK_H

#include <deque>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>
#include <unistd.h>

#define TASK_TYPE_PlainModMulEqScalar 1
#define TASK_TYPE_PlusScalar 2
#define TASK_TYPE_MinusScalar 3
#define TASK_TYPE_PlainModMulScalar 4
#define TASK_TYPE_Minus 5
#define TASK_TYPE_PlusInPlace 6
#define TASK_TYPE_MinusInPlace 7
#define TASK_TYPE_AutomorphismTransform 8
#define TASK_TYPE_SwitchModulus 9
#define TASK_TYPE_SwitchFormatInverseTransform 10
#define TASK_TYPE_SwitchFormatForwardTransform 11
#define TASK_TYPE_Plus 12
#define TASK_TYPE_Times 13
#define TASK_TYPE_TimesNoCheck 14
#define TASK_TYPE_TimesInPlace 15

struct CustomTaskItem {
  int task_type;
  
  void* poly;
  void* poly2;
  void* poly3;

  uint64_t param1;
  uint64_t param2;
  uint64_t param3;
  uint64_t param4;
  uint32_t* ptr32_1;

  std::atomic<bool> processed;

  CustomTaskItem(int t) : task_type(t), poly(NULL), poly2(NULL), poly3(NULL), param1(0), param2(0), param3(0), param4(0), ptr32_1(NULL), processed(false){}
};

class WorkQueue {
private:
    std::deque<CustomTaskItem*> queue;
    std::mutex mtx;

    bool finished = false;

    uint32_t num_parallel_jobs = 1;
    uint32_t num_parallel_jobs_synched = 0;

public:
    void addWork(CustomTaskItem* item) {
        {
            std::unique_lock<std::mutex> lock(mtx);        
            queue.push_back(item);
        }
    
        //Busy waiting until the work finishes..
        while (!item->processed.load(std::memory_order_acquire)) {
            std::this_thread::yield(); // Yield to reduce CPU usage
        }
    }

    void setNumParallelJobs(uint32_t _num_parallel_jobs) {
        std::unique_lock<std::mutex> lock(mtx);
        num_parallel_jobs = _num_parallel_jobs;
    }

    bool getWork(std::vector<CustomTaskItem*>& items) {
        std::unique_lock<std::mutex> lock(mtx);
        
        //Busy waiting until a job posted.
        //If num_parallel_jobs is greater than 1, wait for at least num_parallel_jobs tasks.
        while(1) {
            bool queue_ready = !queue.empty() && (num_parallel_jobs==1 || (num_parallel_jobs_synched!=0 || queue.size()>=num_parallel_jobs )); 
            
            if(queue_ready ||  finished) break;

            lock.unlock();
            std::this_thread::yield(); // Yield to reduce CPU usage
            lock.lock();
        }

        if (queue.empty()) return false;
        
        if(num_parallel_jobs > 1) {
            if(num_parallel_jobs_synched == 0) {
                num_parallel_jobs_synched = num_parallel_jobs;
            }
            num_parallel_jobs_synched --;
        }
    
        auto item = queue.front();
        queue.pop_front();        
        items.push_back(item);
        

        void* poly3 = item->poly3;
        int task_type = item->task_type;
        uint64_t modulus =  item->param3;

        // Iterate from the second element to the end to find tasks that can be processed together.
        for (auto it = queue.begin(); it != queue.end(); ++it) {
            if(num_parallel_jobs_synched == 0) break;

            void* o_poly3 = (*it)->poly3;

            bool param_match = false;
            if (poly3) {
                if (poly3 == o_poly3) param_match = true;
            }

            if(task_type == TASK_TYPE_SwitchFormatForwardTransform || task_type == TASK_TYPE_SwitchFormatInverseTransform) {
                if( modulus ==  (*it)->param3 ) param_match = true;
            }

            if (param_match)  {
                auto o_item = queue.front();
                queue.pop_front();
                items.push_back(o_item);

                num_parallel_jobs_synched --;
            }
        }

        return true;
    }

    void workProcessed(CustomTaskItem*& item) {
        item->processed.store(true, std::memory_order_release);
    }

    void finish() {
        std::unique_lock<std::mutex> lock(mtx);
        finished = true;
    }
};


#endif