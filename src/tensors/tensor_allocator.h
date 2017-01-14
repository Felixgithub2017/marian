#pragma once

// This file is part of the Marian toolkit.
// Marian is copyright (c) 2016 Marcin Junczys-Dowmunt.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <set>
#include <deque>

#include "definitions.h"
#include "tensors/tensor.h"

namespace marian {

class TensorAllocatorBase {
  public:
    virtual ~TensorAllocatorBase() {};
    virtual void reserve(size_t) = 0;
    virtual void reserveExact(size_t) = 0;
    virtual void clear() = 0;
    virtual void allocate(Tensor&, Shape) = 0;
    virtual void free(Tensor&) = 0;
    virtual size_t capacity() = 0;
    virtual size_t size() = 0;
    virtual Tensor asTensor() = 0;
};

template <class Device>
class TensorAllocatorDerived : public TensorAllocatorBase {
  private:
    const size_t CHUNK  = 512;
    const size_t MBYTE  = 1024 * 1024;
    const size_t FLOATS = CHUNK * MBYTE / sizeof(float);

    Device device_;

    typedef std::pair<size_t, float*> Gap;
    std::set<Gap> gaps_;
    Gap lastGap_;

    std::deque<Tensor> allocated_;

    void reset(Tensor t, float* start) {
      t->reset(start);
    }

    void resetAllocated(float* oldStart) {
      gaps_.clear();
      size_t prev = 0;
      for(auto&& t : allocated_) {
        size_t dist = t->data() - oldStart;
        reset(t, device_.data() + dist);
        if(dist > prev)
          gaps_.emplace(dist - prev, device_.data() + prev);
        prev = dist + t->size();
      }

      if(allocated_.empty()) {
        lastGap_ = { device_.capacity(), device_.data() };
      }
      else {
        size_t used = allocated_.back()->data() - device_.data() + allocated_.back()->size();
        size_t gap = device_.capacity() - used;
        float* addr = allocated_.back()->data() + allocated_.back()->size();
        lastGap_ = { gap, addr };
      }

      gaps_.insert(lastGap_);
    }

    auto getGap(Shape shape) -> decltype(gaps_.begin()) {
      auto it = std::lower_bound(gaps_.begin(), gaps_.end(),
                                 std::make_pair((size_t)shape.elements(), (float*)0));
      return it;
    }

    auto checkSpace(Shape shape) -> decltype(gaps_.begin()) {
      auto gapIt = getGap(shape);
      if(gapIt == gaps_.end()) {
        size_t incr = device_.capacity() - lastGap_.first + shape.elements();
        reserve(device_.capacity() + incr);
        gapIt = gaps_.find(lastGap_);
      }
      return gapIt;
    }

  public:
    TensorAllocatorDerived() {
      lastGap_ = { device_.capacity(), device_.data() };
      gaps_.insert(lastGap_);
    }

    void reserve(size_t elements = 0) {
      float mult = elements / FLOATS + 1;
      std::cerr << "Extending reserved space to " << mult * CHUNK << " MB" << std::endl;

      size_t old = device_.capacity();
      float* oldStart = device_.data();
      device_.reserve(mult * FLOATS);
      resetAllocated(oldStart);
    }

    void reserveExact(size_t elements = 0) {
      size_t mbytes = (elements * sizeof(float)) / MBYTE;
      std::cerr << "Reserving space for " << elements
        << " floats (" << mbytes << " MB)" << std::endl;

      size_t old = device_.capacity();
      float* oldStart = device_.data();
      device_.reserve(elements);
      resetAllocated(oldStart);
    }

    void clear() {
      gaps_.clear();
      lastGap_ = { device_.capacity(), device_.data() };
      gaps_.insert(lastGap_);
      allocated_.clear();
    }

    void allocate(Tensor &t, Shape shape) {
      if(!t || t->shape() != shape) {
        auto it = checkSpace(shape);
        float* start = it->second;
        t.reset(new typename Device::tensor_type(start, shape));
        allocated_.push_back(t);
        if(it->first > t->size())
          gaps_.emplace(it->first - t->size(), it->second + t->size());
        gaps_.erase(it);
      }
    }

    void free(Tensor& t) {
      //auto it = allocated_.rbegin();
      //while(it != allocated_.rend()) {
      //  if(*it == t) {
      //    Gap gap = { t->size(), t->data() };
      //    allocated_.erase(std::next(it).base());
      //
      //    auto it = gaps_.begin();
      //    while(it != gaps_.end()) {
      //      if(it->second + it->first  == gap.second) {
      //        gap = { gap.first + it->first, it->second };
      //      }
      //      if(gap.second + gap.first == it->second) {
      //        gap = { gap.first + it->first, gap.second };
      //      }
      //      it++;
      //    }
      //    gaps_.insert(gap);
      //    break;
      //  }
      //  it++;
      //}
      //t.reset();
    }

    Tensor asTensor() {
      float* start = device_.data();
      return Tensor(new typename Device::tensor_type(start, {1, (int)size()}));
    }

    size_t capacity() {
      return device_.capacity();
    }

    size_t size() {
      float* start = device_.data();
      float* end = start;
      if(!allocated_.empty())
        end = allocated_.back()->data() + allocated_.back()->size();

      return end - start;
    }
};

typedef std::shared_ptr<TensorAllocatorBase> TensorAllocator;

template <class Device>
TensorAllocator newTensorAllocator() {
  return TensorAllocator(new TensorAllocatorDerived<Device>());
}

}