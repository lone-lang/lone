#!/usr/bin/bash

directory="${1}"
linked="$(readlink "${directory}")"
result="${?}"

if [[ "${result}" -eq 0 && ! -d "${linked}" ]]; then
  mkdir -p "${linked}"
fi
