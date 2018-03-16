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

#ifndef ALEXA_CLIENT_SDK_KWD_RESPEAKERD_INCLUDE_RESPEAKERD_RESPEAKERDKEYWORDDETECTOR_H_
#define ALEXA_CLIENT_SDK_KWD_RESPEAKERD_INCLUDE_RESPEAKERD_RESPEAKERDKEYWORDDETECTOR_H_

#include <atomic>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <dbus/dbus.h>

#include <AVSCommon/Utils/AudioFormat.h>
#include <AVSCommon/AVS/AudioInputStream.h>
#include <AVSCommon/SDKInterfaces/KeyWordObserverInterface.h>
#include <AVSCommon/SDKInterfaces/KeyWordDetectorStateObserverInterface.h>

#include "KWD/AbstractKeywordDetector.h"

namespace alexaClientSDK {
namespace kwd {

class RespeakerdKeyWordDetector : public AbstractKeywordDetector {
public:


    /**
     * Creates a @c KittAiKeyWordDetector.
     *
     * @param stream The stream of audio data. This should be formatted in LPCM encoded with 16 bits per sample and
     * have a sample rate of 16 kHz. Additionally, the data should be in little endian format.
     * @param audioFormat The format of the audio data located within the stream.
     * @param keyWordObservers The observers to notify of keyword detections.
     * @param keyWordDetectorStateObservers The observers to notify of state changes in the engine.
     * @param resourceFilePath The path to the resource file.
     * @param kittAiConfigurations A vector of @c KittAiConfiguration objects that will be used to initialize the
     * engine and keywords.
     * @param audioGain This controls whether to increase (>1) or decrease (<1) input volume.
     * @param applyFrontEnd Whether to apply frontend audio processing.
     * @param msToPushPerIteration The amount of data in milliseconds to push to Kitt.ai at a time. Smaller sizes will
     * lead to less delay but more CPU usage. Additionally, larger amounts of data fed into the engine per iteration
     * might lead longer delays before receiving keyword detection events. This has been defaulted to 20 milliseconds
     * as it is a good trade off between CPU usage and recognition delay.
     * @return A new @c KittAiKeyWordDetector, or @c nullptr if the operation failed.
     * @see https://github.com/Kitt-AI/snowboy for more information regarding @c audioGain and @c applyFrontEnd.
     */
    static std::unique_ptr<RespeakerdKeyWordDetector> create(
        std::shared_ptr<avsCommon::avs::AudioInputStream> stream,
        std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::KeyWordObserverInterface>> keyWordObservers,
        std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::KeyWordDetectorStateObserverInterface>>
            keyWordDetectorStateObservers);

    /**
     * Destructor.
     */
    ~RespeakerdKeyWordDetector() override;

private:
    /**
     * Constructor.
     *
     * @param stream The stream of audio data. This should be formatted in LPCM encoded with 16 bits per sample and
     * have a sample rate of 16 kHz. Additionally, the data should be in little endian format.
     * @param audioFormat The format of the audio data located within the stream.
     * @param keyWordObservers The observers to notify of keyword detections.
     * @param keyWordDetectorStateObservers The observers to notify of state changes in the engine.
     * @param resourceFilePath The path to the resource file.
     * @param kittAiConfigurations A vector of @c KittAiConfiguration objects that will be used to initialize the
     * engine and keywords.
     * @param audioGain This controls whether to increase (>1) or decrease (<1) input volume.
     * @param applyFrontEnd Whether to apply frontend audio processing.
     * @param msToPushPerIteration The amount of data in milliseconds to push to Kitt.ai at a time. Smaller sizes will
     * lead to less delay but more CPU usage. Additionally, larger amounts of data fed into the engine per iteration
     * might lead longer delays before receiving keyword detection events. This has been defaulted to 20 milliseconds
     * as it is a good trade off between CPU usage and recognition delay.
     * @see https://github.com/Kitt-AI/snowboy for more information regarding @c audioGain and @c applyFrontEnd.
     */
    RespeakerdKeyWordDetector(
        std::shared_ptr<avsCommon::avs::AudioInputStream> stream,
        std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::KeyWordObserverInterface>> keyWordObservers,
        std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::KeyWordDetectorStateObserverInterface>>
            keyWordDetectorStateObservers);

    /**
     * Initializes the stream reader and kicks off a thread to read data from the stream. This function should only be
     * called once with each new @c KittAiKeyWordDetector.
     *
     * @param audioFormat The format of the audio data located within the stream.
     * @return @c true if the engine was initialized properly and @c false otherwise.
     */
    bool init();

    /// The main function that reads data and feeds it into the engine.
    void detectionLoop();

    /// The stream of audio data.
    const std::shared_ptr<avsCommon::avs::AudioInputStream> m_stream;

    /// Indicates whether the internal main loop should keep running.
    std::atomic<bool> m_isShuttingDown;

    /// Internal thread that reads audio from the buffer and feeds it to the Kitt.ai engine.
    std::thread m_detectionThread;

    DBusConnection *m_dbus_conn;
};

}  // namespace kwd
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_KWD_RESPEAKERD_INCLUDE_RESPEAKERD_RESPEAKERDKEYWORDDETECTOR_H_
