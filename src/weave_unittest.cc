// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <weave/device.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <weave/provider/test/fake_task_runner.h>
#include <weave/provider/test/mock_bluetooth.h>
#include <weave/provider/test/mock_config_store.h>
#include <weave/provider/test/mock_dns_service_discovery.h>
#include <weave/provider/test/mock_http_client.h>
#include <weave/provider/test/mock_http_server.h>
#include <weave/provider/test/mock_network.h>
#include <weave/provider/test/mock_wifi.h>
#include <weave/test/mock_command.h>
#include <weave/test/mock_device.h>
#include <weave/test/unittest_utils.h>

#include "src/bind_lambda.h"

using testing::_;
using testing::AtLeast;
using testing::AtMost;
using testing::HasSubstr;
using testing::InSequence;
using testing::Invoke;
using testing::InvokeWithoutArgs;
using testing::MatchesRegex;
using testing::Mock;
using testing::Return;
using testing::ReturnRefOfCopy;
using testing::StartsWith;
using testing::StrictMock;
using testing::WithArgs;

namespace weave {

namespace {

using provider::HttpClient;
using provider::Network;
using provider::test::MockHttpClientResponse;
using test::CreateDictionaryValue;
using test::ValueToString;

const char kCommandDefs[] = R"({
  "base": {
    "reboot": {},
    "_shutdown": {
      "parameters": {},
      "results": {}
    }
  }
})";

const char kDeviceResource[] = R"({
  "kind": "clouddevices#device",
  "id": "CLOUD_ID",
  "channel": {
    "supportedType": "pull"
  },
  "deviceKind": "vendor",
  "modelManifestId": "ABCDE",
  "systemName": "",
  "name": "TEST_NAME",
  "displayName": "",
  "description": "Developer device",
  "stateValidationEnabled": true,
  "commandDefs":{
    "base": {
      "reboot": {
        "minimalRole": "user",
        "parameters": {"delay": "integer"},
        "results": {}
      },
      "shutdown": {
        "minimalRole": "user",
        "parameters": {},
        "results": {}
      }
    }
  },
  "state":{
    "base":{
      "firmwareVersion":"FIRMWARE_VERSION",
      "localAnonymousAccessMaxRole":"viewer",
      "localDiscoveryEnabled":true,
      "localPairingEnabled":true,
      "network":{
      }
    },
    "power": {"battery_level":44}
  }
})";

const char kRegistrationResponse[] = R"({
  "kind": "clouddevices#registrationTicket",
  "id": "TICKET_ID",
  "deviceId": "CLOUD_ID",
  "oauthClientId": "CLIENT_ID",
  "userEmail": "USER@gmail.com",
  "creationTimeMs": "1440087183738",
  "expirationTimeMs": "1440087423738"
})";

const char kRegistrationFinalResponse[] = R"({
  "kind": "clouddevices#registrationTicket",
  "id": "TICKET_ID",
  "deviceId": "CLOUD_ID",
  "oauthClientId": "CLIENT_ID",
  "userEmail": "USER@gmail.com",
  "robotAccountEmail": "ROBO@gmail.com",
  "robotAccountAuthorizationCode": "AUTH_CODE",
  "creationTimeMs": "1440087183738",
  "expirationTimeMs": "1440087423738"
})";

const char kAuthTokenResponse[] = R"({
  "access_token" : "ACCESS_TOKEN",
  "token_type" : "Bearer",
  "expires_in" : 3599,
  "refresh_token" : "REFRESH_TOKEN"
})";

const char kStateDefs[] = R"({"power": {"battery_level":"integer"}})";

const char kStateDefaults[] = R"({"power": {"battery_level":44}})";

MATCHER_P(MatchTxt, txt, "") {
  std::vector<std::string> txt_copy = txt;
  std::sort(txt_copy.begin(), txt_copy.end());
  std::vector<std::string> arg_copy = arg;
  std::sort(arg_copy.begin(), arg_copy.end());
  return (arg_copy == txt_copy);
}

template <class Map>
std::set<typename Map::key_type> GetKeys(const Map& map) {
  std::set<typename Map::key_type> result;
  for (const auto& pair : map)
    result.insert(pair.first);
  return result;
}

}  // namespace

class WeaveTest : public ::testing::Test {
 protected:
  void SetUp() override {}

  void ExpectRequest(HttpClient::Method method,
                     const std::string& url,
                     const std::string& json_response) {
    EXPECT_CALL(http_client_, SendRequest(method, url, _, _, _))
        .WillOnce(WithArgs<4>(Invoke([json_response](
            const HttpClient::SendRequestCallback& callback) {
          std::unique_ptr<provider::test::MockHttpClientResponse> response{
              new StrictMock<provider::test::MockHttpClientResponse>};
          EXPECT_CALL(*response, GetStatusCode())
              .Times(AtLeast(1))
              .WillRepeatedly(Return(200));
          EXPECT_CALL(*response, GetContentType())
              .Times(AtLeast(1))
              .WillRepeatedly(Return("application/json; charset=utf-8"));
          EXPECT_CALL(*response, GetData())
              .Times(AtLeast(1))
              .WillRepeatedly(Return(json_response));
          callback.Run(std::move(response), nullptr);
        })));
  }

  void InitConfigStore() {
    EXPECT_CALL(config_store_, SaveSettings("")).WillRepeatedly(Return());
  }

  void InitNetwork() {
    EXPECT_CALL(network_, AddConnectionChangedCallback(_))
        .WillRepeatedly(Invoke(
            [this](const provider::Network::ConnectionChangedCallback& cb) {
              network_callbacks_.push_back(cb);
            }));
    EXPECT_CALL(network_, GetConnectionState())
        .WillRepeatedly(Return(Network::State::kOffline));
  }

  void InitDnsSd() {
    EXPECT_CALL(dns_sd_, PublishService(_, _, _)).WillRepeatedly(Return());
    EXPECT_CALL(dns_sd_, StopPublishing("_privet._tcp")).WillOnce(Return());
  }

  void InitDnsSdPublishing(bool registered, const std::string& flags) {
    std::vector<std::string> txt{
        {"id=TEST_DEVICE_ID"},         {"flags=" + flags}, {"mmid=ABCDE"},
        {"services=developmentBoard"}, {"txtvers=3"},      {"ty=TEST_NAME"}};
    if (registered) {
      txt.push_back("gcd_id=CLOUD_ID");

      // During registration device may announce itself twice:
      // 1. with GCD ID but not connected (DB)
      // 2. with GCD ID and connected (BB)
      EXPECT_CALL(dns_sd_, PublishService("_privet._tcp", 11, MatchTxt(txt)))
          .Times(AtMost(1))
          .WillOnce(Return());

      txt[1] = "flags=BB";
    }

    EXPECT_CALL(dns_sd_, PublishService("_privet._tcp", 11, MatchTxt(txt)))
        .Times(AtMost(1))
        .WillOnce(Return());
  }

  void InitHttpServer() {
    EXPECT_CALL(http_server_, GetHttpPort()).WillRepeatedly(Return(11));
    EXPECT_CALL(http_server_, GetHttpsPort()).WillRepeatedly(Return(12));
    EXPECT_CALL(http_server_, GetRequestTimeout())
        .WillRepeatedly(Return(base::TimeDelta::Max()));
    EXPECT_CALL(http_server_, GetHttpsCertificateFingerprint())
        .WillRepeatedly(Return(std::vector<uint8_t>{1, 2, 3}));
    EXPECT_CALL(http_server_, AddHttpRequestHandler(_, _))
        .WillRepeatedly(Invoke(
            [this](const std::string& path_prefix,
                   const provider::HttpServer::RequestHandlerCallback& cb) {
              http_handlers_[path_prefix] = cb;
            }));
    EXPECT_CALL(http_server_, AddHttpsRequestHandler(_, _))
        .WillRepeatedly(Invoke(
            [this](const std::string& path_prefix,
                   const provider::HttpServer::RequestHandlerCallback& cb) {
              https_handlers_[path_prefix] = cb;
            }));
  }

  void InitDefaultExpectations() {
    InitConfigStore();
    InitNetwork();
    EXPECT_CALL(wifi_, StartAccessPoint(MatchesRegex("TEST_NAME.*prv")))
        .WillOnce(Return());
    InitHttpServer();
    InitDnsSd();
  }

  void StartDevice() {
    device_ = weave::Device::Create(&config_store_, &task_runner_,
                                    &http_client_, &network_, &dns_sd_,
                                    &http_server_, &wifi_, &bluetooth_);

    EXPECT_EQ((std::set<std::string>{
                  "/privet/info", "/privet/v3/pairing/cancel",
                  "/privet/v3/pairing/confirm", "/privet/v3/pairing/start"}),
              GetKeys(http_handlers_));
    EXPECT_EQ((std::set<std::string>{
                  "/privet/info", "/privet/v3/auth",
                  "/privet/v3/checkForUpdates", "/privet/v3/commandDefs",
                  "/privet/v3/commands/cancel", "/privet/v3/commands/execute",
                  "/privet/v3/commands/list", "/privet/v3/commands/status",
                  "/privet/v3/pairing/cancel", "/privet/v3/pairing/confirm",
                  "/privet/v3/pairing/start", "/privet/v3/setup/start",
                  "/privet/v3/setup/status", "/privet/v3/state"}),
              GetKeys(https_handlers_));

    device_->AddCommandDefinitionsFromJson(kCommandDefs);
    device_->AddStateDefinitionsFromJson(kStateDefs);
    device_->SetStatePropertiesFromJson(kStateDefaults, nullptr);

    task_runner_.Run();
  }

  void NotifyNetworkChanged(provider::Network::State state,
                            base::TimeDelta delay) {
    auto task = [this, state] {
      EXPECT_CALL(network_, GetConnectionState()).WillRepeatedly(Return(state));
      for (const auto& cb : network_callbacks_)
        cb.Run();
    };

    task_runner_.PostDelayedTask(FROM_HERE, base::Bind(task), delay);
  }

  std::map<std::string, provider::HttpServer::RequestHandlerCallback>
      http_handlers_;
  std::map<std::string, provider::HttpServer::RequestHandlerCallback>
      https_handlers_;

  StrictMock<provider::test::MockConfigStore> config_store_;
  StrictMock<provider::test::FakeTaskRunner> task_runner_;
  StrictMock<provider::test::MockHttpClient> http_client_;
  StrictMock<provider::test::MockNetwork> network_;
  StrictMock<provider::test::MockDnsServiceDiscovery> dns_sd_;
  StrictMock<provider::test::MockHttpServer> http_server_;
  StrictMock<provider::test::MockWifi> wifi_;
  StrictMock<provider::test::MockBluetooth> bluetooth_;

  std::vector<provider::Network::ConnectionChangedCallback> network_callbacks_;

  std::unique_ptr<weave::Device> device_;
};

TEST_F(WeaveTest, Mocks) {
  // Test checks if mock implements entire interface and mock can be
  // instantiated.
  test::MockDevice device;
  test::MockCommand command;
}

TEST_F(WeaveTest, StartMinimal) {
  InitConfigStore();
  device_ = weave::Device::Create(&config_store_, &task_runner_, &http_client_,
                                  &network_, nullptr, nullptr, &wifi_, nullptr);
}

TEST_F(WeaveTest, StartNoWifi) {
  InitConfigStore();
  InitNetwork();
  InitHttpServer();
  InitDnsSd();
  InitDnsSdPublishing(false, "CB");

  device_ = weave::Device::Create(&config_store_, &task_runner_, &http_client_,
                                  &network_, &dns_sd_, &http_server_, nullptr,
                                  &bluetooth_);
  device_->AddCommandDefinitionsFromJson(kCommandDefs);

  task_runner_.Run();
}

class WeaveBasicTest : public WeaveTest {
 public:
  void SetUp() override {
    WeaveTest::SetUp();

    InitDefaultExpectations();
    InitDnsSdPublishing(false, "DB");
  }
};

TEST_F(WeaveBasicTest, Start) {
  StartDevice();
}

TEST_F(WeaveBasicTest, Register) {
  EXPECT_CALL(network_, OpenSslSocket(_, _, _)).WillRepeatedly(Return());
  StartDevice();

  auto draft = CreateDictionaryValue(kDeviceResource);
  auto response = CreateDictionaryValue(kRegistrationResponse);
  response->Set("deviceDraft", draft->DeepCopy());
  ExpectRequest(HttpClient::Method::kPatch,
                "https://www.googleapis.com/weave/v1/registrationTickets/"
                "TICKET_ID?key=TEST_API_KEY",
                ValueToString(*response));

  response = CreateDictionaryValue(kRegistrationFinalResponse);
  response->Set("deviceDraft", draft->DeepCopy());
  ExpectRequest(HttpClient::Method::kPost,
                "https://www.googleapis.com/weave/v1/registrationTickets/"
                "TICKET_ID/finalize?key=TEST_API_KEY",
                ValueToString(*response));

  ExpectRequest(HttpClient::Method::kPost,
                "https://accounts.google.com/o/oauth2/token",
                kAuthTokenResponse);

  InitDnsSdPublishing(true, "DB");

  bool done = false;
  device_->Register("TICKET_ID", base::Bind([this, &done](ErrorPtr error) {
                      EXPECT_FALSE(error);
                      done = true;
                      task_runner_.Break();
                      EXPECT_EQ("CLOUD_ID", device_->GetSettings().cloud_id);
                    }));
  task_runner_.Run();
  EXPECT_TRUE(done);
}

class WeaveWiFiSetupTest : public WeaveTest {
 public:
  void SetUp() override {
    WeaveTest::SetUp();

    InitConfigStore();
    InitHttpServer();
    InitNetwork();
    InitDnsSd();

    EXPECT_CALL(network_, GetConnectionState())
        .WillRepeatedly(Return(provider::Network::State::kOnline));
  }
};

TEST_F(WeaveWiFiSetupTest, StartOnlineNoPrevSsid) {
  StartDevice();

  // Short disconnect.
  NotifyNetworkChanged(provider::Network::State::kOffline, {});
  NotifyNetworkChanged(provider::Network::State::kOnline,
                       base::TimeDelta::FromSeconds(10));
  task_runner_.Run();

  // Long disconnect.
  NotifyNetworkChanged(Network::State::kOffline, {});
  auto offline_from = task_runner_.GetClock()->Now();
  EXPECT_CALL(wifi_, StartAccessPoint(MatchesRegex("TEST_NAME.*prv")))
      .WillOnce(InvokeWithoutArgs([this, offline_from]() {
        EXPECT_GT(task_runner_.GetClock()->Now() - offline_from,
                  base::TimeDelta::FromMinutes(1));
        task_runner_.Break();
      }));
  task_runner_.Run();
}

// If device has previously configured WiFi it will run AP for limited time
// after which it will try to re-connect.
TEST_F(WeaveWiFiSetupTest, StartOnlineWithPrevSsid) {
  EXPECT_CALL(config_store_, LoadSettings())
      .WillRepeatedly(Return(R"({"last_configured_ssid": "TEST_ssid"})"));
  StartDevice();

  // Long disconnect.
  NotifyNetworkChanged(Network::State::kOffline, {});

  for (int i = 0; i < 5; ++i) {
    auto offline_from = task_runner_.GetClock()->Now();
    // Temporarily offline mode.
    EXPECT_CALL(wifi_, StartAccessPoint(MatchesRegex("TEST_NAME.*prv")))
        .WillOnce(InvokeWithoutArgs([this, &offline_from]() {
          EXPECT_GT(task_runner_.GetClock()->Now() - offline_from,
                    base::TimeDelta::FromMinutes(1));
          task_runner_.Break();
        }));
    task_runner_.Run();

    // Try to reconnect again.
    offline_from = task_runner_.GetClock()->Now();
    EXPECT_CALL(wifi_, StopAccessPoint())
        .WillOnce(InvokeWithoutArgs([this, offline_from]() {
          EXPECT_GT(task_runner_.GetClock()->Now() - offline_from,
                    base::TimeDelta::FromMinutes(5));
          task_runner_.Break();
        }));
    task_runner_.Run();
  }

  NotifyNetworkChanged(Network::State::kOnline, {});
  task_runner_.Run();
}

TEST_F(WeaveWiFiSetupTest, StartOfflineWithSsid) {
  EXPECT_CALL(config_store_, LoadSettings())
      .WillRepeatedly(Return(R"({"last_configured_ssid": "TEST_ssid"})"));
  EXPECT_CALL(network_, GetConnectionState())
      .WillRepeatedly(Return(Network::State::kOffline));

  auto offline_from = task_runner_.GetClock()->Now();
  EXPECT_CALL(wifi_, StartAccessPoint(MatchesRegex("TEST_NAME.*prv")))
      .WillOnce(InvokeWithoutArgs([this, &offline_from]() {
        EXPECT_GT(task_runner_.GetClock()->Now() - offline_from,
                  base::TimeDelta::FromMinutes(1));
        task_runner_.Break();
      }));

  StartDevice();
}

TEST_F(WeaveWiFiSetupTest, OfflineLongTimeWithNoSsid) {
  EXPECT_CALL(network_, GetConnectionState())
      .WillRepeatedly(Return(Network::State::kOffline));
  NotifyNetworkChanged(provider::Network::State::kOnline,
                       base::TimeDelta::FromHours(15));

  {
    InSequence s;
    auto time_stamp = task_runner_.GetClock()->Now();

    EXPECT_CALL(wifi_, StartAccessPoint(MatchesRegex("TEST_NAME.*prv")))
        .WillOnce(InvokeWithoutArgs([this, &time_stamp]() {
          EXPECT_LE(task_runner_.GetClock()->Now() - time_stamp,
                    base::TimeDelta::FromMinutes(1));
          time_stamp = task_runner_.GetClock()->Now();
        }));

    EXPECT_CALL(wifi_, StopAccessPoint())
        .WillOnce(InvokeWithoutArgs([this, &time_stamp]() {
          EXPECT_GT(task_runner_.GetClock()->Now() - time_stamp,
                    base::TimeDelta::FromMinutes(5));
          time_stamp = task_runner_.GetClock()->Now();
          task_runner_.Break();
        }));
  }

  StartDevice();
}

TEST_F(WeaveWiFiSetupTest, OfflineLongTimeWithSsid) {
  EXPECT_CALL(config_store_, LoadSettings())
      .WillRepeatedly(Return(R"({"last_configured_ssid": "TEST_ssid"})"));
  EXPECT_CALL(network_, GetConnectionState())
      .WillRepeatedly(Return(Network::State::kOffline));
  NotifyNetworkChanged(provider::Network::State::kOnline,
                       base::TimeDelta::FromHours(15));

  {
    InSequence s;
    auto time_stamp = task_runner_.GetClock()->Now();
    for (size_t i = 0; i < 10; ++i) {
      EXPECT_CALL(wifi_, StartAccessPoint(MatchesRegex("TEST_NAME.*prv")))
          .WillOnce(InvokeWithoutArgs([this, &time_stamp]() {
            EXPECT_GT(task_runner_.GetClock()->Now() - time_stamp,
                      base::TimeDelta::FromMinutes(1));
            time_stamp = task_runner_.GetClock()->Now();
          }));

      EXPECT_CALL(wifi_, StopAccessPoint())
          .WillOnce(InvokeWithoutArgs([this, &time_stamp]() {
            EXPECT_GT(task_runner_.GetClock()->Now() - time_stamp,
                      base::TimeDelta::FromMinutes(5));
            time_stamp = task_runner_.GetClock()->Now();
          }));
    }

    EXPECT_CALL(wifi_, StartAccessPoint(MatchesRegex("TEST_NAME.*prv")))
        .WillOnce(InvokeWithoutArgs([this]() { task_runner_.Break(); }));
  }

  StartDevice();
}

}  // namespace weave
