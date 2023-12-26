#!/usr/bin/bash
# SPDX-License-Identifier: AGPL-3.0-or-later

lone=${1:-lone}
tests_directory=${2:-test}

code=0

declare -A style tests

if [[ -t 1 ]]; then
  style=(
    [reset]="$(tput sgr0)"
    [test.name]="$(tput setaf 4)"
    [test.executable]="$(tput setaf 6)"
    [test.total]="$(tput setaf 4)"
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
    printf "Expected status: %d\tReturned status: %d\n" "${expected}" "${2}"
    return 1
  fi
}

test-executable() {
  local executable="${1}"
  local name="${2}"
  local test="${3}"

  local input="${test}"/input
  if [[ ! -f "${input}" ]]; then
    input=/dev/null
  fi

  local -a arguments=()
  if [[ -f "${test}"/arguments ]]; then
    readarray -t arguments < "${test}"/arguments
  fi

  local -a environment=()
  if [[ -f "${test}"/environment ]]; then
    readarray -t environment < "${test}"/environment
  fi

  local -a command=(env -i "${environment[@]}" "${executable}" "${arguments[@]}")

  # https://stackoverflow.com/a/59592881
  {
    IFS=$'\n' read -r -d '' error;
    IFS=$'\n' read -r -d '' output;
    IFS=$'\n' read -r -d '' status;
  } < <(                                                                                                     \
                                                                                                             \
         (printf '\0%s\0%d\0'                                                                                \
                 "$("${command[@]}" < "${input}")"                                                           \
                 "${?}"                                                                                      \
                                                                                                             \
       1>&2) 2>&1)

  name="${style[test.name]}${name}${style[reset]}"
  executable="${style[test.executable]}${executable}${style[reset]}"
  local pass="${style[test.pass]}PASS${style[reset]}"
  local fail="${style[test.fail]}FAIL${style[reset]}"
  local result="${pass}"

  compare        "${test}/output" "${output}" || result="${fail}"
  compare        "${test}/error"  "${error}"  || result="${fail}"
  compare-status "${test}/status" "${status}" || result="${fail}"

  printf "%s %s %s\n" "${result}" "${executable}" "${name}"

  if [[ "${result}" == "${pass}" ]]; then
    return 0
  else
    return 1
  fi
}

run-test() {
  local default_executable="${1}"
  local test_name="${2}"
  local test_path="${3}"

  test-executable "${default_executable}" "${test_name}" "${test_path}" &
  tests["${test_name}"]="${!}"
}

find-tests() {
  local test_suite="${1}"

  find "${test_suite}" -type d -links 2 -print0
}

run-all-tests() {
  local default_executable="${1}"
  local test_suite="${2}"

  while IFS= read -r -d '' test_case; do
    run-test "${default_executable}" "$(remove-prefix "${test_suite}"/ "${test_case}")" "${test_case}"
  done < <(find-tests "${test_suite}")
}

collect-test-results() {
  local returned

  for test_name in "${!tests[@]}"; do
    wait "${tests[${test_name}]}"
    returned="${?}"

    tests["${test_name}"]="${returned}"

    if [[ "${returned}" -ne 0 ]]; then
      code="${returned}"
    fi
  done
}

report() {
  local total=0 pass=0 fail=0

  for name in "${!tests[@]}"; do
    total=$((total + 1))
    if [[ "${tests[${name}]}" -ne 0 ]]; then
      fail=$((fail + 1))
      code=1
    fi
  done

  pass=$((total - fail))

  printf "%s%d%s + %s%d%s = %s%d%s\n" \
         "${style[test.pass]}"  "${pass}"  "${style[reset]}" \
         "${style[test.fail]}"  "${fail}"  "${style[reset]}" \
         "${style[test.total]}" "${total}" "${style[reset]}"
}

run-all-tests "${lone}" "${tests_directory}"
collect-test-results
report
exit "${code}"
