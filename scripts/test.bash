#!/usr/bin/bash
# SPDX-License-Identifier: AGPL-3.0-or-later

test_suite="${1}"
default_executable="${2}"
test_executables_path="${3}"

code=0

declare -A styles tests

if [[ -t 1 ]]; then
  styles=(
    [reset]="$(tput sgr0)"
    [test.name]="$(tput setaf 4)"
    [test.executable]="$(tput dim)$(tput setaf 7)"
    [test.PASS]="$(tput setaf 2)"
    [test.FAIL]="$(tput rev)$(tput setaf 1)"
    [test.SKIP]="$(tput rev)$(tput setaf 3)"
    [report.pass]="$(tput setaf 2)"
    [report.fail]="$(tput setaf 1)"
    [report.skip]="$(tput setaf 3)"
    [report.total]="$(tput setaf 4)"
    [report.invalid]="$(tput setaf 1)"
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
  [[ ! -r "${1}" ]] && return 0
  diff --side-by-side --suppress-common-lines --unidirectional-new-file "${1}" <(echo -E "${2}")
}

compare-status() {
  local expected=0

  if [[ -r "${1}" ]]; then
    expected="$(< "${1}")"
  fi

  if [[ "${2}" -eq "${expected}" ]]; then
    return 0
  else
    printf "Expected status: %d\tReturned status: %d\n" "${expected}" "${2}"
    return 1
  fi
}

test-script() {
  local name="${1}"
  local test="${2}"
  local script="${3}"

  local result code
  if "${script}"; then
    result=PASS
    code=0
  else
    result=FAIL
    code=1
  fi

  report-test-result "${name}" "${script}" "${result}"
  return "${code}"
}

test-executable() {
  local name="${1}"
  local test="${2}"
  local default_executable="${3}"
  local test_executables_path="${4}"

  local executable="${test}"/executable
  if [[ -r "${executable}" ]]; then
    executable="$(< "${executable}")"
    executable="${test_executables_path}"/"${executable}"
  else
    executable="${default_executable}"
  fi

  local result=SKIP
  if [[ ! -x "${executable}" ]]; then
    report-test-result "${name}" "${executable}" "${result}"
    return 2
  fi

  local input="${test}"/input
  if [[ ! -r "${input}" ]]; then
    input=/dev/null
  fi

  local -a arguments=()
  if [[ -r "${test}"/arguments ]]; then
    readarray -t arguments < "${test}"/arguments
  fi

  local -a environment=()
  if [[ -r "${test}"/environment ]]; then
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


  result=PASS

  compare        "${test}/output" "${output}" || result=FAIL
  compare        "${test}/error"  "${error}"  || result=FAIL
  compare-status "${test}/status" "${status}" || result=FAIL

  report-test-result "${name}" "${executable}" "${result}"

  case "${result}" in
    PASS) return 0; ;;
    FAIL) return 1; ;;
    SKIP) return 2; ;;
    *)    return 3; ;;
  esac
}

run-test() {
  local test_name="${1}"
  local test_case="${2}"
  local default_executable="${3}"
  local test_executables_path="${4}"

  local test_script="${test_case}"/script
  if [[ -x "${test_script}" ]]; then
    test-script "${test_name}" "${test_case}" "${test_script}" &
  else
    test-executable "${test_name}" "${test_case}" "${default_executable}" "${test_executables_path}" &
  fi

  tests["${test_name}"]="${!}"
}

find-tests() {
  local test_suite="${1}"

  find "${test_suite}" -type d -links 2 -print0
}

run-all-tests() {
  local test_suite="${1}"
  local default_executable="${2}"
  local test_executables_path="${3}"
  local test_name

  while IFS= read -r -d '' test_case; do
    test_name="$(remove-prefix "${test_suite}"/ "${test_case}")"
    run-test "${test_name}" "${test_case}" "${default_executable}" "${test_executables_path}"
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

stylize() {
  local text="${1}"
  local style="${2}"

  printf "%s%s%s\n" \
         "${styles["${style}"]}" \
         "${text}" \
         "${styles[reset]}"
}

report-test-result() {
  local name="${1}"
  local executable="${2}"
  local result="${3}"

  result="$(    stylize "${result}"     test."${result}")"
  name="$(      stylize "${name}"       test.name)"
  executable="$(stylize "${executable}" test.executable)"

  printf "%s %s %s\n" "${result}" "${name}" "${executable}"
}

report() {
  local total=0 pass=0 fail=0 skip=0 invalid=0

  for name in "${!tests[@]}"; do
    case "${tests[${name}]}" in
      0)
        pass=$((pass + 1))
        ;;
      1)
        fail=$((fail + 1))
        code=1
        ;;
      2)
        skip=$((skip + 1))
        ;;
      *)
        invalid=$((invalid + 1))
        printf "Invalid test result: %s = %d\n" "${name}" "${tests[${name}]}"
        ;;
    esac
  done

  total=$((pass + fail))

  pass="$(   stylize "${pass}"    report.pass)"
  fail="$(   stylize "${fail}"    report.fail)"
  total="$(  stylize "${total}"   report.total)"
  skip="$(   stylize "${skip}"    report.skip)"
  invalid="$(stylize "${invalid}" report.invalid)"

  printf "%s + %s = %s | %s + %s\n" \
         "${pass}" "${fail}" "${total}" "${skip}" "${invalid}"
}

run-all-tests "${test_suite}" "${default_executable}" "${test_executables_path}"
collect-test-results
report
exit "${code}"
