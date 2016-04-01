// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "examples/provider/wifi_manager.h"

#include <arpa/inet.h>
#include <dirent.h>
#include <linux/wireless.h>
#include <sys/ioctl.h>
#include <sys/wait.h>

#include <fstream>

#include <base/bind.h>
#include <weave/provider/task_runner.h>

#include "examples/provider/event_network.h"
#include "examples/provider/ssl_stream.h"

namespace weave {
namespace examples {

namespace {

int ForkCmd(const std::string& path, const std::vector<std::string>& args) {
  int pid = fork();
  if (pid != 0)
    return pid;

  std::vector<const char*> args_vector;
  args_vector.push_back(path.c_str());
  for (auto& i : args)
    args_vector.push_back(i.c_str());
  args_vector.push_back(nullptr);
  execvp(path.c_str(), const_cast<char**>(args_vector.data()));
  NOTREACHED();
  return 0;
}

int ForkCmdAndWait(const std::string& path,
                   const std::vector<std::string>& args) {
  int pid = ForkCmd(path, args);
  int status = 0;
  CHECK_EQ(pid, waitpid(pid, &status, 0));
  return status;
}

struct DirCloser {
  void operator()(DIR* dir) { closedir(dir); }
};

std::string FindWirelessInterface() {
  std::string sysfs_net{"/sys/class/net"};
  std::unique_ptr<DIR, DirCloser> net_dir{opendir(sysfs_net.c_str())};
  CHECK(net_dir);
  dirent* iface;
  while ((iface = readdir(net_dir.get()))) {
    auto path = sysfs_net + "/" + iface->d_name + "/wireless";
    std::unique_ptr<DIR, DirCloser> wireless_dir{opendir(path.c_str())};
    if (wireless_dir)
      return iface->d_name;
  }
  return "";
}

std::string GetSsid(const std::string& interface) {
  int sockf_d = socket(AF_INET, SOCK_DGRAM, 0);
  CHECK_GE(sockf_d, 0) << strerror(errno);
  iwreq wreq = {};
  CHECK_LE(interface.size(), sizeof(wreq.ifr_name));
  strncpy(wreq.ifr_name, interface.c_str(), sizeof(wreq.ifr_name));
  std::string essid(' ', IW_ESSID_MAX_SIZE + 1);
  wreq.u.essid.pointer = &essid[0];
  wreq.u.essid.length = essid.size();
  if (ioctl(sockf_d, SIOCGIWESSID, &wreq) >= 0)
    essid.resize(wreq.u.essid.length);
  else
    essid.clear();
  close(sockf_d);
  return essid;
}

bool CheckFreq(const std::string& interface, double start, double end) {
  int sockf_d = socket(AF_INET, SOCK_DGRAM, 0);
  CHECK_GE(sockf_d, 0) << strerror(errno);
  iwreq wreq = {};
  CHECK_LE(interface.size(), sizeof(wreq.ifr_name));
  strncpy(wreq.ifr_name, interface.c_str(), sizeof(wreq.ifr_name));

  iw_range range = {};
  wreq.u.data.pointer = &range;
  wreq.u.data.length = sizeof(range);

  bool result = false;
  if (ioctl(sockf_d, SIOCGIWRANGE, &wreq) >= 0) {
    for (size_t i = 0; !result && i < range.num_frequency; ++i) {
      double freq = range.freq[i].m * std::pow(10., range.freq[i].e);
      if (start <= freq && freq <= end)
        result = true;
    }
  }

  close(sockf_d);
  return result;
}

}  // namespace

WifiImpl::WifiImpl(provider::TaskRunner* task_runner, EventNetworkImpl* network)
  : task_runner_{task_runner}, network_{network}, iface_{FindWirelessInterface()} {
  CHECK(!iface_.empty()) <<  "WiFi interface not found";
  CHECK(IsWifi24Supported() || IsWifi50Supported());
  CHECK_EQ(0u, getuid())
      << "\nWiFi manager expects root access to control WiFi capabilities";
  StopAccessPoint();
}
WifiImpl::~WifiImpl() {
  StopAccessPoint();
}

void WifiImpl::TryToConnect(const std::string& ssid,
                            const std::string& passphrase,
                            int pid,
                            base::Time until,
                            const DoneCallback& callback) {
  if (pid) {
    int status = 0;
    if (pid == waitpid(pid, &status, WNOWAIT)) {
      if (ssid == GetSsid(iface_))
        return task_runner_->PostDelayedTask(FROM_HERE,
                                             base::Bind(callback, nullptr), {});
      pid = 0;  // Try again.
    }
  }

  if (pid == 0) {
    pid = ForkCmd("nmcli",
                  {"dev", "wifi", "connect", ssid, "password", passphrase});
  }

  if (base::Time::Now() >= until) {
    ErrorPtr error;
    Error::AddTo(&error, FROM_HERE, "timeout",
                 "Timeout connecting to WiFI network.");
    task_runner_->PostDelayedTask(
        FROM_HERE, base::Bind(callback, base::Passed(&error)), {});
    return;
  }

  task_runner_->PostDelayedTask(
      FROM_HERE,
      base::Bind(&WifiImpl::TryToConnect, weak_ptr_factory_.GetWeakPtr(), ssid,
                 passphrase, pid, until, callback),
      base::TimeDelta::FromSeconds(1));
}

void WifiImpl::Connect(const std::string& ssid,
                       const std::string& passphrase,
                       const DoneCallback& callback) {
  network_->SetSimulateOffline(false);
  if (!hostapd_ssid_.empty()) {
    ErrorPtr error;
    Error::AddTo(&error, FROM_HERE, "busy", "Running Access Point.");
    task_runner_->PostDelayedTask(
        FROM_HERE, base::Bind(callback, base::Passed(&error)), {});
    return;
  }

  TryToConnect(ssid, passphrase, 0,
               base::Time::Now() + base::TimeDelta::FromMinutes(1), callback);
}

void WifiImpl::StartAccessPoint(const std::string& ssid) {
  CHECK(hostapd_ssid_.empty());

  // Release wifi interface.
  CHECK_EQ(0, ForkCmdAndWait("nmcli", {"nm", "wifi",  "off"}));
  CHECK_EQ(0, ForkCmdAndWait("rfkill", {"unblock", "wlan"}));
  sleep(1);

  std::string hostapd_conf = "/tmp/weave_hostapd.conf";
  {
    std::ofstream ofs(hostapd_conf);
    ofs << "interface=" << iface_ << std::endl;
    ofs << "channel=1" << std::endl;
    ofs << "ssid=" << ssid << std::endl;
  }

  CHECK_EQ(0, ForkCmdAndWait("hostapd", {"-B", "-K", hostapd_conf}));
  hostapd_ssid_ = ssid;

  for (size_t i = 0; i < 10; ++i) {
    if (0 == ForkCmdAndWait("ifconfig", {iface_, "192.168.76.1/24"}))
      break;
    sleep(1);
  }

  std::string dnsmasq_conf = "/tmp/weave_dnsmasq.conf";
  {
    std::ofstream ofs(dnsmasq_conf.c_str());
    ofs << "port=0" << std::endl;
    ofs << "bind-interfaces" << std::endl;
    ofs << "log-dhcp" << std::endl;
    ofs << "dhcp-range=192.168.76.10,192.168.76.100" << std::endl;
    ofs << "interface=" << iface_ << std::endl;
    ofs << "dhcp-leasefile=" << dnsmasq_conf << ".leases" << std::endl;
  }

  CHECK_EQ(0, ForkCmdAndWait("dnsmasq", {"--conf-file=" + dnsmasq_conf}));
}

void WifiImpl::StopAccessPoint() {
  base::IgnoreResult(ForkCmdAndWait("pkill", {"-f", "dnsmasq.*/tmp/weave"}));
  base::IgnoreResult(ForkCmdAndWait("pkill", {"-f", "hostapd.*/tmp/weave"}));
  CHECK_EQ(0, ForkCmdAndWait("nmcli", {"nm", "wifi", "on"}));
  hostapd_ssid_.clear();
}

bool WifiImpl::HasWifiCapability() {
  return !FindWirelessInterface().empty();
}

bool WifiImpl::IsWifi24Supported() const {
  return CheckFreq(iface_, 2.4e9, 2.5e9);
};

bool WifiImpl::IsWifi50Supported() const {
  return CheckFreq(iface_, 4.9e9, 5.9e9);
};

std::string WifiImpl::GetConnectedSsid() const {
  return GetSsid(iface_);
}

}  // namespace examples
}  // namespace weave
