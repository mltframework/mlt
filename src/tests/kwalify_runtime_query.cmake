if(NOT DEFINED MLT_EXECUTABLE)
  message(FATAL_ERROR "Missing required variable: MLT_EXECUTABLE")
endif()
if(NOT DEFINED KWALIFY_EXECUTABLE)
  message(FATAL_ERROR "Missing required variable: KWALIFY_EXECUTABLE")
endif()
if(NOT DEFINED METASCHEMA_FILE)
  message(FATAL_ERROR "Missing required variable: METASCHEMA_FILE")
endif()
if(NOT DEFINED LIST_QUERY)
  message(FATAL_ERROR "Missing required variable: LIST_QUERY")
endif()
if(NOT DEFINED QUERY_PREFIX)
  message(FATAL_ERROR "Missing required variable: QUERY_PREFIX")
endif()

execute_process(
  COMMAND "${MLT_EXECUTABLE}" -query "${LIST_QUERY}"
  RESULT_VARIABLE list_result
  OUTPUT_VARIABLE list_output
  ERROR_VARIABLE list_error
)

if(NOT list_result EQUAL 0)
  message(FATAL_ERROR
    "Failed to query services from melt.\n"
    "Command: ${MLT_EXECUTABLE} -query ${LIST_QUERY}\n"
    "Exit code: ${list_result}\n"
    "stderr:\n${list_error}"
  )
endif()

string(REPLACE "\r\n" "\n" list_output "${list_output}")
string(REPLACE "\r" "\n" list_output "${list_output}")
string(REPLACE "\n" ";" list_lines "${list_output}")

set(service_names "")
set(service_list_headers
  filters
  filter
  transitions
  transition
  links
  link
  consumers
  consumer
  producers
  producer
)
foreach(line IN LISTS list_lines)
  string(STRIP "${line}" line)
  if(line STREQUAL "")
    continue()
  endif()

  if(line MATCHES "^[-*][ \\t]+([A-Za-z0-9_.+-]+)$")
    set(candidate "${CMAKE_MATCH_1}")
  elseif(line MATCHES "^([A-Za-z0-9_.+-]+)([ \\t]+.*)?$")
    set(candidate "${CMAKE_MATCH_1}")
  else()
    continue()
  endif()

  string(TOLOWER "${candidate}" candidate_lower)
  list(FIND service_list_headers "${candidate_lower}" header_index)
  if(NOT header_index EQUAL -1)
    continue()
  endif()

  list(APPEND service_names "${candidate}")
endforeach()

list(REMOVE_DUPLICATES service_names)
list(LENGTH service_names service_count)
if(service_count EQUAL 0)
  message(FATAL_ERROR
    "No services parsed from 'melt -query ${LIST_QUERY}'.\n"
    "Raw output:\n${list_output}\n"
    "stderr:\n${list_error}"
  )
endif()

set(failure_text "")
set(failure_count 0)
set(validated_count 0)

foreach(service_name IN LISTS service_names)
  execute_process(
    COMMAND "${MLT_EXECUTABLE}" -query "${QUERY_PREFIX}${service_name}"
    RESULT_VARIABLE service_result
    OUTPUT_VARIABLE service_yaml
    ERROR_VARIABLE service_error
  )

  if(NOT service_result EQUAL 0)
    message(STATUS "[runtime-kwalify:${LIST_QUERY}] ${service_name} ... FAIL")
    string(STRIP "${service_error}" detail)
    string(REPLACE "\n" "\n    " detail "    ${detail}")
    string(APPEND failure_text "\n[${service_name}] melt query failed (exit=${service_result})\n${detail}\n")
    math(EXPR failure_count "${failure_count} + 1")
    continue()
  endif()

  string(STRIP "${service_yaml}" service_yaml_stripped)
  if(service_yaml_stripped STREQUAL "")
    message(STATUS "[runtime-kwalify:${LIST_QUERY}] ${service_name} ... FAIL")
    string(APPEND failure_text "\n[${service_name}] melt returned empty metadata\n")
    math(EXPR failure_count "${failure_count} + 1")
    continue()
  endif()

  string(REGEX REPLACE "[^A-Za-z0-9_.-]" "_" safe_service_name "${service_name}")
  set(service_yaml_file "${CMAKE_CURRENT_BINARY_DIR}/kwalify_${LIST_QUERY}_${safe_service_name}.yml")
  file(WRITE "${service_yaml_file}" "${service_yaml}")

  execute_process(
    COMMAND "${KWALIFY_EXECUTABLE}" -f "${METASCHEMA_FILE}" "${service_yaml_file}"
    RESULT_VARIABLE schema_result
    OUTPUT_VARIABLE schema_output
    ERROR_VARIABLE schema_error
  )

  set(schema_text "${schema_output}\n${schema_error}")
  set(schema_invalid_output FALSE)
  if(schema_text MATCHES "INVALID")
    set(schema_invalid_output TRUE)
  endif()

  if((NOT schema_result EQUAL 0) OR schema_invalid_output)
    message(STATUS "[runtime-kwalify:${LIST_QUERY}] ${service_name} ... FAIL")
    # Clean up kwalify output (stdout + stderr): strip surrounding whitespace, collapse multiple blank lines
    set(raw_detail "${schema_output}\n${schema_error}")
    string(STRIP "${raw_detail}" detail)
    string(REGEX REPLACE "\n[ \t]*\n[ \t]*" "\n" detail "${detail}")
    if(detail STREQUAL "")
      set(detail "(no output from kwalify)")
    endif()
    string(REPLACE "\n" "\n    " detail "    ${detail}")
    string(APPEND failure_text "\n[${service_name}]\n${detail}\n")
    math(EXPR failure_count "${failure_count} + 1")
    continue()
  endif()

  math(EXPR validated_count "${validated_count} + 1")
  message(STATUS "[runtime-kwalify:${LIST_QUERY}] ${service_name} ... PASS")
endforeach()

math(EXPR total_count "${validated_count} + ${failure_count}")
message(STATUS "[runtime-kwalify:${LIST_QUERY}] ${total_count} tested, ${failure_count} failed")
if(failure_count GREATER 0)
  message(FATAL_ERROR "${failure_text}")
endif()
