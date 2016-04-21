# libweave provider examples

This directory contains example implementations of `weave` system providers.

## Providers

-   `avahi_client.cc`

    -   implements: `weave::providerDnsServiceDiscovery`
    -   build-depends: libavahi-client
    -   run-depends: `avahi-daemon`

-   `bluez_client.cc`

    -   not-implemented

-   `curl_http_client.cc`

    -   implements: `weave::provider::HttpClient`
    -   build-depends: libcurl

-   `event_http_server.cc`

    -   implements: `weave::provider::HttpServer`
    -   build-depends: libevhtp

-   `event_network.cc`

    -   implements: `weave::provider::Network`
    -   build-depends: libevent

-   `event_task_runner.cc`

    -   implements: `weave::provider::TaskRunner`
    -   build-depends: libevent

-   `file_config_store.cc`

    -   implements: `weave::provider::ConfigStore`

-   `wifi_manager.cc`

    -   implements: `weave::provider::Wifi`
    -   build-depends: `weave::examples::EventNetworkImpl`
    -   run-depends: `network-manager`, `dnsmasq`, `hostapd`

## Note

-   The example providers are based on `libevent` and should be portable between
    most GNU/Linux distributions.
-   `weave::examples::WifiImpl` currently shells out to system command tools
    like `nmcli`, `dnsmasq`, `ifconfig` and `hostpad`.
