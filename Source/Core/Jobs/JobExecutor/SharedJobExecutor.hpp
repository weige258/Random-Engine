#pragma once
#include <boost/lockfree/queue.hpp>
#include <atomic>
#include <vector>
#include <thread>
#include <unordered_set>
#include <mutex>
#include <type_traits>

namespace RandEngine::Core::Jobs::JobExecutor
{

    template <typename Task>
    class SharedJobExecutor
    {
    private:
        boost::lockfree::queue<Task, boost::lockfree::fixed_sized<false>> m_task_queue;
        std::vector<std::thread> m_threads;
        std::atomic<bool> is_running{false};
        std::atomic<bool> m_is_paused{false};

        std::unordered_set<Task> m_pending_removals;
        std::mutex m_removal_mutex;
        std::atomic<size_t> m_pending_removal_count{0};

        // 通用有效性检查 (兼容指针与自定义 ObserverPtr)
        static bool IsValid(const Task &task)
        {
            if constexpr (std::is_pointer_v<Task>)
                return task != nullptr;
            else
                return static_cast<bool>(task);
        }

    public:
        SharedJobExecutor(size_t capacity = 2048) : m_task_queue(capacity) {}
        virtual ~SharedJobExecutor() { Stop(); }

        SharedJobExecutor(const SharedJobExecutor &) = delete;
        SharedJobExecutor &operator=(const SharedJobExecutor &) = delete;

        void SetThreadCount(size_t thread_count)
        {
            if (thread_count == this->m_threads.size() || !this->m_threads.empty())
                return;

            size_t target_count = (thread_count < 1) ? 1 : thread_count;
            this->m_threads.reserve(target_count);

            for (size_t i = 0; i < target_count; ++i)
            {
                this->m_threads.emplace_back([this]()
                                             { WorkerLoop(); });
            }
        }

        bool PushTask(const Task &task)
        {
            if (!IsValid(task))
                return false;
            return m_task_queue.push(task);
        }

        bool RemoveTask(const Task &task)
        {
            if (!IsValid(task))
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

            for (auto &w : m_threads)
            {
                if (w.joinable())
                    w.join();
            }
            m_threads.clear();
        }

        void Pause() { m_is_paused.store(true, std::memory_order_release); }
        void Resume() { m_is_paused.store(false, std::memory_order_release); }
        bool IsPaused() const { return m_is_paused.load(std::memory_order_relaxed); }

    protected:
        virtual void ExecuteTask(Task task) = 0;

        void WorkerLoop()
        {
            while (is_running.load(std::memory_order_relaxed))
            {
                
                if (m_is_paused.load(std::memory_order_relaxed))
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    continue;
                }

                Task task{};
                if (m_task_queue.pop(task))
                {
                    if (IsValid(task))
                    {
                        bool is_removed = false;

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

                        if (!is_removed)
                        {
                            ExecuteTask(task);
                            m_task_queue.push(task);
                        }
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