////////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2014-2016 ArangoDB GmbH, Cologne, Germany
/// Copyright 2004-2014 triAGENS GmbH, Cologne, Germany
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
///
/// Copyright holder is ArangoDB GmbH, Cologne, Germany
///
/// @author Max Neunhoeffer
////////////////////////////////////////////////////////////////////////////////

#ifndef ARANGOD_AQL_AQL_VALUE_H
#define ARANGOD_AQL_AQL_VALUE_H 1

#include "Basics/Common.h"
#include "Basics/JsonHelper.h"
#include "Aql/Range.h"
#include "Aql/types.h"

#include <v8.h>

struct TRI_df_marker_t;
struct TRI_document_collection_t;

namespace arangodb {
class AqlTransaction;

namespace basics {
class StringBuffer;
}

namespace velocypack {
template <typename T>
class Buffer;
}
namespace aql {

class AqlItemBlock;

// Temporary Forward
struct AqlValue$;

////////////////////////////////////////////////////////////////////////////////
/// @brief a struct to hold a value, registers hole AqlValue* during the
/// execution
////////////////////////////////////////////////////////////////////////////////

struct AqlValue {
  //////////////////////////////////////////////////////////////////////////////
  /// @brief AqlValueType, indicates what sort of value we have
  //////////////////////////////////////////////////////////////////////////////

  enum AqlValueType {
    EMPTY,   // contains no data
    JSON,    // Json*
    SHAPED,  // TRI_df_marker_t*
    DOCVEC,  // a vector of blocks of results coming from a subquery
    RANGE    // a pointer to a range remembering lower and upper bound
  };

  //////////////////////////////////////////////////////////////////////////////
  /// @brief constructors for the various value types, note that they all take
  /// ownership of the corresponding pointers
  //////////////////////////////////////////////////////////////////////////////

  AqlValue() : _json(nullptr), _type(EMPTY) {}

  explicit AqlValue(arangodb::basics::Json* json) : _json(json), _type(JSON) {
    TRI_ASSERT(_json != nullptr);
  }

  explicit AqlValue(TRI_df_marker_t const* marker)
      : _marker(marker), _type(SHAPED) {
    TRI_ASSERT(_marker != nullptr);
  }

  explicit AqlValue(std::vector<AqlItemBlock*>* vector)
      : _vector(vector), _type(DOCVEC) {
    TRI_ASSERT(_vector != nullptr);
  }

  AqlValue(int64_t low, int64_t high) : _range(nullptr), _type(RANGE) {
    _range = new Range(low, high);
  }

  explicit AqlValue(AqlValue$ const& other);

  //////////////////////////////////////////////////////////////////////////////
  /// @brief destructor, doing nothing automatically!
  //////////////////////////////////////////////////////////////////////////////

  ~AqlValue() {}

  //////////////////////////////////////////////////////////////////////////////
  /// @brief return the value type
  //////////////////////////////////////////////////////////////////////////////

  inline AqlValueType type() const noexcept { return _type; }

  //////////////////////////////////////////////////////////////////////////////
  /// @brief a quick method to decide whether the destroy value needs to be
  /// called for a value
  //////////////////////////////////////////////////////////////////////////////

  inline bool requiresDestruction() const noexcept {
    return (_type != EMPTY && _type != SHAPED);
  }

  //////////////////////////////////////////////////////////////////////////////
  /// @brief a quick method to decide whether a value is empty
  //////////////////////////////////////////////////////////////////////////////

  inline bool isEmpty() const noexcept { return _type == EMPTY; }

  //////////////////////////////////////////////////////////////////////////////
  /// @brief whether or not the AqlValue is a shape
  //////////////////////////////////////////////////////////////////////////////

  inline bool isShaped() const noexcept { return _type == SHAPED; }

  //////////////////////////////////////////////////////////////////////////////
  /// @brief whether or not the AqlValue is a JSON
  //////////////////////////////////////////////////////////////////////////////

  inline bool isJson() const noexcept { return _type == JSON; }

  //////////////////////////////////////////////////////////////////////////////
  /// @brief whether or not the AqlValue is a RANGE
  //////////////////////////////////////////////////////////////////////////////

  inline bool isRange() const noexcept { return _type == RANGE; }

  //////////////////////////////////////////////////////////////////////////////
  /// @brief return the shape marker
  //////////////////////////////////////////////////////////////////////////////

  TRI_df_marker_t const* getMarker() const {
    TRI_ASSERT(isShaped());
    return _marker;
  }

  //////////////////////////////////////////////////////////////////////////////
  /// @brief return the range
  //////////////////////////////////////////////////////////////////////////////

  inline Range const* getRange() const {
    TRI_ASSERT(isRange());
    return _range;
  }

  //////////////////////////////////////////////////////////////////////////////
  /// @brief a quick method to decide whether a value is true
  //////////////////////////////////////////////////////////////////////////////

  bool isTrue() const;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief erase, this does not free the stuff in the AqlValue, it only
  /// erases the pointers and makes the AqlValue structure EMPTY, this
  /// is used when the AqlValue is stolen and stored in another object
  //////////////////////////////////////////////////////////////////////////////

  inline void erase() noexcept {
    _type = EMPTY;
    _json = nullptr;
  }

  //////////////////////////////////////////////////////////////////////////////
  /// @brief destroy, explicit destruction, only when needed
  //////////////////////////////////////////////////////////////////////////////

  void destroy();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief get the name of an AqlValue type
  //////////////////////////////////////////////////////////////////////////////

  std::string getTypeString() const;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief clone for recursive copying
  //////////////////////////////////////////////////////////////////////////////

  AqlValue clone() const;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief shallow clone
  //////////////////////////////////////////////////////////////////////////////

  AqlValue shallowClone() const;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief whether or not the AqlValue contains a string value
  //////////////////////////////////////////////////////////////////////////////

  bool isString() const;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief whether or not the AqlValue contains a numeric value
  //////////////////////////////////////////////////////////////////////////////

  bool isNumber() const;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief whether or not the AqlValue contains a boolean value
  //////////////////////////////////////////////////////////////////////////////

  bool isBoolean() const;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief whether or not the AqlValue contains an array value
  //////////////////////////////////////////////////////////////////////////////

  bool isArray() const;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief whether or not the AqlValue contains an object value
  //////////////////////////////////////////////////////////////////////////////

  bool isObject() const;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief whether or not the AqlValue contains a null value
  //////////////////////////////////////////////////////////////////////////////

  bool isNull(bool emptyIsNull) const;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief returns the array member at position i
  //////////////////////////////////////////////////////////////////////////////

  arangodb::basics::Json at(arangodb::AqlTransaction*, size_t i) const;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief returns the length of an AqlValue containing an array
  //////////////////////////////////////////////////////////////////////////////

  size_t arraySize() const;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief get the numeric value of an AqlValue
  //////////////////////////////////////////////////////////////////////////////

  int64_t toInt64() const;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief get the numeric value of an AqlValue
  //////////////////////////////////////////////////////////////////////////////

  double toNumber(bool&) const;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief get a string representation of the AqlValue
  /// this will fail if the value is not a string
  //////////////////////////////////////////////////////////////////////////////

  std::string toString() const;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief construct a V8 value as input for the expression execution in V8
  //////////////////////////////////////////////////////////////////////////////

  v8::Handle<v8::Value> toV8(v8::Isolate* isolate, arangodb::AqlTransaction*,
                             TRI_document_collection_t const*) const;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief construct a V8 value as input for the expression execution in V8
  /// only construct those attributes that are needed in the expression
  //////////////////////////////////////////////////////////////////////////////

  v8::Handle<v8::Value> toV8Partial(v8::Isolate* isolate,
                                    arangodb::AqlTransaction*,
                                    std::unordered_set<std::string> const&,
                                    TRI_document_collection_t const*) const;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief toJson method
  //////////////////////////////////////////////////////////////////////////////

  arangodb::basics::Json toJson(arangodb::AqlTransaction*,
                                TRI_document_collection_t const*, bool) const;

  void toVelocyPack(arangodb::AqlTransaction*, TRI_document_collection_t const*,
                    arangodb::velocypack::Builder&) const;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief creates a hash value for the AqlValue
  //////////////////////////////////////////////////////////////////////////////

  uint64_t hash(arangodb::AqlTransaction*,
                TRI_document_collection_t const*) const;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief extract an attribute value from the AqlValue
  /// this will return null if the value is not an object
  //////////////////////////////////////////////////////////////////////////////

  arangodb::basics::Json extractObjectMember(
      arangodb::AqlTransaction*, TRI_document_collection_t const*, char const*,
      bool, arangodb::basics::StringBuffer&) const;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief extract a value from an array AqlValue
  /// this will return null if the value is not an array
  /// depending on the last parameter, the return value will either contain a
  /// copy of the original value in the array or a reference to it (which must
  /// not be freed)
  //////////////////////////////////////////////////////////////////////////////

  arangodb::basics::Json extractArrayMember(arangodb::AqlTransaction*,
                                            TRI_document_collection_t const*,
                                            int64_t, bool) const;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief create an AqlValue from a vector of AqlItemBlock*s
  //////////////////////////////////////////////////////////////////////////////

  static AqlValue CreateFromBlocks(arangodb::AqlTransaction*,
                                   std::vector<AqlItemBlock*> const&,
                                   std::vector<std::string> const&);

  //////////////////////////////////////////////////////////////////////////////
  /// @brief create an AqlValue from a vector of AqlItemBlock*s
  //////////////////////////////////////////////////////////////////////////////

  static AqlValue CreateFromBlocks(arangodb::AqlTransaction*,
                                   std::vector<AqlItemBlock*> const&,
                                   arangodb::aql::RegisterId);

  //////////////////////////////////////////////////////////////////////////////
  /// @brief 3-way comparison for AqlValue objects
  //////////////////////////////////////////////////////////////////////////////

  static int Compare(arangodb::AqlTransaction*, AqlValue const&,
                     TRI_document_collection_t const*, AqlValue const&,
                     TRI_document_collection_t const*, bool compareUtf8);

  //////////////////////////////////////////////////////////////////////////////
  /// @brief the actual data
  //////////////////////////////////////////////////////////////////////////////

  union {
    arangodb::basics::Json* _json;
    TRI_df_marker_t const* _marker;
    std::vector<AqlItemBlock*>* _vector;
    Range const* _range;
  };

  //////////////////////////////////////////////////////////////////////////////
  /// @brief _type, the type of value
  //////////////////////////////////////////////////////////////////////////////

  AqlValueType _type;
};

// do not add virtual methods!
struct AqlValue$ {
 friend struct std::hash<arangodb::aql::AqlValue$>;
 friend struct std::equal_to<arangodb::aql::AqlValue$>;
 public:

  //////////////////////////////////////////////////////////////////////////////
  /// @brief AqlValueType, indicates what sort of value we have
  //////////////////////////////////////////////////////////////////////////////

  enum AqlValueType : uint8_t { INTERNAL, EXTERNAL, REFERENCE, REFERENCE_STICKY, RANGE };

  //////////////////////////////////////////////////////////////////////////////
  /// @brief Holds the actual data for this AqlValue
  /// it has the following semantics:
  ///
  /// All values with a size less than 16 will be stored directly in this
  /// AqlValue using the data.internal structure.
  /// All values of a larger size will be stored in _data.external.
  /// The last byte of this union will be used to identify if we have to use
  /// internal or external.
  /// Delete of the Buffer should free every structure that is not using the
  /// VPack external or reference value type.
  //////////////////////////////////////////////////////////////////////////////

 private:
  union {
    char internal[16];
    arangodb::velocypack::Buffer<uint8_t>* external;
    uint8_t const* reference;
  } _data;

 public:
  AqlValue$();
  explicit AqlValue$(arangodb::velocypack::Builder const&);
  explicit AqlValue$(arangodb::velocypack::Builder const*);
  explicit AqlValue$(arangodb::velocypack::Slice const&);
  AqlValue$(arangodb::velocypack::Slice const&, AqlValueType);
  AqlValue$(int64_t, int64_t); // range
  explicit AqlValue$(bool); // boolean type

  AqlValue$(AqlValue const&, arangodb::AqlTransaction*,
            TRI_document_collection_t const*);
  
  ~AqlValue$();
  
  ////////////////////////////////////////////////////////////////////////////////
  /// @brief copy
  ////////////////////////////////////////////////////////////////////////////////

  AqlValue$(AqlValue$ const& other);
  AqlValue$& operator=(AqlValue$ const& other);
  
  ////////////////////////////////////////////////////////////////////////////////
  /// @brief move
  ////////////////////////////////////////////////////////////////////////////////

  AqlValue$(AqlValue$&& other);
  AqlValue$& operator=(AqlValue$&& other);

  //////////////////////////////////////////////////////////////////////////////
  /// @brief Returns the type of this value. If true it uses an external pointer
  /// if false it uses the internal data structure
  //////////////////////////////////////////////////////////////////////////////

  AqlValueType type() const {
    return static_cast<AqlValueType>(_data.internal[15]);
  }

  //////////////////////////////////////////////////////////////////////////////
  /// @brief returns a slice to read this Value's data
  //////////////////////////////////////////////////////////////////////////////

  arangodb::velocypack::Slice slice() const;
  
  //////////////////////////////////////////////////////////////////////////////
  /// @brief whether or not the value is empty / none
  //////////////////////////////////////////////////////////////////////////////
  
  bool isNone() const;
  bool isEmpty() const { return isNone(); }
  
  //////////////////////////////////////////////////////////////////////////////
  /// @brief whether or not the value is a number
  //////////////////////////////////////////////////////////////////////////////

  bool isNumber() const;
  
  //////////////////////////////////////////////////////////////////////////////
  /// @brief whether or not the value is an object
  //////////////////////////////////////////////////////////////////////////////

  bool isObject() const;
  
  //////////////////////////////////////////////////////////////////////////////
  /// @brief whether or not the value is an array (note: this treats ranges
  /// as arrays, too!)
  //////////////////////////////////////////////////////////////////////////////

  bool isArray() const;
  
  //////////////////////////////////////////////////////////////////////////////
  /// @brief get the (array) length (note: this treats ranges as arrays, too!)
  //////////////////////////////////////////////////////////////////////////////

  size_t length() const;
  
  //////////////////////////////////////////////////////////////////////////////
  /// @brief get the (array) element at position 
  //////////////////////////////////////////////////////////////////////////////

  AqlValue$ at(int64_t position) const;
  
  //////////////////////////////////////////////////////////////////////////////
  /// @brief get the (object) element by name(s)
  //////////////////////////////////////////////////////////////////////////////
  
  AqlValue$ get(std::string const&) const;
  
  AqlValue$ get(std::vector<char const*> const&) const;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief get the numeric value of an AqlValue
  //////////////////////////////////////////////////////////////////////////////

  double toDouble() const;
  int64_t toInt64() const;
  
  //////////////////////////////////////////////////////////////////////////////
  /// @brief whether or not an AqlValue evaluates to true/false
  //////////////////////////////////////////////////////////////////////////////

  bool isTrue() const;
  bool isFalse() const { return !isTrue(); }
  
  //////////////////////////////////////////////////////////////////////////////
  /// @brief construct a V8 value as input for the expression execution in V8
  //////////////////////////////////////////////////////////////////////////////

  v8::Handle<v8::Value> toV8(v8::Isolate* isolate, arangodb::AqlTransaction*) const;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief construct a V8 value as input for the expression execution in V8
  /// only construct those attributes that are needed in the expression
  //////////////////////////////////////////////////////////////////////////////

  v8::Handle<v8::Value> toV8Partial(v8::Isolate* isolate,
                                    arangodb::AqlTransaction*,
                                    std::unordered_set<std::string> const&) const;
  
  //////////////////////////////////////////////////////////////////////////////
  /// @brief compare function for two values
  /// TODO: implement
  //////////////////////////////////////////////////////////////////////////////

  static int Compare(arangodb::AqlTransaction*, AqlValue$ const& left, AqlValue$ const& right, bool useUtf8) {
    // TODO: implement
    return 0;
  }

 private:
  
  //////////////////////////////////////////////////////////////////////////////
  /// @brief sets the value type
  //////////////////////////////////////////////////////////////////////////////

   void setType(AqlValueType type);

  //////////////////////////////////////////////////////////////////////////////
  /// @brief helper function for copy construction/assignment
  //////////////////////////////////////////////////////////////////////////////

  void copy(AqlValue$ const&);

  //////////////////////////////////////////////////////////////////////////////
  /// @brief helper function for move construction/assignment
  //////////////////////////////////////////////////////////////////////////////

  void move(AqlValue$&);
  
  //////////////////////////////////////////////////////////////////////////////
  /// @brief destroy the internal data structures
  //////////////////////////////////////////////////////////////////////////////

  void destroy();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief invalidates/resets a value to None
  //////////////////////////////////////////////////////////////////////////////

  void invalidate(AqlValue$&);
  
  //////////////////////////////////////////////////////////////////////////////
  /// @brief get pointer of the included Range object
  //////////////////////////////////////////////////////////////////////////////

  Range* range() const;
};

static_assert(sizeof(AqlValue$) == 16, "invalid AqlValue$ size");

// do not add virtual methods or data members!
struct AqlValueReference final : public AqlValue$ {
 public:
  explicit AqlValueReference(arangodb::velocypack::Slice const& slice)
      : AqlValue$(slice, AqlValueType::REFERENCE) {}
};

static_assert(sizeof(AqlValueReference) == 16, "invalid AqlValueReference size");

// do not add virtual methods or data members!
struct AqlValueDocument final : public AqlValue$ {
 public:
  explicit AqlValueDocument(arangodb::velocypack::Slice const& slice)
      : AqlValue$(slice, AqlValueType::REFERENCE_STICKY) {}
};

static_assert(sizeof(AqlValueDocument) == 16, "invalid AqlValueDocument size");

}  // closes namespace arangodb::aql
}  // closes namespace arangodb

////////////////////////////////////////////////////////////////////////////////
/// @brief hash function for AqlValue objects
////////////////////////////////////////////////////////////////////////////////

namespace std {

template <>
struct hash<arangodb::aql::AqlValue$> {
  size_t operator()(arangodb::aql::AqlValue$ const& x) const {
    std::hash<uint8_t> intHash;
    std::hash<void const*> ptrHash;
    arangodb::aql::AqlValue$::AqlValueType type = x.type();
    size_t res = intHash(type);
    switch (type) {
      case arangodb::aql::AqlValue$::INTERNAL: {
        return res ^ x.slice().hash();
      }
      case arangodb::aql::AqlValue$::EXTERNAL: {
        return res ^ ptrHash(x._data.external);
      }
      case arangodb::aql::AqlValue$::REFERENCE: 
      case arangodb::aql::AqlValue$::REFERENCE_STICKY: {
        return res ^ ptrHash(x._data.reference);
      }
      case arangodb::aql::AqlValue$::RANGE: {
        return res ^ ptrHash(x.range());
      }
    }

    TRI_ASSERT(false);
    return 0;
  }
};

template <>
struct equal_to<arangodb::aql::AqlValue$> {
  bool operator()(arangodb::aql::AqlValue$ const& a,
                  arangodb::aql::AqlValue$ const& b) const {
    arangodb::aql::AqlValue$::AqlValueType type = a.type();
    if (type != b.type()) {
      return false;
    }
    switch (type) {
      case arangodb::aql::AqlValue$::INTERNAL: {
        return a.slice().equals(b.slice());
      }
      case arangodb::aql::AqlValue$::EXTERNAL: {
        return a._data.external == b._data.external;
      }
      case arangodb::aql::AqlValue$::REFERENCE: 
      case arangodb::aql::AqlValue$::REFERENCE_STICKY: {
        return a._data.reference == b._data.reference;
      }
      case arangodb::aql::AqlValue$::RANGE: {
        return a.range() == b.range();
      }

      default: {
        TRI_ASSERT(false);
        return false;
      }
    }
  }
};


template <>
struct hash<arangodb::aql::AqlValue> {
  size_t operator()(arangodb::aql::AqlValue const& x) const {
    std::hash<uint32_t> intHash;
    std::hash<void const*> ptrHash;
    size_t res = intHash(static_cast<uint32_t>(x._type));
    switch (x._type) {
      case arangodb::aql::AqlValue::JSON: {
        return res ^ ptrHash(x._json);
      }
      case arangodb::aql::AqlValue::SHAPED: {
        return res ^ ptrHash(x._marker);
      }
      case arangodb::aql::AqlValue::DOCVEC: {
        return res ^ ptrHash(x._vector);
      }
      case arangodb::aql::AqlValue::RANGE: {
        return res ^ ptrHash(x._range);
      }
      case arangodb::aql::AqlValue::EMPTY: {
        return res;
      }
    }

    TRI_ASSERT(false);
    return 0;
  }
};

template <>
struct equal_to<arangodb::aql::AqlValue> {
  bool operator()(arangodb::aql::AqlValue const& a,
                  arangodb::aql::AqlValue const& b) const {
    if (a._type != b._type) {
      return false;
    }
    switch (a._type) {
      case arangodb::aql::AqlValue::JSON: {
        return a._json == b._json;
      }
      case arangodb::aql::AqlValue::SHAPED: {
        return a._marker == b._marker;
      }
      case arangodb::aql::AqlValue::DOCVEC: {
        return a._vector == b._vector;
      }
      case arangodb::aql::AqlValue::RANGE: {
        return a._range == b._range;
      }
      // case arangodb::aql::AqlValue::EMPTY intentionally not handled here!
      // (should fall through and fail!)

      default: {
        TRI_ASSERT(false);
        return true;
      }
    }
  }
};

}  // closes namespace std

#endif
