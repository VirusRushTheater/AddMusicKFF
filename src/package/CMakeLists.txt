# Generates package files for each folder here.

function(create_file_package SRC_FOLDER)
	set(SRC_FOLDER_ABS ${CMAKE_CURRENT_SOURCE_DIR}/${SRC_FOLDER})
	file(GLOB_RECURSE PACKAGE_FOLDERS
		${SRC_FOLDER_ABS}/*)

	set(BLOB_CONTENTS )
	set(BLOB_FILENAMES )
	set(BLOB_LENGTHS )
	set(BLOB_ACCUM_LENGTHS "0")

	set(CURRENT_ACCUM_LENGTH "0")

	# For each file in the folder (recursive)
	foreach(PACKAGE_FILE_I ${PACKAGE_FOLDERS})
		# PACKAGE_FOLDER_I_REL is the relative name of the file in the package.
		file(RELATIVE_PATH PACKAGE_FILE_I_REL ${SRC_FOLDER_ABS} ${PACKAGE_FILE_I})

		# Append lengths
		file(SIZE ${PACKAGE_FILE_I} FILE_SIZE_I)

		# Append contents as only one blob. Will delimit using ranges.
		file(READ ${PACKAGE_FILE_I} FILE_CONTENTS_I HEX)
		string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1, " FILE_CONTENTS_I_HDR "${FILE_CONTENTS_I}")
		set(BLOB_CONTENTS "${BLOB_CONTENTS}${FILE_CONTENTS_I_HDR}")

		# Append all these metadata that tells files apart from within the blob.
		list(APPEND BLOB_FILENAMES "\"${PACKAGE_FILE_I_REL}\"")
		list(APPEND BLOB_LENGTHS ${FILE_SIZE_I})
		math(EXPR CURRENT_ACCUM_LENGTH "${CURRENT_ACCUM_LENGTH} + ${FILE_SIZE_I}")
		list(APPEND BLOB_ACCUM_LENGTHS ${CURRENT_ACCUM_LENGTH})
	endforeach()

	# Remove leading commas from every list.
	string(REGEX REPLACE ", *$" "" BLOB_CONTENTS, ${BLOB_CONTENTS})

	list(LENGTH BLOB_FILENAMES BLOB_FILE_AMOUNT)

	# Turn semicolons into commas
	list(JOIN BLOB_FILENAMES ", " BLOB_FILENAMES)
	list(JOIN BLOB_LENGTHS ", " BLOB_LENGTHS)
	list(JOIN BLOB_ACCUM_LENGTHS ", " BLOB_ACCUM_LENGTHS)

	set(BLOB_FULL_LENGTH ${CURRENT_ACCUM_LENGTH})
	string(REGEX REPLACE "[\/\-]" "_" BLOB_NAME ${SRC_FOLDER})
	
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/packageblob.h.in "packageblob_${SRC_FOLDER}.h")
endfunction()

create_file_package(asm)
create_file_package(boilerplate)