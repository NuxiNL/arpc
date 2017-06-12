#include <memory>
#include <string_view>
#include <vector>

#include <arpc++/arpc++.h>
#include <argdata.hpp>

#include "argdata_reader_file_descriptor_parser.h"

using namespace arpc;

const argdata_t* ArgdataStore::CreateFileDescriptor(
    const std::shared_ptr<FileDescriptor>& value) {
  return argdatas_
      .emplace_back(
          argdata_create_fd(file_descriptors_.emplace_back(value)->get_fd()))
      .get();
}

const argdata_t* ArgdataStore::CreateMap(std::vector<const argdata_t*> keys,
                                         std::vector<const argdata_t*> values) {
  std::size_t size = keys.size();
  return argdatas_
      .emplace_back(argdata_create_map(
          vectors_.emplace_front(std::move(keys)).data(),
          vectors_.emplace_front(std::move(values)).data(), size))
      .get();
}

const argdata_t* ArgdataStore::CreateString(std::string_view value) {
  std::size_t size = value.size();
  return argdatas_
      .emplace_back(
          argdata_create_str(strings_.emplace_front(value).data(), size))
      .get();
}
