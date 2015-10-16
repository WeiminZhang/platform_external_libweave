// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "examples/provider/wifi_manager.h"

#include <arpa/inet.h>
#include <linux/wireless.h>
#include <sys/ioctl.h>
#include <sys/wait.h>

#include <fstream>

#include <base/bind.h>
#include <weave/provider/task_runner.h>

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

}  // namespace

WifiImpl::WifiImpl(provider::TaskRunner* task_runner, bool force_bootstrapping)
    : force_bootstrapping_{force_bootstrapping}, task_runner_{task_runner} {
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
      int sockf_d = socket(AF_INET, SOCK_DGRAM, 0);
      CHECK_GE(sockf_d, 0) << strerror(errno);

      iwreq wreq = {};
      snprintf(wreq.ifr_name, sizeof(wreq.ifr_name), "wlan0");
      std::string essid(' ', IW_ESSID_MAX_SIZE + 1);
      wreq.u.essid.pointer = &essid[0];
      wreq.u.essid.length = essid.size();
      CHECK_GE(ioctl(sockf_d, SIOCGIWESSID, &wreq), 0) << strerror(errno);
      essid.resize(wreq.u.essid.length);
      close(sockf_d);

      if (ssid == essid)
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
    Error::AddTo(&error, FROM_HERE, "wifi", "timeout",
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
  force_bootstrapping_ = false;
  CHECK(!hostapd_started_);
  if (hostapd_started_) {
    ErrorPtr error;
    Error::AddTo(&error, FROM_HERE, "wifi", "busy", "Running Access Point.");
    task_runner_->PostDelayedTask(
        FROM_HERE, base::Bind(callback, base::Passed(&error)), {});
    return;
  }

  TryToConnect(ssid, passphrase, 0,
               base::Time::Now() + base::TimeDelta::FromMinutes(1), callback);
}

void WifiImpl::StartAccessPoint(const std::string& ssid) {
  if (hostapd_started_)
    return;

  // Release wlan0 interface.
  CHECK_EQ(0, std::system("nmcli nm wifi off"));
  CHECK_EQ(0, std::system("rfkill unblock wlan"));
  sleep(1);

  std::string hostapd_conf = "/tmp/weave_hostapd.conf";
  {
    std::ofstream ofs(hostapd_conf);
    ofs << "interface=wlan0" << std::endl;
    ofs << "channel=1" << std::endl;
    ofs << "ssid=" << ssid << std::endl;
  }

  CHECK_EQ(0, std::system(("hostapd -B -K " + hostapd_conf).c_str()));
  hostapd_started_ = true;

  for (size_t i = 0; i < 10; ++i) {
    if (0 == std::system("ifconfig wlan0 192.168.76.1/24"))
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
    ofs << "interface=wlan0" << std::endl;
    ofs << "dhcp-leasefile=" << dnsmasq_conf << ".leases" << std::endl;
  }

  CHECK_EQ(0, std::system(("dnsmasq --conf-file=" + dnsmasq_conf).c_str()));
}

void WifiImpl::StopAccessPoint() {
  base::IgnoreResult(std::system("pkill -f dnsmasq.*/tmp/weave"));
  base::IgnoreResult(std::system("pkill -f hostapd.*/tmp/weave"));
  CHECK_EQ(0, std::system("nmcli nm wifi on"));
  hostapd_started_ = false;
}

bool WifiImpl::HasWifiCapability() {
  return std::system("nmcli dev | grep ^wlan0") == 0;
}

}  // namespace examples
}  // namespace weave
