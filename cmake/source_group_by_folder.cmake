macro(source_group_by_folder target)
  set(SOURCE_GROUP_DELIMITER "/")

  get_target_property(target_src ${target} SOURCES)

  set(last_dir "")
  set(dir_files "")

  foreach(file ${target_src})
    get_filename_component(dir "${file}" PATH)
    if (NOT "${dir}" STREQUAL "${last_dir}")
      if (dir_files)
        source_group("${last_dir}" FILES ${dir_files})
      endif (dir_files)
      set(dir_files "")
    endif (NOT "${dir}" STREQUAL "${last_dir}")
    set(dir_files ${dir_files} ${file})
    set(last_dir "${dir}")
  endforeach(file)

  if (files)
    source_group("${last_dir}" FILES ${files})
  endif (files)
endmacro(source_group_by_folder)

