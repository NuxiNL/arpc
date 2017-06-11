#include "argdata_reader_file_descriptor_parser.h"

using namespace arpc;

std::shared_ptr<FileDescriptor> ArgdataReaderFileDescriptorParser::Get(
    const argdata_t& ad) {
  // Parse file descriptor object.
  int fd;
  if (argdata_get_fd(&ad, &fd) != 0)
    return nullptr;

  // Return existing shared pointer instance.
  auto lookup = file_descriptors_.find(fd);
  if (lookup != file_descriptors_.end())
    return *lookup;

  // None found. Create new instance.
  argdata_reader_release_fd(reader_, fd);
  return *file_descriptors_.insert(std::make_shared<FileDescriptor>(fd)).first;
}
