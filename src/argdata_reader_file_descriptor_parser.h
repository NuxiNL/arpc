#ifndef ARGDATA_READER_FILE_DESCRIPTOR_PARSER_H
#define ARGDATA_READER_FILE_DESCRIPTOR_PARSER_H

#include <memory>
#include <set>
#include <type_traits>

#include <arpc++/arpc++.h>

namespace arpc {
class ArgdataReaderFileDescriptorParser : public arpc::FileDescriptorParser {
 public:
  ArgdataReaderFileDescriptorParser(argdata_reader_t* reader)
      : reader_(reader) {
  }

  std::shared_ptr<FileDescriptor> Parse(const argdata_t& ad) override;

 private:
  // Comparator for finding file descriptors in a set of shared pointers
  // to FileDescriptor objects.
  class FileDescriptorComparator {
   public:
    using is_transparent = std::true_type;

    bool operator()(const std::shared_ptr<FileDescriptor>& a,
                    const std::shared_ptr<FileDescriptor>& b) {
      return a->get_fd() < b->get_fd();
    }
    bool operator()(int a, const std::shared_ptr<FileDescriptor>& b) {
      return a < b->get_fd();
    }
    bool operator()(const std::shared_ptr<FileDescriptor>& a, int b) {
      return a->get_fd() < b;
    }
  };

  argdata_reader_t* const reader_;
  std::set<std::shared_ptr<FileDescriptor>, FileDescriptorComparator>
      file_descriptors_;
};
}

#endif
