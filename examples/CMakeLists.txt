message(STATUS "[OMNI] Configuring examples")

file(GLOB omni_example_sources CONFIGURE_DEPENDS *.cpp)
list(SORT omni_example_sources)

foreach(source IN LISTS omni_example_sources)
  get_filename_component(example_name "${source}" NAME_WE)

  add_executable("${example_name}" "${source}")
  target_link_libraries("${example_name}" PRIVATE omni::omni)
endforeach()
