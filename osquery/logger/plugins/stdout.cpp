/**
 *  Copyright (c) 2014-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under both the Apache 2.0 license (found in the
 *  LICENSE file in the root directory of this source tree) and the GPLv2 (found
 *  in the COPYING file in the root directory of this source tree).
 *  You may select, at your option, one of the above-listed licenses.
 */

#include "stdout.h"

#include <iostream>
#include <glog/logging.h>
#include <osquery/flags.h>

namespace osquery {

Status StdoutLoggerPlugin::logString(const std::string& s) {
  std::cout << s << std::endl;
  return Status();
}

Status StdoutLoggerPlugin::logStatus(const std::vector<StatusLogLine>& log) {
  for (const auto& item : log) {
    std::string line = "severity=" + std::to_string(item.severity) +
                       " location=" + item.filename + ":" +
                       std::to_string(item.line) + " message=" + item.message;
    std::cout << line << std::endl;
  }
  return Status();
}

void StdoutLoggerPlugin::init(const std::string& name,
                              const std::vector<StatusLogLine>& log) {
  // Stop the internal Glog facilities.
  FLAGS_alsologtostderr = false;
  FLAGS_logtostderr = false;
  FLAGS_stderrthreshold = 5;

  // Now funnel the intermediate status logs provided to `init`.
  logStatus(log);
}
}
