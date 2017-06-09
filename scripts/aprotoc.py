#!/usr/bin/env python3.4

import hashlib
import pypeg2
import re
import sys
import toposort


# TODO(ed): Fill in more language keywords.
FORBIDDEN_WORDS = {'namespace'}


class ScalarType:

    def get_dependencies(self):
        return set()

    def print_fields(self, name, declarations):
        print('  %s %s_;' % (self.get_storage_type(declarations), name))


class NumericType(ScalarType):

    def get_initializer(self, name, declarations):
        return '%s_(%s)' % (name, self.get_default_value())

    def get_return_type(self, declarations):
        return self.get_storage_type(declarations)

    def print_accessors(self, name, declarations):
        print('  %s %s() const { return %s_; }' % (self.get_storage_type(declarations), name, name))
        print('  void set_%s(%s value) { %s_ = value; }' % (name, self.get_storage_type(declarations), name))
        print('  void clear_%s() { %s_ = %s; }' % (name, name, self.get_default_value()))

    def print_accessors_repeated(self, name, declarations):
        print('  %s %s(std::size_t index) const { return %s_[index]; }' % (self.get_storage_type(declarations), name, name))
        print('  void set_%s(std::size_t index, %s value) { %s_[index] = value; }' % (name, self.get_storage_type(declarations), name))
        print('  void add_%s(%s value) { %s_.push_back(value); }' % (name, self.get_storage_type(declarations), name))


class IntegerType(NumericType):

    def __init__(self, name):
        self._name = name

    def get_default_value(self):
        return '0'

    def get_storage_type(self, declarations):
        return 'std::%s_t' % self._name

    def print_parsing(self, name, declarations):
        print('        argdata_get_int(value, &%s_);' % name)

    def print_parsing_map_key(self):
        print('          std::%s_t mapkey;' % self._name)
        print('          if (argdata_get_int(value2, &mapkey) != 0)')
        print('            continue;')

    def print_parsing_map_value(self, name, declarations):
        print('          std::%s_t key2int;' % self._name);
        print('          if (argdata_get_int(key2, &key2int) == 0)')
        print('            %s_.emplace(mapkey, 0).first->second = key2int;' % name)

    def print_parsing_repeated(self, name, declarations):
        print('          std::%s_t elementint;' % self._name)
        print('          if (argdata_get_int(element, &elementint) == 0)')
        print('            %s_.push_back(elementint);' % name)


class Int32Type(IntegerType):

    grammar = ['int32', 'sint32', 'sfixed32']

    def __init__(self):
        super(Int32Type, self).__init__('int32')


class UInt32Type(IntegerType):

    grammar = ['uint32', 'fixed32']

    def __init__(self):
        super(UInt32Type, self).__init__('uint32')


class Int64Type(IntegerType):

    grammar = ['int64', 'sint64', 'sfixed64']

    def __init__(self):
        super(Int64Type, self).__init__('int64')


class UInt64Type(IntegerType):

    grammar = ['uint64', 'fixed64']

    def __init__(self):
        super(UInt64Type, self).__init__('uint64')


class FloatingPointType(NumericType):

    grammar = ['double', 'float']

    def get_default_value(self):
        return '0.0'

    def get_storage_type(self, declarations):
        return 'double'

    def print_parsing(self, name, declarations):
        print('          argdata_get_float(value, &%s_);' % name)


class BooleanType(NumericType):

    grammar = ['bool']

    def get_default_value(self):
        return 'false'

    def get_storage_type(self, declarations):
        return 'bool'

    def print_parsing(self, name, declarations):
        print('        argdata_get_bool(value, &%s_);' % name)


class StringlikeType(ScalarType):

    def get_initializer(self, name, declarations):
        return ''

    def get_return_type(self, declarations):
        return 'const std::string&'

    def get_storage_type(self, declarations):
        return 'std::string'

    def print_accessors(self, name, declarations):
        print('  const std::string& %s() const { return %s_; }' % (name, name))
        print('  void set_%s(std::string_view value) { %s_ = value; }' % (name, name))
        print('  std::string* mutable_%s() { return &%s_; }' % (name, name))
        print('  void clear_%s() { %s_.clear(); }' % (name, name))

    def print_accessors_repeated(self, name, declarations):
        print('  const std::string& %s(std::size_t index) const { return %s_[index]; }' % (name, name))
        print('  void set_%s(std::size_t index, std::string_view value) { %s_[index] = value; }' % (name, name))
        print('  std::string* mutable_%s(std::size_t index) { return &%s_[index]; }' % (name, name))
        print('  void add_%s(std::string_view value) { %s_.emplace_back(value); }' % (name, name))
        print('  std::string* add_%s() { return &%s_.emplace_back(); }' % (name, name))


class StringType(StringlikeType):

    grammar = ['string']

    def print_parsing(self, name, declarations):
        print('        const char* valuestr;');
        print('        std::size_t valuelen;');
        print('        if (argdata_get_str(value, &valuestr, &valuelen) == 0)')
        print('          %s_ = std::string_view(valuestr, valuelen);' % name)

    def print_parsing_map_key(self):
        print('          const char* value2str;');
        print('          std::size_t value2len;');
        print('          if (argdata_get_str(value2, &value2str, &value2len) != 0)')
        print('            continue;')
        print('          std::string_view mapkey(value2str, value2len);')

    def print_parsing_map_value(self, name, declarations):
        print('          const char* key2str;');
        print('          std::size_t key2len;');
        print('          if (argdata_get_str(key2, &key2str, &key2len) == 0)')
        print('            %s_.emplace(mapkey, std::string()).first->second = std::string_view(key2str, key2len);' % name)

    def print_parsing_repeated(self, name, declarations):
        print('          const char* elementstr;');
        print('          std::size_t elementlen;');
        print('          if (argdata_get_str(element, &elementstr, &elementlen) == 0)')
        print('            %s_.emplace_back(std::string_view(elementstr, elementlen));' % name)


class BytesType(StringlikeType):

    grammar = ['bytes']

    def print_parsing(self, name, declarations):
        print('        const void* valuestr;');
        print('        std::size_t valuelen;');
        print('        if (argdata_get_binary(value, &valuestr, &valuelen) == 0)')
        print('          %s_ = std::string_view(static_cast<const char*>(valuestr), valuelen);' % name)


class ReferenceType:

    grammar = pypeg2.word

    def __init__(self, name):
        self._name = name

    def get_dependencies(self):
        return {self._name}

    def get_initializer(self, name, declarations):
        return declarations[self._name].get_initializer(name)

    def get_storage_type(self, declarations):
        return self._name

    def get_return_type(self, declarations):
        return declarations[self._name].get_return_type()

    def is_stream(self):
        return False

    def print_accessors(self, name, declarations):
        declarations[self._name].print_accessors(name)

    def print_accessors_repeated(self, name, declarations):
        declarations[self._name].print_accessors_repeated(name)

    def print_fields(self, name, declarations):
        declarations[self._name].print_fields(name)

    def print_parsing(self, name, declarations):
        declarations[self._name].print_parsing(name)

    def print_parsing_map_value(self, name, declarations):
        declarations[self._name].print_parsing_map_value(name)

    def print_parsing_repeated(self, name, declarations):
        declarations[self._name].print_parsing_repeated(name)


PrimitiveType = [
    Int32Type,
    UInt32Type,
    Int64Type,
    UInt64Type,
    FloatingPointType,
    BooleanType,
    StringType,
    BytesType,
    ReferenceType,
]


class MapType:

    grammar = 'map', '<', [
        Int32Type,
        UInt32Type,
        Int64Type,
        UInt64Type,
        BooleanType,
        StringType,
    ], ',', PrimitiveType, '>'

    def __init__(self, arguments):
        self._key_type = arguments[0]
        self._value_type = arguments[1]

    def get_dependencies(self):
        return (self._key_type.get_dependencies() |
                self._value_type.get_dependencies())

    def get_initializer(self, name, declarations):
        return ''

    def get_storage_type(self, declarations):
        return 'std::map<%s, %s, std::less<>>' % (self._key_type.get_storage_type(declarations),
                                                  self._value_type.get_storage_type(declarations))

    def print_accessors(self, name, declarations):
        print('  const %s& %s() const { return %s_; }' % (self.get_storage_type(declarations), name, name))
        print('  %s* mutable_%s() { return &%s_; }' % (self.get_storage_type(declarations), name, name))

    def print_fields(self, name, declarations):
        print('  %s %s_;' % (self.get_storage_type(declarations), name))

    def print_parsing(self, name, declarations):
        print('        argdata_map_iterator_t it2;')
        print('        argdata_map_iterate(value, &it2);')
        print('        const argdata_t* key2, *value2;')
        print('        while (argdata_map_next(&it2, &key2, &value2)) {')
        self._key_type.print_parsing_map_key()
        self._value_type.print_parsing_map_value(name, declarations)
        print('        }')


class RepeatedType:

    grammar = 'repeated', PrimitiveType

    def __init__(self, type):
        self._type = type

    def get_dependencies(self):
        return self._type.get_dependencies()

    def get_initializer(self, name, declarations):
        return ''

    def get_storage_type(self, declarations):
        return 'std::vector<%s>' % self._type.get_storage_type(declarations)

    def print_accessors(self, name, declarations):
        print('  std::size_t %s_size() const { return %s_.size(); }' % (name, name))
        self._type.print_accessors_repeated(name, declarations)
        print('  void clear_%s() { %s_.clear(); }' % (name, name))
        print('  const %s& %s() const { return %s_; }' % (self.get_storage_type(declarations), name, name))
        print('  %s* %s() { return &%s_; }' % (self.get_storage_type(declarations), name, name))

    def print_fields(self, name, declarations):
        print('  %s %s_;' % (self.get_storage_type(declarations), name))

    def print_parsing(self, name, declarations):
        print('        argdata_seq_iterator_t it2;')
        print('        argdata_seq_iterate(value, &it2);')
        print('        const argdata_t* element;')
        print('        while (argdata_seq_next(&it2, &element)) {')
        self._type.print_parsing_repeated(name, declarations)
        print('        }')


class StreamType:

    grammar = 'stream', ReferenceType

    def __init__(self, type):
        self._type = type

    def get_dependencies(self):
        return self._type.get_dependencies()

    def get_storage_type(self, declarations):
        return self._type.get_storage_type(declarations)

    def is_stream(self):
        return True


class EnumDeclaration:

    grammar = 'enum', pypeg2.word, '{', pypeg2.some(
        pypeg2.word, '=', re.compile(r'\d+'), ';'
    ), '}'

    def __init__(self, arguments):
        self._name = arguments[0]
        self._constants = {}
        self._canonical = {}
        for i in range(1, len(arguments), 2):
            key = arguments[i]
            value = int(arguments[i + 1])
            self._constants[key] = value
            if value not in self._canonical:
                self._canonical[value] = key

    def get_dependencies(self):
        return set()

    def get_initializer(self, name):
        return '%s_(%s::%s)' % (name, self._name, self._canonical[0])

    def get_name(self):
        return self._name

    def get_return_type(self):
        return self._name

    def print_accessors(self, name):
        print('  %s %s() const { return %s_; }' % (self._name, name, name))
        print('  void set_%s(%s value) { %s_ = value; }' % (name, self._name, name))
        print('  void clear_%s() { %s_ = %s::%s; }' % (name, name, self._name, self._canonical[0]))

    def print_accessors_repeated(self, name):
        print('  %s %s(std::size_t index) const { return %s_[index]; }' % (self._name, name, name))
        print('  void set_%s(std::size_t index, %s value) { %s_[index] = value; }' % (name, self._name, name))
        print('  void add_%s(%s value) { return %s_.push_back(value); }' % (name, self._name, name))

    def print_code(self, declarations):
        print('enum %s {' % self._name)
        print('  %s' % ',\n  '.join('%s = %d' % constant for constant in sorted(self._constants.items())))
        print('};')
        print()
        print('inline bool %s_IsValid(int value) {' % self._name)
        print('  return %s;' % ' || '.join('value == %d' % v for v in sorted(self._canonical)))
        print('}')
        print()
        print('inline const char* %s_Name(int value) {' % self._name)
        print('  switch (value) {')
        for value, name in sorted(self._canonical.items()):
            print('  case %d: return "%s";' % (value, name))
        print('  default: return "";')
        print('  }')
        print('}')
        print()
        print('inline bool %s_Parse(std::string_view name, %s* value) {' % (self._name, self._name))
        for name in sorted(self._constants):
            print('  if (name == "%s") { *value = %s::%s; return true; }' % (name, self._name, name))
        print('  return false;')
        print('}')
        print()
        print('const %s %s_MIN = %s::%s;' % (self._name, self._name, self._name, self._canonical[min(self._canonical)]))
        print('const %s %s_MAX = %s::%s;' % (self._name, self._name, self._name, self._canonical[max(self._canonical)]))
        print('const std::size_t %s_ARRAYSIZE = %d;' % (self._name, max(self._canonical) + 1))

    def print_fields(self, name):
        print('  %s %s_;' % (self._name, name))

    def print_parsing(self, name):
        print('        const char *enumstr;')
        print('        std::size_t enumlen;')
        print('        if (argdata_get_str(value, &enumstr, &enumlen) == 0)')
        print('          %s_Parse(std::string_view(enumstr, enumlen), &%s_);' % (self._name, name))

    def print_parsing_map_value(self, name):
        print('          const char *enumstr;')
        print('          std::size_t enumlen;')
        print('          if (argdata_get_str(value, &enumstr, &enumlen) == 0)')
        print('            %s_Parse(std::string_view(enumstr, enumlen), &%s_.emplace(mapkey, %s::%s).first->second);' % (self._name, name, self._name, self._canonical[0]))

    def print_parsing_repeated(self, name):
        print('          const char *enumstr;')
        print('          std::size_t enumlen;')
        print('          if (argdata_get_str(value, &enumstr, &enumlen) == 0)')
        print('            %s_Parse(std::string_view(enumstr, enumlen), &%s_.emplace_back(%s::%s));' % (self._name, name, self._name, self._canonical[0]))


class MessageFieldDeclaration:

    grammar = [
        MapType,
        RepeatedType,
        PrimitiveType,
    ], pypeg2.word, '=', pypeg2.ignore(re.compile(r'\d+')), ';',

    def __init__(self, arguments):
        self._type = arguments[0]
        self._name = arguments[1]

    def get_name(self, sanitized):
        if sanitized and self._name in FORBIDDEN_WORDS:
            return self._name + '_'
        return self._name

    def get_type(self):
        return self._type


class MessageDeclaration:

    grammar = 'message', pypeg2.word, '{', pypeg2.maybe_some(
        MessageFieldDeclaration
    ), '}'

    def __init__(self, arguments):
        self._name = arguments[0]
        self._fields = arguments[1:]

    def get_dependencies(self):
        r = set()
        for field in self._fields:
            r |= field.get_type().get_dependencies()
        return r

    def get_initializer(self, name):
        return 'has_%s_(false)' % name

    def get_name(self):
        return self._name

    def get_return_type(self):
        return 'const %s&' % self._name

    def print_accessors(self, name):
        print('  bool has_%s() const { return has_%s_; }' % (name, name))
        print('  const %s& %s() const { return %s_; }' % (self._name, name, name))
        print('  %s* mutable_%s() {' % (self._name, name))
        print('    has_%s_ = true;' % name)
        print('    return &%s_;' % name)
        print('  }')
        print('  void clear_%s() {' % name)
        print('    has_%s_ = false;' % name)
        print('    %s_ = %s();' % (name, self._name))
        print('  }')

    def print_accessors_repeated(self, name):
        print('  const %s& %s(std::size_t index) const { return %s_[index]; }' % (self._name, name, name))
        print('  %s* mutable_%s(std::size_t index) { return &%s_[index]; }' % (self._name, name, name))
        print('  %s* add_%s() { return &%s_.emplace_back(); }' % (self._name, name, name))


    def print_code(self, declarations):
        print('class %s final : public arpc::Message {' % self._name)
        print(' public:')
        initializers = list(filter(None, (
            field.get_type().get_initializer(field.get_name(True), declarations)
            for field in sorted(self._fields, key=lambda field: field.get_name(False)))))
        if initializers:
            print('  %s() : %s {}' % (self._name, ', '.join(initializers)))
            print()
        print('  void Parse(const argdata_t& ad) override {')
        if self._fields:
            print('    argdata_map_iterator_t it;')
            print('    argdata_map_iterate(&ad, &it);')
            print('    const argdata_t* key;')
            print('    const argdata_t* value;')
            print('    while (argdata_map_next(&it, &key, &value)) {')
            print('      const char* keystr;')
            print('      std::size_t keylen;')
            print('      if (argdata_get_str(key, &keystr, &keylen) != 0)')
            print('        continue;')
            print('      std::string_view keyss(keystr, keylen);')
            prefix = ''
            for field in sorted(self._fields, key=lambda field: field.get_name(False)):
                print('      %sif (keyss == "%s") {' % (prefix, field.get_name(False)))
                field.get_type().print_parsing(field.get_name(True), declarations)
                prefix = '} else '
            print('      }')
            print('    }')
        print('  }')
        print()

        for field in sorted(self._fields, key=lambda field: field.get_name(False)):
            field.get_type().print_accessors(field.get_name(True), declarations)
            print()

        print(' private:')
        for field in sorted(self._fields, key=lambda field: field.get_name(False)):
            field.get_type().print_fields(field.get_name(True), declarations)

        print('};')

    def print_fields(self, name):
        print('  bool has_%s_;' % name)
        print('  %s %s_;' % (self._name, name))

    def print_parsing(self, name):
        print('        has_%s_ = true;' % name)
        print('        %s_.Parse(*value);' % name)

    def print_parsing_map_value(self, name):
        print('          %s_.emplace(mapkey, %s()).first->second.Parse(*value2);' % (name, self._name))

    def print_parsing_repeated(self, name):
        print('          %s_.emplace_back().Parse(*element);' % name)


class ServiceRpcDeclaration:

    grammar = 'rpc', pypeg2.word, '(', [
        StreamType,
        ReferenceType,
    ], ')', 'returns', '(', [
        StreamType,
        ReferenceType,
    ], ')', [
        ';',
        ('{', '}'),
    ]

    def __init__(self, arguments):
        self._name = arguments[0]
        self._argument_type = arguments[1]
        self._return_type = arguments[2]

    def get_dependencies(self):
        return (self._argument_type.get_dependencies() |
                self._return_type.get_dependencies())

    def get_name(self):
        return self._name

    def print_service_function(self, declarations):
        if self._argument_type.is_stream():
            if self._return_type.is_stream():
                print('  virtual arpc::Status %s(arpc::ServerContext* context, arpc::ServerReaderWriter<%s, %s>* stream) {' % (self._name, self._argument_type.get_storage_type(declarations), self._return_type.get_storage_type(declarations)))
            else:
                print('  virtual arpc::Status %s(arpc::ServerContext* context, arpc::ServerReader<%s>* reader, %s* response) {' % (self._name, self._argument_type.get_storage_type(declarations), self._return_type.get_storage_type(declarations)))
        else:
            if self._return_type.is_stream():
                print('  virtual arpc::Status %s(arpc::ServerContext* context, const %s* request, arpc::ServerWriter<%s>* writer) {' % (self._name, self._argument_type.get_storage_type(declarations), self._return_type.get_storage_type(declarations)))
            else:
                print('  virtual arpc::Status %s(arpc::ServerContext* context, const %s* request, %s* response) {' % (self._name, self._argument_type.get_storage_type(declarations), self._return_type.get_storage_type(declarations)))
        print('    return arpc::Status(arpc::StatusCode::UNIMPLEMENTED, "Operation not provided by this implementation");')
        print('  }')

    def print_stub_function(self, service, declarations):
        if self._argument_type.is_stream():
            if self._return_type.is_stream():
                print('  std::unique_ptr<arpc::ClientReaderWriter<%s, %s>> %s(arpc::ClientContext* context) {' % (self._argument_type.get_storage_type(declarations), self._return_type.get_storage_type(declarations), self._name))
                print('    return std::make_unique<arpc::ClientReaderWriter<%s, %s>>(channel_.get(), "%s", "%s", context);' % (self._argument_type.get_storage_type(declarations), self._return_type.get_storage_type(declarations), service, self._name))
                print('}')
            else:
                print('  std::unique_ptr<arpc::ClientWriter<%s>> %s(arpc::ClientContext* context, %s* response) {' % (self._argument_type.get_storage_type(declarations), self._name, self._return_type.get_storage_type(declarations)))
                print('    return std::make_unique<arpc::ClientWriter<%s>>(channel_.get(), "%s", "%s", context, response);' % (self._argument_type.get_storage_type(declarations), service, self._name))
                print('}')
        else:
            if self._return_type.is_stream():
                print('  std::unique_ptr<arpc::ClientReader<%s>> %s(arpc::ClientContext* context, const %s& request) {' % (self._return_type.get_storage_type(declarations), self._name, self._argument_type.get_storage_type(declarations)))
                print('    return std::make_unique<arpc::ClientReader<%s>>(channel_.get(), "%s", "%s", context, request);' % (self._return_type.get_storage_type(declarations), service, self._name))
                print('}')
            else:
                print('  arpc::Status %s(arpc::ClientContext* context, const %s& request, %s* response) {' % (self._name, self._argument_type.get_storage_type(declarations), self._return_type.get_storage_type(declarations)))
                print('    return arpc::Status(arpc::StatusCode::UNIMPLEMENTED, "TODO(ed)");')
                print('}')


class ServiceDeclaration:

    grammar = 'service', pypeg2.word, '{', pypeg2.maybe_some(
        ServiceRpcDeclaration
    ), '}'

    def __init__(self, arguments):
        self._name = arguments[0]
        self._rpcs = sorted(arguments[1:], key=lambda rpc: rpc.get_name())

    def get_name(self):
        return self._name

    def get_dependencies(self):
        r = set()
        for rpc in self._rpcs:
            r |= rpc.get_dependencies()
        return r

    def print_code(self, declarations):
        print('namespace %s {' % self._name)
        print()
        print('class Service {')
        print(' public:')
        for rpc in self._rpcs:
            rpc.print_service_function(declarations)
            print()
        print('};')
        print()
        print('class Stub {')
        print(' public:')
        print('  explicit Stub(const std::shared_ptr<arpc::Channel>& channel)')
        print('      : channel_(channel) {}')
        print()
        for rpc in self._rpcs:
            rpc.print_stub_function(self._name, declarations)
            print()
        print(' private:')
        print('  const std::shared_ptr<arpc::Channel> channel_;')
        print('};')
        print()
        print('std::unique_ptr<Stub> NewStub(const std::shared_ptr<arpc::Channel>& channel) {')
        print('  return std::make_unique<Stub>(channel);')
        print('}')
        print()
        print('}')
        pass


ProtoFile = (
    'syntax', '=', ['"proto3"', '\'proto3\''], ';',
    'package', pypeg2.word, ';',
    pypeg2.ignore((
        pypeg2.maybe_some(['import', 'option'], pypeg2.restline),
    )),
    pypeg2.maybe_some([
        EnumDeclaration,
        MessageDeclaration,
        ServiceDeclaration,
    ])
)


input_str = sys.stdin.read()
input_sha256 = hashlib.sha256(input_str.encode('UTF-8')).hexdigest()
declarations = pypeg2.parse(input_str, ProtoFile, comment=pypeg2.comment_cpp)
package = declarations[0]
declarations = {declaration.get_name(): declaration
                for declaration in declarations[2:]}

def sort_declarations_by_dependencies(declarations):
    return toposort.toposort_flatten(
        {declaration.get_name(): declaration.get_dependencies()
         for declaration in declarations},
        sort=True)

print('#ifndef APROTOC_%s' % input_sha256)
print('#define APROTOC_%s' % input_sha256)
print()
print('#include <cstdint>')
print('#include <map>')
print('#include <memory>')
print('#include <string>')
print('#include <string_view>')
print('#include <vector>')
print()
print('#include <argdata.h>')
print('#include <arpc++/arpc++.h>')
print()
print('namespace %s {' % package)
print('namespace {')
print()

for declaration in sort_declarations_by_dependencies(declarations.values()):
    declarations[declaration].print_code(declarations)
    print()

print('}')
print('}')
print()

print('#endif')
