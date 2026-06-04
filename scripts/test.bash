#!/usr/bin/bash
# SPDX-License-Identifier: AGPL-3.0-or-later

readonly tmp_root="${TMPDIR:-/dev/shm}/lone/test"

remove-tree() {
  local tree="${tmp_root}${1:+/${1}}"

  # target is always the tmp_root tor a path inside it
  # only .. in the argument can escape it
  if [[ "${1-}" == *..* ]]; then
    >&2 printf 'remove-tree: refusing to remove %q\n' "${tree}"
    return 1
  fi

  rm -rf -- "${tree}"
}

if [[ "${1}" = --clean ]]; then
  remove-tree
  exit 0
fi

suite="${1}"
prefix="${2%/}"
name="${3}"

[[ -d "${suite}" ]] && suite="$(cd "${suite}" && pwd)"

run="${prefix##*/}/$$"
tmp="${tmp_root}/${run}"
remove-tree "${run}"
trap 'remove-tree "${run}"' EXIT

if [[ -n "${prefix}" && -d "${prefix}" ]]; then
  prefix="$(cd "${prefix}" && pwd)"
  while IFS= read -r -d '' directory; do
    PATH="${directory}:${PATH}"
  done < <(find "${prefix}" -type f -executable -printf '%h\0' | sort -uz)
  export PATH
fi

code=0

readonly default_timeout=60
max_jobs="${LONE_TEST_JOBS:-$(( 2 * $(nproc 2>/dev/null || echo 1) ))}"
[[ "${max_jobs}" =~ ^[1-9][0-9]*$ ]] || max_jobs=1
readonly max_jobs

declare -A styles tests pid_to_name

# C tests in source/tests/
# have matching exit codes
declare -ra outcomes=(PASS FAIL SKIP ERROR)   # exit code -> outcome word
declare -A  outcome_code                      # outcome word -> exit code
for outcome_index in "${!outcomes[@]}"; do
  outcome_code["${outcomes[outcome_index]}"]="${outcome_index}"
done

declare -rA outcome_color=([PASS]=2 [FAIL]=1 [SKIP]=3 [ERROR]=1)

diff_color=never

if [[ -t 1 ]]; then
  diff_color=always
  styles=(
    [reset]="$(tput sgr0)"
    [test.name]="$(tput setaf 4)"
    [test.executable]="$(tput dim)$(tput setaf 7)"
    [report.total]="$(tput setaf 4)"
  )
  for outcome in "${outcomes[@]}"; do
    styles[test."${outcome}"]="$(tput rev)$(tput setaf "${outcome_color["${outcome}"]}")"
    styles[report."${outcome}"]="$(tput setaf "${outcome_color["${outcome}"]}")"
  done
  styles[test.PASS]="$(tput setaf 2)"   # PASS line is not reverse-video
fi

compare() {
  [[ ! -r "${1}" ]] && return 0
  [[ ! -r "${2}" ]] && return 2

  local expected actual expected_code actual_code
  IFS= read -rd '' expected < "${1}"; expected_code="${?}"
  IFS= read -rd '' actual   < "${2}"; actual_code="${?}"

  if (( expected_code == 1 && actual_code == 1 )); then
    [[ "${expected}" == "${actual}" ]] && return 0
  else
    cmp -s "${1}" "${2}" && return 0
  fi

  diff \
    --color="${diff_color}" \
    --new-file --side-by-side --suppress-common-lines \
    --label=expected "${1}" \
    --label=got      "${2}"
}

compare-status() {
  local expected=0
  if [[ -r "${1}" ]]; then
    expected="$(< "${1}")"
  fi

  local got
  if [[ -r "${2}" ]]; then
    got="$(< "${2}")"
  else
    >&2 printf "Unreadable status file: %s\n" "${2}"
    return 2
  fi

  if [[ "${expected}" = "${got}" ]]; then
    return 0
  else
    printf "Expected status: %s\tReturned status: %s\n" "${expected}" "${got}"
    return 1
  fi
}

test-script() {
  local name="${1}"
  local test="${2}"
  local time_limit="${3}"
  local script="${test}"/script

  if [[ ! -x "${script}" ]]; then
    report-test-result "${name}" "" SKIP
    return 2
  fi

  local actual="${tmp}/${name}"
  local work="${actual}/work"
  mkdir -p "${work}"

  local result=PASS
  ( cd "${work}" && timeout "${time_limit}" "${script}" ) > "${actual}"/output 2> "${actual}"/error || result=FAIL

  report-test-result "${name}" "${script}" "${result}"

  if [[ "${result}" == FAIL ]]; then
    [[ -s "${actual}"/error  ]] && cat "${actual}"/error >&2
    [[ -s "${actual}"/output ]] && cat "${actual}"/output
  fi

  result-to-code "${result}"
}

resolve-attributes() {
  local test_case="${1}"

  # Resolves into the caller's protocol, executable and time_limit locals,
  # walking up to the suite root in a single pass.
  protocol="" executable="" time_limit=""

  while true; do
    [[ -z "${protocol}"   && -r "${test_case}/protocol"   && ! -x "${test_case}/protocol"   ]] && read -r protocol   < "${test_case}/protocol"
    [[ -z "${executable}" && -r "${test_case}/executable" && ! -x "${test_case}/executable" ]] && read -r executable < "${test_case}/executable"
    [[ -z "${time_limit}" && -r "${test_case}/timeout"    && ! -x "${test_case}/timeout"    ]] && read -r time_limit < "${test_case}/timeout"

    [[ -n "${protocol}" && -n "${executable}" && -n "${time_limit}" ]] && break
    [[ "${test_case}" == "${suite}" || "${test_case}" != */* ]] && break
    test_case="${test_case%/*}"
  done
}

execute-test() {
  local name="${1}"
  local test="${2}"
  local executable="${3}"
  local input="${4}"
  local time_limit="${5}"

  if [[ ! -x "${executable}" ]]; then
    return 3
  fi

  local program_name="${executable##*/}"
  if [[ -r "${test}"/program-name ]]; then
    read -r program_name < "${test}"/program-name
  fi

  local -a arguments=()
  if [[ -r "${test}"/arguments ]]; then
    readarray -t arguments < "${test}"/arguments
  fi

  local -a environment=()
  if [[ -r "${test}"/environment ]]; then
    readarray -t environment < "${test}"/environment
  fi

  local actual="${tmp}/${name}"

  timeout "${time_limit}" env --argv0 "${program_name}" --ignore-environment "${environment[@]}" "${executable}" "${arguments[@]}" < "${input}" > "${actual}/output" 2> "${actual}/error"
  printf '%d\n' "${?}" > "${actual}/status"
}

compare-outputs() {
  local test="${1}"
  local actual="${2}"
  local result=0

  compare        "${test}"/output "${actual}"/output  || result=1
  compare        "${test}"/error  "${actual}"/error   || result=1
  compare-status "${test}"/status "${actual}"/status  || result=1

  return "${result}"
}

result-to-code() {
  local outcome="${1:-ERROR}"
  return "${outcome_code["${outcome}"]:-3}"
}

test-executable() {
  local name="${1}"
  local test="${2}"
  local executable="${3}"
  local time_limit="${4}"

  local input="${test}"/input
  if [[ ! -r "${input}" ]]; then
    input=/dev/null
  fi

  execute-test "${name}" "${test}" "${executable}" "${input}" "${time_limit}"
  if [[ "${?}" -eq 3 ]]; then
    report-test-result "${name}" "${executable}" SKIP
    return 2
  fi

  local actual="${tmp}/${name}"
  local report_file="${actual}/report"
  local result=PASS

  compare-outputs "${test}" "${actual}" > "${actual}/diff" || result=FAIL

  report-test-result "${name}" "${executable}" "${result}" > "${report_file}"
  [[ "${result}" == FAIL ]] && cat "${actual}/diff" >> "${report_file}"
  printf '%s\n' "$(< "${report_file}")"

  result-to-code "${result}"
}

test-lone-test() {
  local name="${1}"
  local test="${2}"
  local executable="${3}"
  local time_limit="${4}"

  execute-test "${name}" "${test}" "${executable}" /dev/null "${time_limit}"
  if [[ "${?}" -eq 3 ]]; then
    report-test-result "${name}" "${executable}" SKIP
    return 2
  fi

  local actual="${tmp}/${name}"
  local report_file="${actual}/report"

  local aggregate status
  status="$(< "${actual}/status")"
  aggregate="${outcomes[status]:-ERROR}"

  local -A subtotals=([PASS]=0 [FAIL]=0 [SKIP]=0 [ERROR]=0)
  local current_test="" subtest_result bucket line

  while IFS= read -r line; do
    if [[ "${line}" =~ ^TEST\ (.+)$ ]]; then
      current_test="${BASH_REMATCH[1]}"
    elif [[ "${line}" =~ ^$'\t'RESULT\ (.+)$ ]]; then
      subtest_result="${BASH_REMATCH[1]}"
      bucket="${subtest_result}"
      [[ -v subtotals["${bucket}"] ]] || bucket=ERROR
      subtotals["${bucket}"]=$(( subtotals["${bucket}"] + 1 ))
      { printf "\t"; report-test-result "${current_test}" "" "${subtest_result}"; } >> "${report_file}"
    fi
  done < "${actual}/output"

  (( subtotals["${aggregate}"] )) || subtotals["${aggregate}"]=1

  report-test-result "${name}" "${executable}" "${aggregate}" >> "${report_file}"
  printf '%s\n' "$(<"${report_file}")"

  local outcome
  for outcome in "${outcomes[@]}"; do
    printf '%s %d\n' "${outcome}" "${subtotals["${outcome}"]}"
  done > "${actual}/subtotals"

  result-to-code "${aggregate}"
}

classify-tests() {
  local test_suite="${1}"
  local prefix="${2}"
  local test_case test_name protocol executable time_limit resolved

  while IFS= read -r -d '' test_case; do
    resolve-attributes "${test_case}"
    : "${protocol:=standard}"
    : "${time_limit:=${default_timeout}}"

    test_name="${test_case#"${test_suite}"/}"
    resolved="${executable:+${prefix}/${executable}}"

    case "${protocol}" in
      standard)  standard_tests+=("${test_name}:${resolved}:${time_limit}")  ;;
      script)    script_tests+=("${test_name}:${time_limit}")                ;;
      lone/test) lone_tests+=("${test_name}:${resolved}:${time_limit}")      ;;
      *)         tests["${test_name}"]=3                                     ;;
    esac
  done < <(find "${test_suite}" -mindepth 1 -type d -links 2 -print0)
}

run-test() {
  local name="${1}"
  local test_case="${2}"
  local executable="${3}"
  local protocol="${4}"
  local time_limit="${5}"
  local pid

  case "${protocol}" in
    standard)  test-executable "${name}" "${test_case}" "${executable}" "${time_limit}" & ;;
    lone/test) test-lone-test  "${name}" "${test_case}" "${executable}" "${time_limit}" & ;;
    script)    test-script     "${name}" "${test_case}" "${time_limit}"                 & ;;
    *)
      tests["${name}"]=3
      return
      ;;
  esac

  pid="${!}"
  pid_to_name["${pid}"]="${name}"

  while (( ${#pid_to_name[@]} >= max_jobs )); do
    reap-test
  done
}

run-all-tests() {
  local test_suite="${1%/}"
  local prefix="${2%/}"
  local entry test_name executable time_limit rest

  local -a standard_tests=() script_tests=() lone_tests=()
  classify-tests "${test_suite}" "${prefix}"

  local -a tmp_dirs=()
  for entry in "${standard_tests[@]}" "${lone_tests[@]}" "${script_tests[@]}"; do
    tmp_dirs+=("${tmp}/${entry%%:*}")
  done
  (( ${#tmp_dirs[@]} )) && mkdir -p "${tmp_dirs[@]}"

  for entry in "${standard_tests[@]}"; do
    test_name="${entry%%:*}"
    rest="${entry#*:}"
    executable="${rest%%:*}"
    time_limit="${rest#*:}"
    run-test "${test_name}" "${test_suite}/${test_name}" "${executable}" standard "${time_limit}"
  done

  for entry in "${lone_tests[@]}"; do
    test_name="${entry%%:*}"
    rest="${entry#*:}"
    executable="${rest%%:*}"
    time_limit="${rest#*:}"
    run-test "${test_name}" "${test_suite}/${test_name}" "${executable}" lone/test "${time_limit}"
  done

  for entry in "${script_tests[@]}"; do
    test_name="${entry%%:*}"
    time_limit="${entry#*:}"
    run-test "${test_name}" "${test_suite}/${test_name}" "" script "${time_limit}"
  done

  collect-parallel-results
}

reap-test() {
  local pid code

  wait -n -p pid
  code="${?}"

  [[ -z "${pid}" ]] && return
  [[ -v pid_to_name["${pid}"] ]] || return
  tests["${pid_to_name[${pid}]}"]="${code}"
  unset "pid_to_name[${pid}]"
}

collect-parallel-results() {
  while (( ${#pid_to_name[@]} > 0 )); do
    reap-test
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
  local reset="${styles[reset]}"

  printf "%s%s%s %s%s%s %s%s%s\n" \
         "${styles[test."${result}"]}" "${result}"     "${reset}" \
         "${styles[test.name]}"        "${name}"       "${reset}" \
         "${styles[test.executable]}"  "${executable}" "${reset}"
}

report() {
  local -A counts=([PASS]=0 [FAIL]=0 [SKIP]=0 [ERROR]=0)
  local name outcome word n total

  for name in "${!tests[@]}"; do
    local subtotals_file="${tmp}/${name}/subtotals"

    if [[ -r "${subtotals_file}" ]]; then
      while read -r word n; do
        counts["${word}"]=$(( counts["${word}"] + n ))
      done < "${subtotals_file}"
    else
      outcome="${outcomes[${tests[${name}]}]:-ERROR}"
      counts["${outcome}"]=$(( counts["${outcome}"] + 1 ))
      case "${outcome}" in
        ERROR)
          printf "Invalid test result: %s = %d\n" "${name}" "${tests[${name}]}"
          ;;
      esac
    fi
  done

  total=$(( counts[PASS] + counts[FAIL] ))

  if (( counts[FAIL] > 0 || counts[ERROR] > 0 )); then
    code=1
  fi

  local pass fail skip invalid
  pass="$(   stylize "${counts[PASS]}"  report.PASS)"
  fail="$(   stylize "${counts[FAIL]}"  report.FAIL)"
  total="$(  stylize "${total}"         report.total)"
  skip="$(   stylize "${counts[SKIP]}"  report.SKIP)"
  invalid="$(stylize "${counts[ERROR]}" report.ERROR)"

  printf "%s + %s = %s | %s + %s\n" \
         "${pass}" "${fail}" "${total}" "${skip}" "${invalid}"
}

run-single-test() {
  local test_name="${1}"
  local test_case="${2}"
  local prefix="${3}"
  local protocol executable time_limit resolved

  if [[ ! -d "${test_case}" ]]; then
    >&2 printf "Test not found: %s\n" "${test_name}"
    code=1
    return 1
  fi

  resolve-attributes "${test_case}"
  : "${protocol:=standard}"
  : "${time_limit:=${default_timeout}}"
  resolved="${executable:+${prefix}/${executable}}"

  mkdir -p "${tmp}/${test_name}"

  run-test "${test_name}" "${test_case}" "${resolved}" "${protocol}" "${time_limit}"
  collect-parallel-results
}

test-suite() {
  local root="${1%/}"
  local prefix="${2%/}"
  local name="${3}"

  if [[ -n "${name}" ]]; then
    run-single-test "${name}" "${root}"/"${name}" "${prefix}"
  else
    run-all-tests "${root}" "${prefix}"
  fi
}

test-suite "${suite}" "${prefix}" "${name}"
report
exit "${code}"
