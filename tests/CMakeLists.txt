cmake_minimum_required(VERSION 3.16)

set(MLINK_DEVICE_TEST_COMMON
    ${CMAKE_CURRENT_SOURCE_DIR}/test_framework.c
)

function(add_mlink_device_test target source)
    add_executable(${target}
        ${MLINK_DEVICE_TEST_COMMON}
        ${CMAKE_CURRENT_SOURCE_DIR}/${source}
    )
    target_include_directories(${target}
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/../include
            ${CMAKE_CURRENT_SOURCE_DIR}/../include/mcp
            ${CMAKE_CURRENT_SOURCE_DIR}/../include/utils
            ${CMAKE_CURRENT_SOURCE_DIR}/../include/transport
            ${CMAKE_CURRENT_SOURCE_DIR}
    )
    target_link_libraries(${target}
        PRIVATE
            mlink_device
            Threads::Threads
    )
    if(UNIX AND NOT APPLE)
        target_link_libraries(${target} PRIVATE m)
    endif()
    install(
        TARGETS ${target}
        RUNTIME DESTINATION bin
    )
endfunction()

add_mlink_device_test(
    mlink_device_api_contract_test
    api_contract/test_api_contract.c
)

add_mlink_device_test(
    mlink_device_protocol_e2e_test
    protocol/test_protocol_e2e.c
)

add_mlink_device_test(
    mlink_device_lifecycle_test
    lifecycle/test_lifecycle.c
)
