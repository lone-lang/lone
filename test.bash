#!/usr/bin/bash

lone=${1:-./lone}
tests_directory=${2:-test}

code=0

declare -A style tests

if [[ -t 1 ]]; then
  style=(
    [reset]="$(tput sgr0)"
    [test.name]="$(tput setaf 4)"
    [test.pass]="$(tput setaf 2)"
    [test.fail]="$(tput setaf 1)"
  )
fi

remove-prefix() {
  local prefix="${1}"
  shift
  for path in "$@"; do
    echo "${path#"${prefix}"}"
  done
}

compare() {
  [[ ! -f "${1}" ]] && return 0
  diff --side-by-side --suppress-common-lines --unidirectional-new-file "${1}" <(echo -E "${2}")
}

compare-status() {
  local expected=0

  if [[ -f "${1}" ]]; then
    expected="$(< "${1}")"
  fi

  if [[ "${2}" -eq "${expected}" ]]; then
    return 0
  else
    2>&1 printf "Expected status: %d\tReturned status:%d\n" "${expected}" "${2}"
    return 1
  fi
}

test-executable() {
  local executable="${1}"
  local name="${2}"
  local test="${3}"

  local arguments=''
  if [[ -f "${test}"/arguments ]]; then
    arguments="$(xargs < "${test}"/arguments)"
  fi

  local environment=''
  if [[ -f "${test}"/environment ]]; then
    environment="$(xargs < "${test}"/environment)"
  fi

  # https://stackoverflow.com/a/59592881
  {
    IFS=$'\n' read -r -d '' error;
    IFS=$'\n' read -r -d '' output;
    IFS=$'\n' read -r -d '' status;
  } < <((printf '\0%s\0%d\0' "$(env -i ${environment} "${executable}" ${arguments} < "${test}/input")" "${?}" 1>&2) 2>&1)

  local pass="${style[test.pass]}PASS${style[reset]}"
  local fail="${style[test.fail]}FAIL${style[reset]}"
  name="${style[test.name]}${name}${style[reset]}"

  local result="${pass}"
  compare        "${test}/output" "${output}" || result="${fail}"
  compare        "${test}/error"  "${error}"  || result="${fail}"
  compare-status "${test}/status" "${status}" || result="${fail}"

  printf "%s %s\n" "${result}" "${name}"

  if [[ "${result}" == "${pass}" ]]; then
    return 0
  else
    return 1
  fi
}

run-test() {
  test-executable "${1}" "${2}" "${3}/${2}"
  local returned="$?"

  if [[ "${returned}" -ne 0 ]]; then
    code="${returned}"
  fi

  tests["${2}"]="${returned}"
}

find-tests() {
  find "${1}" -type d -links 2 -print0
}

run-all-tests() {
  local failed=no

  while IFS= read -r -d '' test_case; do
    if ! run-test "${1}" "$(remove-prefix "${2}/" "${test_case}")" "${2}"; then
      failed=yes
    fi
  done < <(find-tests "${2}")

  if [[ "${failed}" == "no" ]]; then
    return 0
  else
    return 1
  fi
}

run-all-tests "${lone}" "${tests_directory}"
exit "${code}"
