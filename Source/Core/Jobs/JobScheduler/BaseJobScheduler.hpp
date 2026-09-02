#pragma once
#include <cstddef>
#include <type_traits>

namespace RandEngine::Core::Jobs::Executor { class BaseJobExecutor; }

namespace RandEngine::Core::Jobs::Scheduler
{
    class BaseJobScheduler
    {
    protected:
        // 统一持有顶层 Executor 的引用，用于获取全局上下文（如 Worker 总数、线程组信息等）
        Executor::BaseJobExecutor* m_executor = nullptr;

    public:
        BaseJobScheduler() = default;
        virtual ~BaseJobScheduler() = default;

        // 禁用拷贝与赋值
        BaseJobScheduler(const BaseJobScheduler&) = delete;
        BaseJobScheduler& operator=(const BaseJobScheduler&) = delete;

        // 绑定所属的 Executor
        void BindExecutor(Executor::BaseJobExecutor* executor) { m_executor = executor; }

        // =========================================================
        // 1. 任务分发通用接口 (如 TaskDistributorScheduler 使用)
        // =========================================================
        
        // 提交一个抽象 Task 到调度器，具体分发给哪个 Worker 由子类策略决定
        virtual bool DispatchTask(void* task_ptr, int target_worker_idx = -1) = 0;

        // 等待当前调度器管理的所有任务结束 (帧同步 Barrier/Semaphore)
        virtual void WaitForAll() = 0;

        // =========================================================
        // 2. Worker 反向生命周期/工作步回调 (虚拟钩子 Virtual Hooks)
        // =========================================================
        
        // 线程启动/退出 (用于初始化线程局部变量或负载均衡统计)
        virtual void OnWorkerStart(size_t worker_index) {}
        virtual void OnWorkerStop(size_t worker_index) {}

        // 循环开始/结束 (用于 LoadBalanceScheduler 抓取各 Worker 负载快照)
        virtual void OnWorkerLoopStart(size_t worker_index) {}
        virtual void OnWorkerLoopEnd(size_t worker_index) {}

        // 单个 Task 执行完成触发 (用于 TaskDistributor 解锁 DAG 或递减信号量计数)
        virtual void OnTaskCompleted(size_t worker_index, void* task_ptr) {}
    };
}