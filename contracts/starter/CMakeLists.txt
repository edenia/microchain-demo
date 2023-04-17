# Build starter.wasm contract
add_executable(starter src/starter.cpp src/events.cpp)
target_include_directories(starter PUBLIC include)
target_link_libraries(starter eosio-contract-simple-malloc)
set_target_properties(starter PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${ROOT_BINARY_DIR})

# Generate starter.abi
add_executable(starter-abigen src/starter.cpp)
target_include_directories(starter-abigen PRIVATE include)
target_link_libraries(starter-abigen eosio-contract-abigen)
add_custom_command(TARGET starter-abigen POST_BUILD
    COMMAND ${ROOT_BINARY_DIR}/cltester starter-abigen.wasm >${ROOT_BINARY_DIR}/starter.abi
)

function(create_testing test_file suffix)
    add_executable(${test_file}${suffix} tests/${test_file}.cpp src/starter.cpp)
    target_include_directories(${test_file}${suffix} PUBLIC include)
    target_include_directories(${test_file}${suffix} PUBLIC ./tests/include)
    target_link_libraries(${test_file}${suffix} catch2 cltestlib${suffix})
    set_target_properties(${test_file}${suffix} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${ROOT_BINARY_DIR})
endfunction()

create_testing("test-starter" "")
create_testing("test-starter" "-debug")
add_tester_test(test-starter)

function(add_starter_microchain suffix)
    add_executable(starter-micro-chain${suffix}
        src/starter-micro-chain.cpp
    )
    target_link_libraries(starter-micro-chain${suffix} clchain${suffix} eosio-contracts-wasi-polyfill${suffix})
    target_include_directories(starter-micro-chain${suffix} PRIVATE
        include
        ../../libraries/eosiolib/contracts/include
        ../../libraries/eosiolib/core/include
    )
    set_target_properties(starter-micro-chain${suffix} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${ROOT_BINARY_DIR})
    target_link_options(starter-micro-chain${suffix} PRIVATE
        -Wl,--stack-first
        -Wl,--no-entry
        -Wl,-z,stack-size=8192
        -nostdlib
        -lc++
        -lc++abi
        -lc
        ${WASI_SDK_PREFIX}/lib/clang/11.0.0/lib/wasi/libclang_rt.builtins-wasm32.a
    )
    add_dependencies(starter-micro-chain${suffix} starter)
endfunction()

add_starter_microchain("")