/* libui/src/periodic_tasks_scheduler.cc --
   Written and Copyright (C) 2018-2019 by vmc.

   This file is part of woinc.

   woinc is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   woinc is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with woinc. If not, see <http://www.gnu.org/licenses/>. */

#include "periodic_tasks_scheduler.h"

#include <algorithm>
#include <cassert>
#include <mutex>
#include <thread>

#ifndef NDEBUG
#include <iostream>
#endif

namespace woinc { namespace ui {

// --- PeriodicTasksSchedulerContext ---

PeriodicTasksSchedulerContext::PeriodicTasksSchedulerContext(const Configuration &config,
                                                             const HandlerRegistry &handler_registry)
    : handler_registry_(handler_registry), configuration_(config)
{}

void PeriodicTasksSchedulerContext::add_host(const std::string &host, HostController &controller) {
    std::lock_guard<decltype(lock_)> guard(lock_);
    tasks_.emplace(host, std::array<Task, 9> {
        Task(PeriodicTask::GET_CCSTATUS),
        Task(PeriodicTask::GET_CLIENT_STATE),
        Task(PeriodicTask::GET_DISK_USAGE),
        Task(PeriodicTask::GET_FILE_TRANSFERS),
        Task(PeriodicTask::GET_MESSAGES),
        Task(PeriodicTask::GET_NOTICES),
        Task(PeriodicTask::GET_PROJECT_STATUS),
        Task(PeriodicTask::GET_STATISTICS),
        Task(PeriodicTask::GET_TASKS)
    });
    host_controllers_.emplace(host, controller);
    states_.emplace(host, State());
}

void PeriodicTasksSchedulerContext::remove_host(const std::string &host) {
    std::lock_guard<decltype(lock_)> guard(lock_);
    tasks_.erase(host);
    host_controllers_.erase(host);
    states_.erase(host);
}

void PeriodicTasksSchedulerContext::reschedule_now(const std::string &host, PeriodicTask to_reschedule) {
    std::lock_guard<decltype(lock_)> guard(lock_);

    for (auto &task : tasks_.at(host)) {
        if (task.type == to_reschedule) {
            task.last_execution = std::chrono::steady_clock::time_point::min();
            condition_.notify_one();
            break;
        }
    }
}

void PeriodicTasksSchedulerContext::trigger_shutdown() {
    std::lock_guard<decltype(lock_)> guard(lock_);
    shutdown_triggered_ = true;
}

// --- PeriodicTasksScheduler ---

PeriodicTasksScheduler::PeriodicTasksScheduler(PeriodicTasksSchedulerContext &context)
    : context_(context)
{}

void PeriodicTasksScheduler::operator()() {
    using namespace std::chrono_literals;

    std::unique_lock<decltype(context_.lock_)> guard(context_.lock_);

    int cache_counter = 0;
    Configuration::Intervals intervals;

    while (!context_.shutdown_triggered_) {
        // update interval cache once a second
        if (cache_counter == 0)
            intervals = std::move(context_.configuration_.intervals());
        cache_counter = (cache_counter + 1) % 5;

        const auto now = std::chrono::steady_clock::now();

        for (auto &host_tasks : context_.tasks_) {
            if (!context_.configuration_.schedule_periodic_tasks(host_tasks.first))
                continue;
            for (auto &task : host_tasks.second)
                if (!task.pending && should_be_scheduled_(task, intervals, now))
                    schedule_(host_tasks.first, task);
        }

        context_.condition_.wait_for(guard, 200ms);
    }
}

void PeriodicTasksScheduler::handle_post_execution(const std::string &host, Job *j) {
    // we schedule and therefore register to periodic tasks only
    assert(dynamic_cast<PeriodicJob *>(j) != nullptr);

    PeriodicJob *job = static_cast<PeriodicJob *>(j);

    std::lock_guard<decltype(context_.lock_)> guard(context_.lock_);

    auto &tasks = context_.tasks_.at(host);
    auto task = std::find_if(tasks.begin(), tasks.end(), [&](const auto &t) {
        return t.type == job->task;
    });

    if (task != tasks.end()) {
        task->last_execution = std::chrono::steady_clock::now();
        task->pending = false;

        if (job->task == PeriodicTask::GET_MESSAGES)
            context_.states_.at(host).messages_seqno = job->payload.seqno;
        else if (job->task == PeriodicTask::GET_NOTICES)
            context_.states_.at(host).notices_seqno = job->payload.seqno;
    }
}

bool PeriodicTasksScheduler::should_be_scheduled_(const PeriodicTasksSchedulerContext::Task &task,
                                                  const Configuration::Intervals &intervals,
                                                  const decltype(PeriodicTasksSchedulerContext::Task::last_execution) &now) const {
    return now >= task.last_execution + std::chrono::seconds(intervals.at(static_cast<size_t>(task.type)));
}

void PeriodicTasksScheduler::schedule_(const std::string &host, PeriodicTasksSchedulerContext::Task &task) {
    task.pending = true;

    PeriodicJob::Payload payload;

    if (task.type == PeriodicTask::GET_MESSAGES)
        payload.seqno = context_.states_.at(host).messages_seqno;
    else if (task.type == PeriodicTask::GET_NOTICES)
        payload.seqno = context_.states_.at(host).notices_seqno;
    else if (task.type == PeriodicTask::GET_TASKS)
        payload.active_only = context_.configuration_.active_only_tasks(host);

    auto job = new PeriodicJob(task.type, context_.handler_registry_, payload);
    job->register_post_execution_handler(this);

    context_.host_controllers_.at(host).schedule(job);
}

}}
