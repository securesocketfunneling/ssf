function(bin2c varname input output)
  get_filename_component(output ${output} ABSOLUTE BASE_DIR ${CMAKE_CURRENT_BINARY_DIR})
  message("Generating C header file ${output} from binary file ${input}")
  file(READ ${input} _data_hex HEX)

  string(REGEX MATCH "([^/]+)$" _filename ${input})
  string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," _data_hex ${_data_hex})
  file(WRITE ${output} "unsigned char ${varname}[] = { ${_data_hex} };")
endfunction(bin2c)
