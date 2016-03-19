# libweave daemon examples

This directory contains examples implementation of `libweave` device daemons.

## Build

- build all examples

```
make all-examples
```

- build only the light daemon

```
make out/Debug/weave_daemon_light
```

## Pre-requisites

- enable user-service-publishing in avahi daemon

set `disable-user-service-publishing=no` in `/etc/avahi/avahi-daemon.conf`

- restart avahi daemon

```
sudo service avahi-daemon restart
```

## Provisioning

### Generate registration tickets
- go to the [OAuth 2.0 Playground](https://developers.google.com/oauthplayground/)
  - `Step 1`: enter the Weave API scope `https://www.googleapis.com/auth/weave.app` and click to `Authorize APIs`
  - `Step 2`: click `Exchange authorization code for tokens`
  - `Step 3`:
    - set `HTTP Method`: `POST`
    - set `Request URI`: `https://www.googleapis.com/weave/v1/registrationTickets`
    - click `Enter request body`: `{"userEmail": "me"}`
    - click `Send the request`
  - The `Response` contains a new `registrationTicket` resource.

```
{
  "userEmail": "user@google.com",
  "kind": "weave#registrationTicket",
  "expirationTimeMs": "1443204934855",
  "deviceId": "0f8a5ff5-1ef0-ec39-f9d8-66d1caeb9e3d",
  "creationTimeMs": "1443204694855",
   "id": "93019287-6b26-04a0-22ee-d55ad23a4226"
}
```

- Note: the ticket expires after a few minutes

### Provision the device

- start the daemon with the `registrationTicket` id.

```
sudo out/Debug/weave_daemon_sample --registration_ticket=93019287-6b26-04a0-22ee-d55ad23a4226
```

- the daemon outputs the path to its configuration file and its deviceId

```
Saving settings to /var/lib/weave/weave_settings_XXXXX_config.json
Device registered: 0f8a5ff5-1ef0-ec39-f9d8-66d1caeb9e3d
```

- the device id matches the `cloud_id` in the configuration

```
$ sudo grep "cloud_id" /var/lib/weave/weave_settings_[XXXXX]_config.json
  "cloud_id": 0f8a5ff5-1ef0-ec39-f9d8-66d1caeb9e3d
```

- verify that that the device is online in the
  [Weave Developers Console](https://weave.google.com/console/),
  the [Weave Android app](https://play.google.com/apps/testing/com.google.android.apps.weave.management)
  or
  the [Weave Chrome app](https://chrome.google.com/webstore/detail/weave-device-manager/pcdgflbjckpjmlofgopidgdfonmnodfm).

### Send Commands

- go to the [Weave Developers Console](https://weave.google.com/console/),
- click `Your devices`
- select your device to show the `Device details` page.
- in the `Available commands` click `_sample.hello`
- set command parameters
- click `RUN COMMAND`
- verify the command is handled correctly by looking at the daemon logs.
