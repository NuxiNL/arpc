#ifndef ARGDATA_READER_FILE_DESCRIPTOR_PARSER_H
#define ARGDATA_READER_FILE_DESCRIPTOR_PARSER_H

#include <arpc++/arpc++.h>

namespace arpc {
class ArgdataReaderFileDescriptorParser : public arpc::FileDescriptorParser {
 public:
  ArgdataReaderFileDescriptorParser(argdata_reader_t* reader)
      : reader_(reader) {
  }

  std::shared_ptr<FileDescriptor> Get(const argdata_t& ad) override;

 private:
  argdata_reader_t* const reader_;
};
}

#endif
