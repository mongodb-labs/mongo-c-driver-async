#pragma once

#include <bson/value_ref.h>
#include <bson/view.h>

#include <mlib/time.h>

#include <stdint.h>

typedef struct amongoc_aggregate_params {
    bool           allow_disk_use;
    int32_t        batch_size;
    bool           bypass_document_validation;
    bson_view      collation;
    mlib_duration  max_await_time;
    bson_value_ref comment;
    bson_value_ref hint;
    bson_view      let;
} amongoc_aggregate_params;
