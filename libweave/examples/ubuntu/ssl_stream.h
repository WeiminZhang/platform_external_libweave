// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_EXAMPLES_UBUNTU_SSL_STREAM_H_
#define LIBWEAVE_EXAMPLES_UBUNTU_SSL_STREAM_H_

#include <openssl/ssl.h>

#include <base/memory/weak_ptr.h>
#include <weave/stream.h>

namespace weave {

class TaskRunner;

namespace examples {

class SSLStream : public Stream {
 public:
  explicit SSLStream(TaskRunner* task_runner);

  ~SSLStream() override;

  void ReadAsync(
      void* buffer,
      size_t size_to_read,
      const base::Callback<void(size_t)>& success_callback,
      const base::Callback<void(const Error*)>& error_callback) override;

  void WriteAllAsync(
      const void* buffer,
      size_t size_to_write,
      const base::Closure& success_callback,
      const base::Callback<void(const Error*)>& error_callback) override;

  void CancelPendingAsyncOperations() override;

  bool Init(const std::string& host, uint16_t port);

 private:
  void RunDelayedTask(const base::Closure& success_callback);

  TaskRunner* task_runner_{nullptr};
  std::unique_ptr<SSL_CTX, decltype(&SSL_CTX_free)> ctx_{nullptr, SSL_CTX_free};
  std::unique_ptr<SSL, decltype(&SSL_free)> ssl_{nullptr, SSL_free};

  base::WeakPtrFactory<SSLStream> weak_ptr_factory_{this};
};

}  // namespace examples
}  // namespace weave

#endif  // LIBWEAVE_EXAMPLES_UBUNTU_SSL_STREAM_H_
