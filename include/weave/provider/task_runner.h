// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_INCLUDE_WEAVE_PROVIDER_TASK_RUNNER_H_
#define LIBWEAVE_INCLUDE_WEAVE_PROVIDER_TASK_RUNNER_H_

#include <string>
#include <utility>
#include <vector>

#include <base/callback.h>
#include <base/location.h>
#include <base/time/time.h>

namespace weave {
namespace provider {

// This interface should be implemented by the user of libweave and
// provided during device creation in Device::Create(...)
// libweave will use this interface to schedule task execution.
//
// libweave is a single threaded library that uses task scheduling (similar
// to message loop) to perform asynchronous tasks. libweave itself does not
// implement message loop, and rely on user to provide implementation
// for its platform.
//
// Implementation of PostDelayedTask(...) should add specified task (callback)
// after all current tasks in the queue with delay=0. If there are tasks in the
// queue with delay > 0, new task may be scheduled before or between existing
// tasks. Position of the new task in the queue depends on remaining delay for
// existing tasks and specified delay. Normally, new task should be scheduled
// after the last task with "remaining delay" <= "new task delay". This will
// guarantee that all tasks with delay=0 will be executed in the same order
// they are put in the task queue.
//
// If delay is specified, task should be invoked no sooner then timeout is
// reached (it might be delayed due to other tasks in the queue).

// Interface with methods to post tasks into platform-specific message loop of
// the current thread.
class TaskRunner {
 public:
  // Posts tasks to be executed with the given delay.
  // |from_here| argument is used for debugging and usually just provided by
  // FROM_HERE macro. Implementation may ignore this argument.
  virtual void PostDelayedTask(const tracked_objects::Location& from_here,
                               const base::Closure& task,
                               base::TimeDelta delay) = 0;

 protected:
  virtual ~TaskRunner() {}
};

}  // namespace provider
}  // namespace weave

#endif  // LIBWEAVE_INCLUDE_WEAVE_PROVIDER_TASK_RUNNER_H_
