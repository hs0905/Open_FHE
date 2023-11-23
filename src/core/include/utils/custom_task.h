#ifndef CUSTOM_TASK_H
#define CUSTOM_TASK_H

#include <queue>
#include <mutex>
#include <condition_variable>

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
#define TASK_TYPE_TimesInPlace 14

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

  std::shared_ptr<uint64_t> vector_op0;

  bool processed;
  std::condition_variable cv;

  CustomTaskItem(int t) : task_type(t), processed(false) {}
};

class WorkQueue {
private:
    std::queue<CustomTaskItem*> queue;
    std::mutex mtx;
    std::condition_variable cv_consumer;
    bool finished = false;

public:
    void addWork(CustomTaskItem* item) {
        std::unique_lock<std::mutex> lock(mtx);
        queue.push(item);
        cv_consumer.notify_one();

        // Wait for the consumer to process the item
        item->cv.wait(lock, [item](){ return item->processed; });
    }

    bool getWork(CustomTaskItem*& item) {
        std::unique_lock<std::mutex> lock(mtx);
        cv_consumer.wait(lock, [this](){ return !queue.empty() || finished; });

        if (queue.empty()) return false;

        // std::cout << "Q:" <<queue.size() <<std::endl;

        item = queue.front();
        queue.pop();
        return true;
    }

    void workProcessed(CustomTaskItem*& item) {
        {
          std::lock_guard<std::mutex> lock(mtx);
          item->processed = true;
        }
        item->cv.notify_one();
    }

    void finish() {
        std::unique_lock<std::mutex> lock(mtx);
        finished = true;
        cv_consumer.notify_all();
    }
};


#endif