#!/bin/bash
# Copyright 2016 The Weave Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

set -e

SCRIPT=$(readlink -f "$0")
THIRD_PARTY=$(dirname "${SCRIPT}")
cd "${THIRD_PARTY}"

OUT="cross"
DISTDIR="${OUT}/distfiles"

CROS_OVERLAY_URL="https://chromium.googlesource.com/chromiumos/overlays/chromiumos-overlay/+/master/chromeos"
CONF_SDK_LATEST="${CROS_OVERLAY_URL}/binhost/host/sdk_version.conf"

SDK_BUCKET="https://commondatastorage.googleapis.com/chromiumos-sdk"
BINPKG_BUCKET="https://commondatastorage.googleapis.com/chromeos-prebuilt"
CROS_BUCKET="https://commondatastorage.googleapis.com/chromeos-image-archive"

PKGS=(
  app-emulation/qemu
)
TARGETS=(
#  aarch64-cros-linux-gnu
  armv7a-cros-linux-gnueabi
  i686-pc-linux-gnu
#  mips-cros-linux-gnu
  mipsel-cros-linux-gnu
  x86_64-cros-linux-gnu
)
BOARDS=(
#  aarch64-generic-full
  amd64-generic-full
  arm-generic-full
#  mips-o32-generic-full
  mipsel-o32-generic-full
  x86-generic-full
)

usage() {
  cat <<EOF
Usage: get_cross.sh

Download cross-compilers for building & testing against other arches.
EOF
  exit 0
}

get_gitiles() {
  local url="$1" data
  data=$(curl -s "${url}?format=TEXT")
  echo "${data}" | base64 -d
}

json() {
  local file="$1" arg="$2"
  python <<EOF
import json
print(json.load(open("${file}"))${arg})
EOF
}

fetch() {
  local url=$1
  file="${2:-${DISTDIR}/${url##*/}}"
  if [[ ! -e ${file} ]]; then
    printf '[downloading] '
    mkdir -p "${DISTDIR}"
    wget "${url}" -O "${file}"
  fi
}

unpack() {
  local out="$1" file="$2"
  printf '[unpacking] '
  rm -rf "${out}"
  mkdir -p "${out}"
  tar xf "${file}" -C "${out}"
}

fetch_pkgs() {
  local pkg
  local sub_url url file manifest
  local out ver_file old_ver ver

  # Grab a few helper packages.
  printf 'Getting SDK manifest ... '
  sub_url="cros-sdk-${SDK_LATEST_VERSION}.tar.xz.Manifest"
  url="${SDK_BUCKET}/${sub_url}"
  fetch "${url}"
  manifest=${file}
  printf '%s\n' "${manifest}"

  for pkg in "${PKGS[@]}"; do
    printf 'Getting binpkg %s ... ' "${pkg}"
    ver=$(json "${manifest}" '["packages"]["app-emulation/qemu"][0][0]')
    sub_url="host/amd64/amd64-host/chroot-${SDK_LATEST_VERSION}/packages/${pkg}-${ver}.tbz2"
    url="${BINPKG_BUCKET}/${sub_url}"
    fetch "${url}"

    out="${OUT}/${pkg}"
    ver_file="${out}/.ver"
    old_ver=$(cat "${ver_file}" 2>/dev/null || :)
    if [[ "${old_ver}" != "${ver}" ]]; then
      unpack "${out}" "${file}"
      echo "${ver}" > "${ver_file}"
    fi

    printf '%s\n' "${ver}"
  done
}

fetch_toolchains() {
  local target
  local sub_url url file
  local out ver_file ver

  # Download the base toolchains.
  for target in "${TARGETS[@]}"; do
    printf 'Getting toolchain for %s ... ' "${target}"

    sub_url="${TC_PATH/\%(target)s/${target}}"
    url="${SDK_BUCKET}/${sub_url}"
    file="${DISTDIR}/${url##*/}"
    fetch "${url}"

    out="${OUT}/${target}"
    ver_file="${out}/.ver"
    ver=$(cat "${ver_file}" 2>/dev/null || :)
    if [[ "${ver}" != "${SDK_LATEST_VERSION}" ]]; then
      unpack "${out}" "${file}"
      echo "${SDK_LATEST_VERSION}" > "${ver_file}"
    fi

    printf '%s\n' "${sub_url}"
  done
}

fetch_sysroots() {
  local board
  local board_latest_url sub_url url file
  local out ver_file ver

  # Get the full sysroot.
  for board in "${BOARDS[@]}"; do
    printf 'Getting sysroot for %s ... ' "${board}"
    board_latest_url="${CROS_BUCKET}/${board}/LATEST-master"
    if ! board_ver=$(curl --fail -s "${board_latest_url}"); then
      echo 'error: not found'
      continue
    fi

    url="${CROS_BUCKET}/${board}/${board_ver}/sysroot_chromeos-base_chromeos-chrome.tar.xz"
    file="${DISTDIR}/${board}-${board_ver}-${url##*/}"
    fetch "${url}" "${file}"

    out="${OUT}/${board}"
    ver_file="${out}/.ver"
    ver=$(cat "${ver_file}" 2>/dev/null || :)
    if [[ "${ver}" != "${board_ver}" ]]; then
      unpack "${out}" "${file}"
      echo "${board_ver}" > "${ver_file}"
    fi

    printf '%s\n' "${board_ver}"
  done
}

main() {
  if [[ $# -ne 0 ]]; then
    usage
  fi

  # Get the current SDK versions.
  printf 'Getting CrOS SDK version ... '
  data=$(get_gitiles "${CONF_SDK_LATEST}")
  eval "${data}"
  echo "${SDK_LATEST_VERSION}"

  fetch_pkgs
  fetch_toolchains
  fetch_sysroots
}
main "$@"
