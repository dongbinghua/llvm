//==---------------------- verification_collector.cpp ----------------------==//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

/// \file verification_collector.cpp
/// Routines to verify function arguments and trace.

#include "xpti/xpti_trace_framework.h"

#include "pi_arguments_handler.hpp"
#include "pi_structs.hpp"
#include "usm_analyzer.hpp"

#include <detail/plugin_printers.hpp>
#include <sycl/detail/spinlock.hpp>

#include <iostream>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>

extern sycl::detail::SpinLock GlobalLock;

void vPrintersInit() {
  std::string_view PrinterType(std::getenv("SYCL_TRACE_PRINT_FORMAT"));
  // all types are the same now
  std::ignore = PrinterType;

  auto &GS = USMAnalyzer::getInstance();
  GS.setupUSMHandlers();
  // this environment variable is for proper testing only
  GS.changeTerminationOnErrorState(
      std::getenv("SYCL_TRACE_TERMINATE_ON_WARNING"));
}

void vPrintersFinish() {}

XPTI_CALLBACK_API void vCallback(uint16_t TraceType,
                                 xpti::trace_event_data_t * /*Parent*/,
                                 xpti::trace_event_data_t *ObjectEvent,
                                 uint64_t /*Instance*/, const void *UserData) {
  auto &GS = USMAnalyzer::getInstance();
  GS.fillLastTracepointData(ObjectEvent);

  // Lock while we print information
  std::lock_guard<sycl::detail::SpinLock> _{GlobalLock};
  const auto *Data = static_cast<const xpti::function_with_args_t *>(UserData);
  const auto *Plugin = static_cast<pi_plugin *>(Data->user_data);
  if (TraceType == xpti::trace_function_with_args_begin) {
    GS.ArgHandlerPreCall.handle(Data->function_id, *Plugin, std::nullopt,
                                Data->args_data);
  } else if (TraceType == xpti::trace_function_with_args_end) {
    const pi_result Result = *static_cast<pi_result *>(Data->ret_data);
    GS.ArgHandlerPostCall.handle(Data->function_id, *Plugin, Result,
                                 Data->args_data);
  }
}
