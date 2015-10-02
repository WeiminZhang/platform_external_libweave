// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/privet/privet_types.h"

#include <string>

#include <weave/enum_to_string.h>
#include <weave/export.h>
#include <weave/provider/network.h>

namespace weave {

namespace {

using privet::ConnectionState;
using privet::CryptoType;
using privet::SetupState;
using privet::WifiType;
using provider::Network;

const EnumToStringMap<PairingType>::Map kPairingTypeMap[] = {
    {PairingType::kPinCode, "pinCode"},
    {PairingType::kEmbeddedCode, "embeddedCode"},
};

const EnumToStringMap<ConnectionState::Status>::Map kConnectionStateMap[] = {
    {ConnectionState::kDisabled, "disabled"},
    {ConnectionState::kUnconfigured, "unconfigured"},
    {ConnectionState::kConnecting, "connecting"},
    {ConnectionState::kOnline, "online"},
    {ConnectionState::kOffline, "offline"},
};

const EnumToStringMap<SetupState::Status>::Map kSetupStateMap[] = {
    {SetupState::kNone, nullptr},
    {SetupState::kInProgress, "inProgress"},
    {SetupState::kSuccess, "success"},
};

const EnumToStringMap<WifiType>::Map kWifiTypeMap[] = {
    {WifiType::kWifi24, "2.4GHz"},
    {WifiType::kWifi50, "5.0GHz"},
};

const EnumToStringMap<CryptoType>::Map kCryptoTypeMap[] = {
    {CryptoType::kNone, "none"},
    {CryptoType::kSpake_p224, "p224_spake2"},
};

const EnumToStringMap<AuthScope>::Map kAuthScopeMap[] = {
    {AuthScope::kNone, "none"},
    {AuthScope::kViewer, "viewer"},
    {AuthScope::kUser, "user"},
    {AuthScope::kOwner, "owner"},
};

const EnumToStringMap<Network::State>::Map kNetworkStateMap[] = {
    {Network::State::kOffline, "offline"},
    {Network::State::kFailure, "failure"},
    {Network::State::kConnecting, "connecting"},
    {Network::State::kConnected, "connected"},
};

}  // namespace

template <>
LIBWEAVE_EXPORT EnumToStringMap<PairingType>::EnumToStringMap()
    : EnumToStringMap(kPairingTypeMap) {}

template <>
LIBWEAVE_EXPORT EnumToStringMap<ConnectionState::Status>::EnumToStringMap()
    : EnumToStringMap(kConnectionStateMap) {}

template <>
LIBWEAVE_EXPORT EnumToStringMap<SetupState::Status>::EnumToStringMap()
    : EnumToStringMap(kSetupStateMap) {}

template <>
LIBWEAVE_EXPORT EnumToStringMap<WifiType>::EnumToStringMap()
    : EnumToStringMap(kWifiTypeMap) {}

template <>
LIBWEAVE_EXPORT EnumToStringMap<CryptoType>::EnumToStringMap()
    : EnumToStringMap(kCryptoTypeMap) {}

template <>
LIBWEAVE_EXPORT EnumToStringMap<AuthScope>::EnumToStringMap()
    : EnumToStringMap(kAuthScopeMap) {}

template <>
LIBWEAVE_EXPORT EnumToStringMap<Network::State>::EnumToStringMap()
    : EnumToStringMap(kNetworkStateMap) {}

}  // namespace weave
