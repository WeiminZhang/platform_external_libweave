// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "libweave/examples/ubuntu/ssl_stream.h"

#include <base/bind.h>
#include <weave/task_runner.h>

namespace weave {
namespace examples {

SSLStream::SSLStream(TaskRunner* task_runner) : task_runner_{task_runner} {}

SSLStream::~SSLStream() {
  CancelPendingOperations();
}

void SSLStream::RunDelayedTask(const base::Closure& success_callback) {
  success_callback.Run();
}

void SSLStream::Read(
    void* buffer,
    size_t size_to_read,
    const base::Callback<void(size_t)>& success_callback,
    const base::Callback<void(const Error*)>& error_callback) {
  int res = SSL_read(ssl_.get(), buffer, size_to_read);
  if (res > 0) {
    task_runner_->PostDelayedTask(
        FROM_HERE,
        base::Bind(&SSLStream::RunDelayedTask, weak_ptr_factory_.GetWeakPtr(),
                   base::Bind(success_callback, res)),
        {});
    return;
  }

  int err = SSL_get_error(ssl_.get(), res);

  if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
    task_runner_->PostDelayedTask(
        FROM_HERE,
        base::Bind(&SSLStream::Read, weak_ptr_factory_.GetWeakPtr(),
                   buffer, size_to_read, success_callback, error_callback),
        base::TimeDelta::FromSeconds(1));
    return;
  }

  ErrorPtr weave_error;
  Error::AddTo(&weave_error, FROM_HERE, "ssl", "socket_read_failed",
               "SSL error");
  task_runner_->PostDelayedTask(
      FROM_HERE,
      base::Bind(
          &SSLStream::RunDelayedTask, weak_ptr_factory_.GetWeakPtr(),
          base::Bind(error_callback, base::Owned(weave_error.release()))),
      {});
  return;
}

void SSLStream::Write(
    const void* buffer,
    size_t size_to_write,
    const base::Closure& success_callback,
    const base::Callback<void(const Error*)>& error_callback) {
  int res = SSL_write(ssl_.get(), buffer, size_to_write);
  if (res > 0) {
    buffer = static_cast<const char*>(buffer) + res;
    size_to_write -= res;
    if (size_to_write == 0) {
      task_runner_->PostDelayedTask(
          FROM_HERE,
          base::Bind(&SSLStream::RunDelayedTask, weak_ptr_factory_.GetWeakPtr(),
                     success_callback),
          {});
      return;
    }

    task_runner_->PostDelayedTask(
        FROM_HERE,
        base::Bind(&SSLStream::Write, weak_ptr_factory_.GetWeakPtr(),
                   buffer, size_to_write, success_callback, error_callback),
        base::TimeDelta::FromSeconds(1));

    return;
  }

  int err = SSL_get_error(ssl_.get(), res);

  if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
    task_runner_->PostDelayedTask(
        FROM_HERE,
        base::Bind(&SSLStream::Write, weak_ptr_factory_.GetWeakPtr(),
                   buffer, size_to_write, success_callback, error_callback),
        base::TimeDelta::FromSeconds(1));
    return;
  }

  ErrorPtr weave_error;
  Error::AddTo(&weave_error, FROM_HERE, "ssl", "socket_write_failed",
               "SSL error");
  task_runner_->PostDelayedTask(
      FROM_HERE,
      base::Bind(
          &SSLStream::RunDelayedTask, weak_ptr_factory_.GetWeakPtr(),
          base::Bind(error_callback, base::Owned(weave_error.release()))),
      {});
  return;
}

void SSLStream::CancelPendingOperations() {
  weak_ptr_factory_.InvalidateWeakPtrs();
}

bool SSLStream::Init(const std::string& host, uint16_t port) {
  ctx_.reset(SSL_CTX_new(TLSv1_2_client_method()));
  CHECK(ctx_);
  ssl_.reset(SSL_new(ctx_.get()));

  char end_point[255];
  snprintf(end_point, sizeof(end_point), "%s:%u", host.c_str(), port);
  BIO* stream_bio = BIO_new_connect(end_point);
  CHECK(stream_bio);
  BIO_set_nbio(stream_bio, 1);

  while (BIO_do_connect(stream_bio) != 1) {
    CHECK(BIO_should_retry(stream_bio));
    sleep(1);
  }

  SSL_set_bio(ssl_.get(), stream_bio, stream_bio);
  SSL_set_connect_state(ssl_.get());

  for (;;) {
    int res = SSL_do_handshake(ssl_.get());
    if (res) {
      return true;
    }

    res = SSL_get_error(ssl_.get(), res);

    if (res != SSL_ERROR_WANT_READ || res != SSL_ERROR_WANT_WRITE) {
      return false;
    }

    sleep(1);
  }
  return false;
}

}  // namespace examples
}  // namespace weave
