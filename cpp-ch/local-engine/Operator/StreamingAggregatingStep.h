/*
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once
#include <cstddef>
#include <unordered_map>
#include <Core/Block.h>
#include <Interpreters/Aggregator.h>
#include <Interpreters/Context.h>
#include <Processors/Chunk.h>
#include <Processors/IProcessor.h>
#include <Processors/Port.h>
#include <Processors/QueryPlan/IQueryPlanStep.h>
#include <Processors/QueryPlan/ITransformingStep.h>
#include <Processors/Transforms/AggregatingTransform.h>
#include <QueryPipeline/SizeLimits.h>
#include <Poco/Logger.h>
#include <Common/logger_useful.h>

namespace local_engine
{
class StreamingAggregatingTransform : public DB::IProcessor
{
public:
    using Status = DB::IProcessor::Status;
    explicit StreamingAggregatingTransform(DB::ContextPtr context_, const DB::Block &header_, DB::AggregatingTransformParamsPtr params_);
    ~StreamingAggregatingTransform() override;
    String getName() const override { return "StreamingAggregatingTransform"; }
    Status prepare() override;
    void work() override;
private:
    DB::ContextPtr context;
    DB::Block header;
    DB::ColumnRawPtrs key_columns;
    DB::Aggregator::AggregateColumns aggregate_columns;
    DB::AggregatingTransformParamsPtr params;

    bool no_more_keys = false;
    bool is_consume_finished = false;
    bool is_clear_aggregator = false;
    DB::AggregatedDataVariantsPtr data_variants = nullptr;
    bool has_input = false;
    bool has_output = false;
    DB::Chunk input_chunk;
    DB::Chunk output_chunk;
    
    DB::BlocksList pending_blocks;
    Poco::Logger * logger = &Poco::Logger::get("StreamingAggregatingTransform");

    double per_key_memory_usage = 0;

    // metrics
    size_t total_input_blocks = 0;
    size_t total_input_rows = 0;
    size_t total_output_blocks = 0;
    size_t total_output_rows = 0;
    size_t total_clear_data_variants_num = 0;
    size_t total_aggregate_time = 0;
    size_t total_convert_data_variants_time = 0;

    bool isMemoryOverflow();
};

class StreamingAggregatingStep : public DB::ITransformingStep
{
public:
    explicit StreamingAggregatingStep(
        DB::ContextPtr context_,
        const DB::DataStream & input_stream_,
        DB::Aggregator::Params params_);
    ~StreamingAggregatingStep() override = default;

    String getName() const override { return "StreamingAggregating"; }

    void transformPipeline(DB::QueryPipelineBuilder & pipeline, const DB::BuildQueryPipelineSettings &) override;

    void describeActions(DB::JSONBuilder::JSONMap & map) const override;
    void describeActions(DB::IQueryPlanStep::FormatSettings & settings) const override;

private:
    DB::ContextPtr context;
    DB::Aggregator::Params params;
    void updateOutputStream() override;
};
}
