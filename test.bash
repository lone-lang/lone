#!/usr/bin/bash

lone=${1:-./lone}
tests_directory=${2:-test}

code=0

declare -A style tests

if [[ -t 1 ]]; then
  style=(
    [reset]="$(tput sgr0)"
    [test.info]="$(tput setaf 4)"
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
                 "$("${command[@]}" < "${test}/input")"                                                      \
                 "${?}"                                                                                      \
                                                                                                             \
       1>&2) 2>&1)

  name="${style[test.info]}${name}${style[reset]}"
  local pass="${style[test.pass]}PASS${style[reset]}"
  local fail="${style[test.fail]}FAIL${style[reset]}"
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
  test-executable "${1}" "${2}" "${3}/${2}" &
  tests["${2}"]="${!}"
}

find-tests() {
  find "${1}" -type d -links 2 -print0
}

run-all-tests() {
  while IFS= read -r -d '' test_case; do
    run-test "${1}" "$(remove-prefix "${2}/" "${test_case}")" "${2}"
  done < <(find-tests "${2}")
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
         "${style[test.pass]}" "${pass}"  "${style[reset]}" \
         "${style[test.fail]}" "${fail}"  "${style[reset]}" \
         "${style[test.info]}" "${total}" "${style[reset]}"
}

run-all-tests "${lone}" "${tests_directory}"
collect-test-results
report
exit "${code}"
