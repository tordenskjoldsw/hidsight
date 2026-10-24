// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#pragma once

#include "result.h"

#include <cstddef>
#include <memory>
#include <vector>

namespace hidpp::detail
{

template<typename T>
struct Sequence {
    std::size_t count = 0;
    std::function<void(std::size_t, Callback<T>)> readOne;
    Callback<std::vector<T>> done;
    std::vector<T> items;
};

template<typename T>
void continueSequence(const std::shared_ptr<Sequence<T>> &sequence)
{
    if (sequence->items.size() == sequence->count) {
        sequence->done(std::move(sequence->items));
        return;
    }
    sequence->readOne(sequence->items.size(), [sequence](Result<T> item) {
        if (!item) {
            sequence->done(item.error());
            return;
        }
        sequence->items.push_back(std::move(item).value());
        continueSequence(sequence);
    });
}

// Reads count items one request after another and hands over all of them, or
// the first error. Each step runs from the reply of the one before, so the
// stack does not grow with count.
template<typename T>
void readSequentially(std::size_t count, std::function<void(std::size_t, Callback<T>)> readOne, Callback<std::vector<T>> done)
{
    auto sequence = std::make_shared<Sequence<T>>();
    sequence->count = count;
    sequence->readOne = std::move(readOne);
    sequence->done = std::move(done);
    sequence->items.reserve(count);
    continueSequence(sequence);
}

} // namespace hidpp::detail
