#pragma once
#include <boost/lockfree/queue.hpp>
#include <atomic>
#include <vector>
#include <thread>
#include <unordered_set>
#include <mutex>

namespace RandEngine::Core::Job
{
    template <typename Task>
    class SharedJobWorker
    {
    private:
        boost::lockfree::queue<Task *, boost::lockfree::fixed_sized<false>> task_queue;
        std::vector<std::thread> threads;
        std::atomic<bool> is_running{false};

        std::unordered_set<Task *> m_pending_removals;
        std::mutex m_removal_mutex;
        std::atomic<size_t> m_pending_removal_count{0};

    public:
        SharedJobWorker(size_t capacity = 2048) : task_queue(capacity) {}
        virtual ~SharedJobWorker() { Stop(); }

        SharedJobWorker(const SharedJobWorker &) = delete;
        SharedJobWorker &operator=(const SharedJobWorker &) = delete;

        void SetThreadCount(size_t thread_count)
        {
            if (thread_count == this->threads.size())
                return;
            if (!this->threads.empty())
                return; // 启动后固定线程数，避免重复创建

            size_t target_count = (thread_count < 1) ? 1 : thread_count;
            this->threads.reserve(target_count);

            for (size_t i = 0; i < target_count; ++i)
            {
                this->threads.emplace_back([this]()
                                           { WorkerLoop(); });
            }
        }

        // 添加task
        bool PushTask(Task *task)
        {
            if (!task)
                return false;
            return task_queue.push(task);
        }

        // 单参数 Task 删除：标记墓碑（Lazy Deletion）
        bool RemoveTask(Task *task)
        {
            if (!task)
                return false;

            {
                std::lock_guard<std::mutex> lock(m_removal_mutex);
                m_pending_removals.insert(task);
            }
            m_pending_removal_count.fetch_add(1, std::memory_order_relaxed);
            return true;
        }

        void Start(size_t thread_count)
        {
            if (is_running.load(std::memory_order_relaxed))
                return;

            is_running.store(true, std::memory_order_release);
            SetThreadCount(thread_count);
        }

        void Stop()
        {
            if (!is_running.exchange(false, std::memory_order_acq_rel))
                return;

            for (auto &w : threads)
            {
                if (w.joinable())
                    w.join();
            }
            threads.clear();
        }

    protected:
        virtual void ExecuteTask(Task *task) = 0;

        void WorkerLoop()
        {
            while (is_running.load(std::memory_order_relaxed))
            {
                Task *task = nullptr;
                if (task_queue.pop(task))
                {
                    if (task)
                    {
                        bool is_removed = false;

                        // 1. 快通道判断：若无待删除任务，直接跳过锁检测（开销仅为一次 Relaxed Atomic Load）
                        if (m_pending_removal_count.load(std::memory_order_relaxed) > 0)
                        {
                            std::lock_guard<std::mutex> lock(m_removal_mutex);
                            auto it = m_pending_removals.find(task);
                            if (it != m_pending_removals.end())
                            {
                                m_pending_removals.erase(it);
                                m_pending_removal_count.fetch_sub(1, std::memory_order_relaxed);
                                is_removed = true;
                            }
                        }

                        // 2. 未被删除才执行并重新推回队列
                        if (!is_removed)
                        {
                            ExecuteTask(task);
                            task_queue.push(task); // 重新推回共享无锁队列
                        }
                        // 若已被删除，则直接放任其丢弃，终止轮转
                    }
                }
                else
                {
                    std::this_thread::yield();
                }
            }
        }
    };
}