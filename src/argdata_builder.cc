// Copyright (c) 2017 Nuxi (https://nuxi.nl/) and contributors.
//
// This file is distributed under a 2-clause BSD license.
// See the LICENSE file for details.

#include <memory>
#include <string_view>
#include <vector>

#include <arpc++/arpc++.h>
#include <argdata.hpp>

using namespace arpc;

const argdata_t* ArgdataBuilder::BuildFd(
    const std::shared_ptr<FileDescriptor>& value) {
  return argdatas_
      .emplace_back(
          argdata_create_fd(file_descriptors_.emplace_back(value)->get()))
      .get();
}

const argdata_t* ArgdataBuilder::BuildMap(
    std::vector<const argdata_t*> keys, std::vector<const argdata_t*> values) {
  std::size_t size = keys.size();
  return argdatas_
      .emplace_back(argdata_create_map(
          vectors_.emplace_front(std::move(keys)).data(),
          vectors_.emplace_front(std::move(values)).data(), size))
      .get();
}

const argdata_t* ArgdataBuilder::BuildSeq(
    std::vector<const argdata_t*> elements) {
  std::size_t size = elements.size();
  return argdatas_
      .emplace_back(argdata_create_seq(
          vectors_.emplace_front(std::move(elements)).data(), size))
      .get();
}

const argdata_t* ArgdataBuilder::BuildStr(std::string_view value) {
  std::size_t size = value.size();
  return argdatas_
      .emplace_back(
          argdata_create_str(strings_.emplace_front(value).data(), size))
      .get();
}
