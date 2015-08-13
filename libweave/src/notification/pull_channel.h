// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_SRC_NOTIFICATION_PULL_CHANNEL_H_
#define LIBWEAVE_SRC_NOTIFICATION_PULL_CHANNEL_H_

#include <memory>
#include <string>

#include <base/macros.h>
#include <base/memory/weak_ptr.h>
#include <base/single_thread_task_runner.h>
#include <base/timer/timer.h>
#include <weave/task_runner.h>

#include "libweave/src/notification/notification_channel.h"

namespace weave {

class PullChannel : public NotificationChannel {
 public:
  PullChannel(base::TimeDelta pull_interval, TaskRunner* task_runner);
  ~PullChannel() override = default;

  // Overrides from NotificationChannel.
  std::string GetName() const override;
  bool IsConnected() const override;
  void AddChannelParameters(base::DictionaryValue* channel_json) override;
  void Start(NotificationDelegate* delegate) override;
  void Stop() override;

  void UpdatePullInterval(base::TimeDelta pull_interval);

 private:
  void OnTimer();
  void RePost();

  base::TimeDelta pull_interval_;
  TaskRunner* task_runner_{nullptr};
  NotificationDelegate* delegate_{nullptr};

  base::WeakPtrFactory<PullChannel> weak_ptr_factory_{this};
  DISALLOW_COPY_AND_ASSIGN(PullChannel);
};

}  // namespace weave

#endif  // LIBWEAVE_SRC_NOTIFICATION_PULL_CHANNEL_H_
