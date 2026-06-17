add_executable(iggy_scenario_toml_runner
  apps/scenario_toml_runner/IggyScenarioTomlRunner.cpp
)

target_link_libraries(iggy_scenario_toml_runner PRIVATE
  iggy_engine
)

target_include_directories(iggy_scenario_toml_runner PRIVATE src)
