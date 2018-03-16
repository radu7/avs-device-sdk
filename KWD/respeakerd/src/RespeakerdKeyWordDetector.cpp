/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *     http://aws.amazon.com/apache2.0/
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#include <memory>
#include <sstream>

#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/Memory/Memory.h>

#include "respeakerd/RespeakerdKeyWordDetector.h"

namespace alexaClientSDK {
namespace kwd {

using namespace avsCommon;
using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils;

static const std::string TAG("RespeakerdKeyWordDetector");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// The number of hertz per kilohertz.
static const size_t HERTZ_PER_KILOHERTZ = 1000;

/// The timeout to use for read calls to the SharedDataStream.
const std::chrono::milliseconds TIMEOUT_FOR_READ_CALLS = std::chrono::milliseconds(1000);

std::unique_ptr<RespeakerdKeyWordDetector> RespeakerdKeyWordDetector::create(
    std::shared_ptr<avsCommon::avs::AudioInputStream> stream,
    std::unordered_set<std::shared_ptr<KeyWordObserverInterface>> keyWordObservers,
    std::unordered_set<std::shared_ptr<KeyWordDetectorStateObserverInterface>> keyWordDetectorStateObservers) {
    if (!stream) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullStream"));
        return nullptr;
    }

    std::unique_ptr<RespeakerdKeyWordDetector> detector(new RespeakerdKeyWordDetector(
        stream,
        keyWordObservers,
        keyWordDetectorStateObservers));
    if (!detector->init()) {
        ACSDK_ERROR(LX("createFailed").d("reason", "initDetectorFailed"));
        return nullptr;
    }
    return detector;
}

RespeakerdKeyWordDetector::~RespeakerdKeyWordDetector() {
    m_isShuttingDown = true;
    if (m_detectionThread.joinable()) {
        m_detectionThread.join();
    }
}

RespeakerdKeyWordDetector::RespeakerdKeyWordDetector(
    std::shared_ptr<avsCommon::avs::AudioInputStream> stream,
    std::unordered_set<std::shared_ptr<KeyWordObserverInterface>> keyWordObservers,
    std::unordered_set<std::shared_ptr<KeyWordDetectorStateObserverInterface>> keyWordDetectorStateObservers) :
        AbstractKeywordDetector(keyWordObservers, keyWordDetectorStateObservers),
        m_stream{stream} {
    //emtpy
}

bool RespeakerdKeyWordDetector::init() {
    DBusError dbus_err;
    // initialise the errors
    dbus_error_init(&dbus_err);
    // connect to the bus and check for errors
    m_dbus_conn = dbus_bus_get(DBUS_BUS_SYSTEM, &dbus_err);
    if (dbus_error_is_set(&dbus_err)) {
        ACSDK_ERROR(LX("initFailed")
                    .d("reason", "getDBusConnectionFailed")
                    .d("detail reason", dbus_err.message));
        dbus_error_free(&dbus_err);
    }
    if (!m_dbus_conn) {
        ACSDK_ERROR(LX("initFailed").d("reason", "getDBusConnectionFailed-1"));
        return false;
    }

    dbus_bus_add_match(m_dbus_conn, "type='signal',interface='respeakerd.signal'", &dbus_err); // see signals from the given interface
    dbus_connection_flush(m_dbus_conn);
    if (dbus_error_is_set(&dbus_err)) {
        ACSDK_ERROR(LX("initFailed")
                    .d("reason", "DBusAddMatchFailed")
                    .d("detail reason", dbus_err.message));
        return false;
    }

    m_isShuttingDown = false;
    m_detectionThread = std::thread(&RespeakerdKeyWordDetector::detectionLoop, this);
    return true;
}

void RespeakerdKeyWordDetector::detectionLoop() {
    notifyKeyWordDetectorStateObservers(KeyWordDetectorStateObserverInterface::KeyWordDetectorState::ACTIVE);

    bool bus_alive = true;
    DBusMessage *msg;

    while (!m_isShuttingDown) {
        // block to wait until the bus can be read/write
        bus_alive = dbus_connection_read_write(m_dbus_conn, 0);
        if (!bus_alive) {
            notifyKeyWordDetectorStateObservers(KeyWordDetectorStateObserverInterface::KeyWordDetectorState::ERROR);
            ACSDK_ERROR(LX("readWriteFailed").d("reason", "bus_alive == FALSE"));
            break;
        }
        msg = dbus_connection_pop_message(m_dbus_conn);
        if (msg) {
            if (dbus_message_is_signal(msg, "respeakerd.signal", "trigger")) {
                notifyKeyWordObservers(
                    m_stream,
                    "anykeyword",
                    KeyWordObserverInterface::UNSPECIFIED_INDEX,
                    KeyWordObserverInterface::UNSPECIFIED_INDEX);
            }
            dbus_message_unref(msg);
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

}  // namespace kwd
}  // namespace alexaClientSDK
