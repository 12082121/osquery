/**
 *  Copyright (c) 2014-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under both the Apache 2.0 license (found in the
 *  LICENSE file in the root directory of this source tree) and the GPLv2 (found
 *  in the COPYING file in the root directory of this source tree).
 *  You may select, at your option, one of the above-listed licenses.
 */

#pragma once

#include <string>
#include <vector>

#ifdef WIN32
#define GLOG_NO_ABBREVIATED_SEVERITIES
#define GOOGLE_GLOG_DLL_DECL
#endif

#include <glog/logging.h>

#include <boost/noncopyable.hpp>

#include <osquery/core.h>
#include <osquery/flags.h>
#include <osquery/plugins/plugin.h>
#include <osquery/query.h>

namespace osquery {

/**
 * @brief Helper logging macro for table-generated verbose log lines.
 *
 * Since logging in tables does not always mean a critical warning or error
 * but more likely a parsing or expected edge-case, we provide a TLOG.
 *
 * The tool user can set within config or via the CLI what level of logging
 * to tolerate. It's the table developer's job to assume consistency in logging.
 */
#define TLOG VLOG(1)

/**
 * @brief Prepend a reference number to the log line.
 *
 * A reference number is an external-search helper for somewhat confusing or
 * seeminly-critical log lines.
 */
#define RLOG(n) "[Ref #" #n "] "


/// Set the verbose mode, changes Glog's sinking logic and will affect plugins.
void setVerboseLevel();

/**
 * @brief Start status logging to a buffer until the logger plugin is online.
 *
 * This will also call google::InitGoogleLogging. Use the default init_glog
 * to control this in tests to protect against calling the API twice.
 */
void initStatusLogger(const std::string& name, bool init_glog = true);

/**
 * @brief Initialize the osquery Logger facility by dumping the buffered status
 * logs and configurating status log forwarding.
 *
 * initLogger will disable the `BufferedLogSink` facility, dump any status logs
 * emitted between process start and this init call, then configure the new
 * logger facility to receive status logs.
 *
 * The `forward_all` control is used when buffering logs in extensions.
 * It is fine if the logger facility in the core app does not want to receive
 * status logs, but this is NOT an option in extensions/modules. All status
 * logs must be forwarded to the core.
 *
 * @param name The process name.
 */
void initLogger(const std::string& name);

/**
 * @brief Log a string using the default logger receiver.
 *
 * Note that this method should only be used to log results. If you'd like to
 * log normal osquery operations, use Google Logging.
 *
 * @param message the string to log
 * @param category a category/metadata key
 *
 * @return Status indicating the success or failure of the operation
 */
Status logString(const std::string& message, const std::string& category);

/**
 * @brief Log a string using a specific logger receiver.
 *
 * Note that this method should only be used to log results. If you'd like to
 * log normal osquery operations, use Google Logging.
 *
 * @param message the string to log
 * @param category a category/metadata key
 * @param receiver a string representing the log receiver to use
 *
 * @return Status indicating the success or failure of the operation
 */
Status logString(const std::string& message,
                 const std::string& category,
                 const std::string& receiver);

/**
 * @brief Log results of scheduled queries to the default receiver
 *
 * @param item a struct representing the results of a scheduled query
 *
 * @return Status indicating the success or failure of the operation
 */
Status logQueryLogItem(const QueryLogItem& item);

/**
 * @brief Log results of scheduled queries to a specified receiver
 *
 * @param item a struct representing the results of a scheduled query
 * @param receiver a string representing the log receiver to use
 *
 * @return Status indicating the success or failure of the operation
 */
Status logQueryLogItem(const QueryLogItem& item, const std::string& receiver);

/**
 * @brief Log raw results from a query (or a snapshot scheduled query).
 *
 * @param item the unmangled results from the query planner.
 *
 * @return Status indicating the success or failure of the operation
 */
Status logSnapshotQuery(const QueryLogItem& item);

/**
 * @brief Sink a set of buffered status logs.
 *
 * When the osquery daemon uses a watcher/worker set, the watcher's status logs
 * are accumulated in a buffered log sink. Well-performing workers should have
 * the set of watcher status logs relayed and sent to the configured logger
 * plugin.
 *
 * Status logs from extensions will be forwarded to the extension manager (core)
 * normally, but the watcher does not receive or send registry requests.
 * Extensions, the registry, configuration, and optional config/logger plugins
 * are all protected as a monitored worker.
 */
void relayStatusLogs(bool async = false);

/// Inspect the number of internal-buffered status log lines.
size_t queuedStatuses();

/// Inspect the number of active internal status log sender threads.
size_t queuedSenders();

/**
 * @brief Write a log line to the OS system log.
 *
 * There are occasional needs to log independently of the osquery logging
 * facilities. This allows a feature (not a table) to bypass all osquery
 * configuration and log to the OS system log.
 *
 * Linux/Darwin: this uses syslog's LOG_NOTICE.
 * Windows: This will end up inside the Facebook/osquery in the Windows
 * Event Log.
 */
void systemLog(const std::string& line);
} // namespace osquery
