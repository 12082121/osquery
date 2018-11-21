/**
 *  Copyright (c) 2014-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under both the Apache 2.0 license (found in the
 *  LICENSE file in the root directory of this source tree) and the GPLv2 (found
 *  in the COPYING file in the root directory of this source tree).
 *  You may select, at your option, one of the above-listed licenses.
 */

#include <chrono>
#include <deque>
#include <random>
#include <sstream>
#include <thread>

#include <csignal>
#include <ctime>

#include <gtest/gtest.h>

#include <boost/filesystem/operations.hpp>

#include <osquery/logger.h>
#include <osquery/registry_factory.h>
#include <osquery/sql.h>
#include <osquery/system.h>
#include <osquery/utils/system/time.h>
#include <osquery/utils/conversions/tryto.h>

#include <osquery/core/process.h>
#include <osquery/tests/test_util.h>
#include <osquery/utils/info/platform_type.h>

namespace fs = boost::filesystem;

namespace osquery {

/// Will be set with initTesting in test harness main.
std::string kFakeDirectory;

/// Will be set with initTesting in test harness main.
std::string kTestWorkingDirectory;

/// The relative path within the source repo to find test content.
std::string kTestDataPath{"../../../tools/tests/"};

DECLARE_string(database_path);
DECLARE_string(extensions_socket);
DECLARE_string(extensions_autoload);
DECLARE_string(enroll_tls_endpoint);
DECLARE_bool(disable_logging);
DECLARE_bool(disable_database);

using chrono_clock = std::chrono::high_resolution_clock;

void initTesting() {
  Config::setStartTime(getUnixTime());

  kToolType = ToolType::TEST;
  if (osquery::isPlatform(PlatformType::TYPE_OSX)) {
    kTestWorkingDirectory = "/private/tmp/osquery-tests";
  } else {
    kTestWorkingDirectory =
        (fs::temp_directory_path() / "osquery-tests").string();
  }

  if (osquery::isPlatform(PlatformType::TYPE_WINDOWS)) {
    kTestDataPath = "../" + kTestDataPath;
  }

  registryAndPluginInit();

  // Allow unit test execution from anywhere in the osquery source/build tree.
  if (fs::exists("test_data/test_inline_pack.conf")) {
    // If the execution occurs within the build/shared directory and shared
    // is pointing to a tmp build directory. See #3414.
    kTestDataPath = "test_data/";
  } else if (fs::exists("../test_data/test_inline_pack.conf")) {
    // ctest executes from the osquery subdirectory. If this is a build/shared
    // link then the test_data directory links to the source repo.
    kTestDataPath = "../test_data/";
  } else {
    while (kTestDataPath.find("tools") != 0) {
      if (!fs::exists(kTestDataPath + "test_inline_pack.conf")) {
        kTestDataPath = kTestDataPath.substr(3, kTestDataPath.size());
      } else {
        break;
      }
    }
  }

  // The tests will fail randomly without test data.
  if (!fs::exists(kTestDataPath)) {
    throw std::runtime_error("Cannot find test data path");
  }

  // Seed the random number generator, some tests generate temporary files
  // ports, sockets, etc using random numbers.
  std::srand(static_cast<unsigned int>(
      chrono_clock::now().time_since_epoch().count()));

  // Set safe default values for path-based flags.
  // Specific unittests may edit flags temporarily.
  kTestWorkingDirectory += std::to_string(platformGetUid()) + "/";
  kFakeDirectory = kTestWorkingDirectory + kFakeDirectoryName;

  fs::remove_all(kTestWorkingDirectory);
  fs::create_directories(kTestWorkingDirectory);
  FLAGS_database_path = kTestWorkingDirectory + "unittests.db";
  FLAGS_extensions_socket = kTestWorkingDirectory + "unittests.em";
  FLAGS_extensions_autoload = kTestWorkingDirectory + "unittests-ext.load";

  FLAGS_disable_logging = true;
  FLAGS_disable_database = true;

  // Tests need a database plugin.
  // Set up the database instance for the unittests.
  DatabasePlugin::setAllowOpen(true);
  DatabasePlugin::initPlugin();

  Initializer::platformSetup();
}

void shutdownTesting() {
  DatabasePlugin::shutdown();

  Initializer::platformTeardown();
}

ScheduledQuery getOsqueryScheduledQuery() {
  ScheduledQuery sq;
  sq.query = "SELECT filename FROM fs WHERE path = '/bin' ORDER BY filename";
  sq.interval = 5;
  return sq;
}

std::string getCACertificateContent() {
  std::string content;
  readFile(fs::path(kTestDataPath) / "test_cert.pem", content);
  return content;
}

std::string getEtcHostsContent() {
  std::string content;
  readFile(fs::path(kTestDataPath) / "test_hosts.txt", content);
  return content;
}

std::string getEtcHostsIcsContent() {
  std::string content;
  readFile(fs::path(kTestDataPath) / "test_hosts_ics.txt", content);
  return content;
}

std::string getEtcProtocolsContent() {
  std::string content;
  readFile(fs::path(kTestDataPath) / "test_protocols.txt", content);
  return content;
}

QueryData getEtcHostsExpectedResults() {
  Row row1;
  Row row2;
  Row row3;
  Row row4;
  Row row5;
  Row row6;

  row1["address"] = "127.0.0.1";
  row1["hostnames"] = "localhost";
  row2["address"] = "255.255.255.255";
  row2["hostnames"] = "broadcasthost";
  row3["address"] = "::1";
  row3["hostnames"] = "localhost";
  row4["address"] = "fe80::1%lo0";
  row4["hostnames"] = "localhost";
  row5["address"] = "127.0.0.1";
  row5["hostnames"] = "example.com example";
  row6["address"] = "127.0.0.1";
  row6["hostnames"] = "example.net";
  return {row1, row2, row3, row4, row5, row6};
}

QueryData getEtcHostsIcsExpectedResults() {
  Row row1;

  row1["address"] = "192.168.11.81";
  row1["hostnames"] = "VM-q27rkc8son.mshome.net";
  return {row1};
}

QueryData getEtcProtocolsExpectedResults() {
  Row row1;
  Row row2;
  Row row3;

  row1["name"] = "ip";
  row1["number"] = "0";
  row1["alias"] = "IP";
  row1["comment"] = "internet protocol, pseudo protocol number";
  row2["name"] = "icmp";
  row2["number"] = "1";
  row2["alias"] = "ICMP";
  row2["comment"] = "internet control message protocol";
  row3["name"] = "tcp";
  row3["number"] = "6";
  row3["alias"] = "TCP";
  row3["comment"] = "transmission control protocol";

  return {row1, row2, row3};
}

QueryData genRows(EventSubscriberPlugin* sub) {
  auto vtc = new VirtualTableContent();
  QueryContext context(vtc);
  RowGenerator::pull_type generator(std::bind(&EventSubscriberPlugin::genTable,
                                              sub,
                                              std::placeholders::_1,
                                              std::move(context)));

  QueryData results;
  if (!generator) {
    delete vtc;
    return results;
  }

  while (generator) {
    results.push_back(generator.get());
    generator();
  }
  delete vtc;
  return results;
}

} // namespace osquery
