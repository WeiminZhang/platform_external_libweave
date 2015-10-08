// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_SRC_STREAMS_H_
#define LIBWEAVE_SRC_STREAMS_H_

#include <base/memory/weak_ptr.h>
#include <weave/stream.h>

namespace weave {

namespace provider {
class TaskRunner;
}

class MemoryStream : public InputStream, public OutputStream {
 public:
  MemoryStream(const std::vector<uint8_t>& data,
               provider::TaskRunner* task_runner);

  void Read(void* buffer,
            size_t size_to_read,
            const ReadSuccessCallback& success_callback,
            const ErrorCallback& error_callback) override;

  void Write(const void* buffer,
             size_t size_to_write,
             const SuccessCallback& success_callback,
             const ErrorCallback& error_callback) override;

  const std::vector<uint8_t>& GetData() const { return data_; }

 private:
  std::vector<uint8_t> data_;
  provider::TaskRunner* task_runner_{nullptr};
  size_t read_position_{0};
};

class StreamCopier {
 public:
  StreamCopier(InputStream* source, OutputStream* destination);

  void Copy(const InputStream::ReadSuccessCallback& success_callback,
            const ErrorCallback& error_callback);

 private:
  void OnSuccessRead(const InputStream::ReadSuccessCallback& success_callback,
                     const ErrorCallback& error_callback,
                     size_t size);

  InputStream* source_{nullptr};
  OutputStream* destination_{nullptr};

  size_t size_done_{0};
  std::vector<uint8_t> buffer_;

  base::WeakPtrFactory<StreamCopier> weak_ptr_factory_{this};
};

}  // namespace weave

#endif  // LIBWEAVE_SRC_STREAMS_H_
