cmake_minimum_required(VERSION 3.16)

# mlink_device 测试构建配置。
# 注意：这里只负责编译测试可执行程序，不再通过 CTest 注册 add_test，
# 测试运行推荐直接使用 srun 或手动执行可执行文件。

# 公共测试框架源码
set(MLINK_DEVICE_TEST_COMMON
    ${CMAKE_CURRENT_SOURCE_DIR}/test_framework.c
)

# 主 Demo / 基本 sanity 测试：mlink_test
add_executable(mlink_device_mlink_test
    ${MLINK_DEVICE_TEST_COMMON}
    ${CMAKE_CURRENT_SOURCE_DIR}/mlink_test.c
)
target_include_directories(mlink_device_mlink_test
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/mcp
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/utils
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/transport
        ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(mlink_device_mlink_test
    PRIVATE
        mlink_device
        Threads::Threads
)
if(UNIX AND NOT APPLE)
    target_link_libraries(mlink_device_mlink_test PRIVATE m)
endif()

# 端到端（E2E）测试
add_executable(mlink_device_test_e2e
    ${MLINK_DEVICE_TEST_COMMON}
    ${CMAKE_CURRENT_SOURCE_DIR}/e2e/test_e2e.c
)
target_include_directories(mlink_device_test_e2e
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/mcp
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/utils
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/transport
        ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(mlink_device_test_e2e
    PRIVATE
        mlink_device
        Threads::Threads
)
if(UNIX AND NOT APPLE)
    target_link_libraries(mlink_device_test_e2e PRIVATE m)
endif()

# 如需更多单元/集成测试，可在此按相同模式增加 add_executable，
# 通过 srun 或直接执行二进制来运行。


