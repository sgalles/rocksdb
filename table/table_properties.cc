//  Copyright (c) 2013, Facebook, Inc.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.

#include "table/table_properties_internal.h"
#include "rocksdb/table_properties.h"
#include "rocksdb/iterator.h"
#include "rocksdb/env.h"
#include "port/port.h"
#include "table/internal_iterator.h"
#include "util/string_util.h"

namespace rocksdb {

const uint32_t TablePropertiesCollectorFactory::Context::kUnknownColumnFamily =
    port::kMaxInt32;

namespace {
  void AppendProperty(
      std::string& props,
      const std::string& key,
      const std::string& value,
      const std::string& prop_delim,
      const std::string& kv_delim) {
    props.append(key);
    props.append(kv_delim);
    props.append(value);
    props.append(prop_delim);
  }

  template <class TValue>
  void AppendProperty(
      std::string& props,
      const std::string& key,
      const TValue& value,
      const std::string& prop_delim,
      const std::string& kv_delim) {
    AppendProperty(
        props, key, ToString(value), prop_delim, kv_delim
    );
  }
}

std::string TableProperties::ToString(
    const std::string& prop_delim,
    const std::string& kv_delim) const {
  std::string result;
  result.reserve(1024);

  // Basic Info
  AppendProperty(result, "# data blocks", num_data_blocks, prop_delim,
                 kv_delim);
  AppendProperty(result, "# entries", num_entries, prop_delim, kv_delim);

  AppendProperty(result, "raw key size", raw_key_size, prop_delim, kv_delim);
  AppendProperty(result, "raw average key size",
                 num_entries != 0 ? 1.0 * raw_key_size / num_entries : 0.0,
                 prop_delim, kv_delim);
  AppendProperty(result, "raw value size", raw_value_size, prop_delim,
                 kv_delim);
  AppendProperty(result, "raw average value size",
                 num_entries != 0 ? 1.0 * raw_value_size / num_entries : 0.0,
                 prop_delim, kv_delim);

  AppendProperty(result, "data block size", data_size, prop_delim, kv_delim);
  AppendProperty(result, "index block size", index_size, prop_delim, kv_delim);
  AppendProperty(result, "filter block size", filter_size, prop_delim,
                 kv_delim);
  AppendProperty(result, "(estimated) table size",
                 data_size + index_size + filter_size, prop_delim, kv_delim);

  AppendProperty(
      result, "filter policy name",
      filter_policy_name.empty() ? std::string("N/A") : filter_policy_name,
      prop_delim, kv_delim);

  AppendProperty(result, "comparator name",
                 comparator_name.empty() ? std::string("N/A") : comparator_name,
                 prop_delim, kv_delim);

  AppendProperty(
      result, "merge operator name",
      merge_operator_name.empty() ? std::string("N/A") : merge_operator_name,
      prop_delim, kv_delim);

  AppendProperty(result, "property collectors names",
                 property_collectors_names.empty() ? std::string("N/A")
                                                   : property_collectors_names,
                 prop_delim, kv_delim);

  return result;
}

void TableProperties::Add(const TableProperties& tp) {
  data_size += tp.data_size;
  index_size += tp.index_size;
  filter_size += tp.filter_size;
  raw_key_size += tp.raw_key_size;
  raw_value_size += tp.raw_value_size;
  num_data_blocks += tp.num_data_blocks;
  num_entries += tp.num_entries;
}

const std::string TablePropertiesNames::kDataSize  =
    "rocksdb.data.size";
const std::string TablePropertiesNames::kIndexSize =
    "rocksdb.index.size";
const std::string TablePropertiesNames::kFilterSize =
    "rocksdb.filter.size";
const std::string TablePropertiesNames::kRawKeySize =
    "rocksdb.raw.key.size";
const std::string TablePropertiesNames::kRawValueSize =
    "rocksdb.raw.value.size";
const std::string TablePropertiesNames::kNumDataBlocks =
    "rocksdb.num.data.blocks";
const std::string TablePropertiesNames::kNumEntries =
    "rocksdb.num.entries";
const std::string TablePropertiesNames::kFilterPolicy =
    "rocksdb.filter.policy";
const std::string TablePropertiesNames::kFormatVersion =
    "rocksdb.format.version";
const std::string TablePropertiesNames::kFixedKeyLen =
    "rocksdb.fixed.key.length";
const std::string TablePropertiesNames::kComparator = "rocksdb.comparator";
const std::string TablePropertiesNames::kMergeOperator =
    "rocksdb.merge.operator";
const std::string TablePropertiesNames::kPropertyCollectors =
    "rocksdb.property.collectors";

extern const std::string kPropertiesBlock = "rocksdb.properties";
// Old property block name for backward compatibility
extern const std::string kPropertiesBlockOldName = "rocksdb.stats";

// Seek to the properties block.
// Return true if it successfully seeks to the properties block.
Status SeekToPropertiesBlock(InternalIterator* meta_iter, bool* is_found) {
  *is_found = true;
  meta_iter->Seek(kPropertiesBlock);
  if (meta_iter->status().ok() &&
      (!meta_iter->Valid() || meta_iter->key() != kPropertiesBlock)) {
    meta_iter->Seek(kPropertiesBlockOldName);
    if (meta_iter->status().ok() &&
        (!meta_iter->Valid() || meta_iter->key() != kPropertiesBlockOldName)) {
      *is_found = false;
    }
  }
  return meta_iter->status();
}

}  // namespace rocksdb
