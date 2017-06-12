#include "argdata_reader_file_descriptor_parser.h"

using namespace arpc;

ArgdataReaderFileDescriptorParser::~ArgdataReaderFileDescriptorParser() {
  // File descriptors that have been parsed are now owned by the
  // messages containing them. Allow the reader to close any file
  // descriptors attached to the message, except those that have been
  // handed out by us.
  for (const auto& file_descriptor : file_descriptors_)
    argdata_reader_release_fd(reader_, file_descriptor->get_fd());
}

std::shared_ptr<FileDescriptor> ArgdataReaderFileDescriptorParser::Parse(
    const argdata_t& ad) {
  // Parse file descriptor object.
  int fd;
  if (argdata_get_fd(&ad, &fd) != 0)
    return nullptr;

  // Try to return an existing shared pointer instance. Create a new
  // instance if none exists.
  auto lookup = file_descriptors_.find(fd);
  if (lookup != file_descriptors_.end())
    return *lookup;
  return *file_descriptors_.insert(std::make_shared<FileDescriptor>(fd)).first;
}
